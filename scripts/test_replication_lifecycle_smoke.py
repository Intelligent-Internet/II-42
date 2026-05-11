from __future__ import annotations

import argparse
import json
import os
import socket
import subprocess
import tempfile
import time
from pathlib import Path

import psycopg


def run(cmd: list[str], env: dict[str, str] | None = None) -> str:
    result = subprocess.run(
        cmd,
        check=True,
        capture_output=True,
        text=True,
        env=env,
    )
    return result.stdout


def find_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(('127.0.0.1', 0))
        return int(sock.getsockname()[1])


def append_lines(path: Path, lines: list[str]) -> None:
    content = path.read_text(encoding='utf-8')
    if not content.endswith('\n'):
        content += '\n'
    content += '\n'.join(lines) + '\n'
    path.write_text(content, encoding='utf-8')


def wait_ready(dsn: str, timeout_s: float = 20.0) -> None:
    deadline = time.time() + timeout_s
    last_error: str | None = None
    while time.time() < deadline:
        try:
            with psycopg.connect(dsn) as conn:
                with conn.cursor() as cur:
                    cur.execute('SELECT 1')
                    return
        except Exception as exc:  # pragma: no cover
            last_error = str(exc)
            time.sleep(0.25)
    raise RuntimeError(f'database not ready: {last_error}')


def install_extension_via_sql(cur: psycopg.Cursor, sql_path: Path, module: str) -> None:
    sql_text = sql_path.read_text(encoding='utf-8')
    cur.execute('SET LOCAL check_function_bodies = off')
    cur.execute(sql_text.replace('MODULE_PATHNAME', module))
    cur.execute('SET LOCAL check_function_bodies = on')


def standby_index_exists(cur: psycopg.Cursor, index_name: str) -> bool:
    cur.execute('SELECT to_regclass(%s)', (f'public.{index_name}',))
    return cur.fetchone()[0] is not None


def relfilenode(cur: psycopg.Cursor, index_name: str) -> int | None:
    cur.execute(
        'SELECT relfilenode FROM pg_class WHERE oid = %s::regclass',
        (f'public.{index_name}',),
    )
    row = cur.fetchone()
    return None if row is None else int(row[0])


def top_hit(
    cur: psycopg.Cursor,
    index_name: str,
    table_name: str,
    query_ids: list[int],
) -> int | None:
    cur.execute(
        f'''
        SELECT d.id
        FROM public.psql_bm25s_query_ids(
            '{index_name}'::regclass,
            %s::int4[],
            1,
            NULL
        ) h
        JOIN {table_name} d ON d.ctid = h.ctid
        ORDER BY h.score DESC, d.id
        LIMIT 1
        ''',
        (query_ids,),
    )
    row = cur.fetchone()
    return None if row is None else int(row[0])


def index_is_stale(cur: psycopg.Cursor, index_name: str) -> bool:
    cur.execute(
        'SELECT stale FROM public.psql_bm25s_index_details(%s::regclass)',
        (f'public.{index_name}',),
    )
    return bool(cur.fetchone()[0])


def wait_until(description: str, fn, timeout_s: float = 30.0):
    deadline = time.time() + timeout_s
    last_error: str | None = None
    while time.time() < deadline:
        try:
            value = fn()
            if value:
                return value
        except Exception as exc:  # pragma: no cover
            last_error = str(exc)
        time.sleep(0.25)
    raise RuntimeError(
        f'timeout waiting for {description}: {last_error}'
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a primary/standby replication lifecycle smoke test.'
    )
    parser.add_argument(
        '--pg-bindir',
        default='/usr/lib/postgresql/17/bin',
        help='PostgreSQL bindir containing initdb, pg_ctl, and pg_basebackup.',
    )
    parser.add_argument(
        '--bootstrap-sql',
        required=True,
        help='SQL file used to bootstrap the extension manually.',
    )
    parser.add_argument(
        '--module-path',
        required=True,
        help='Absolute path to psql_bm25s.so on the test host.',
    )
    parser.add_argument(
        '--role',
        default=None,
        help='Role name to initialize and connect with. Defaults to $USER.',
    )
    parser.add_argument(
        '--temp-root',
        default='/tmp',
        help='Directory where isolated primary/standby clusters are created.',
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    role = args.role or os.environ.get('USER') or 'postgres'
    pg_bindir = Path(args.pg_bindir)
    initdb = pg_bindir / 'initdb'
    pg_ctl = pg_bindir / 'pg_ctl'
    pg_basebackup = pg_bindir / 'pg_basebackup'
    bootstrap_sql = Path(args.bootstrap_sql)
    module_path = args.module_path
    env = os.environ.copy()
    env.setdefault('LC_ALL', 'C')

    tempdir = Path(
        tempfile.mkdtemp(
            prefix='psql_bm25s_repl_lifecycle_',
            dir=args.temp_root,
        )
    )
    primary_data = tempdir / 'primary'
    standby_data = tempdir / 'standby'
    primary_socket = tempdir / 'primary_socket'
    standby_socket = tempdir / 'standby_socket'
    primary_log = tempdir / 'primary.log'
    standby_log = tempdir / 'standby.log'
    primary_socket.mkdir()
    standby_socket.mkdir()
    primary_port = find_free_port()
    standby_port = find_free_port()
    primary_dsn = (
        f'dbname=postgres user={role} host={primary_socket} port={primary_port}'
    )
    standby_dsn = (
        f'dbname=postgres user={role} host={standby_socket} port={standby_port}'
    )

    summary: dict[str, object] = {
        'result': 'ok',
        'tempdir': str(tempdir),
        'primary_port': primary_port,
        'standby_port': standby_port,
        'bootstrap_sql': str(bootstrap_sql),
        'module_path': module_path,
        'checks': {},
    }

    try:
        run(
            [
                str(initdb),
                '-D',
                str(primary_data),
                '-U',
                role,
                '-A',
                'trust',
                '--no-locale',
            ],
            env=env,
        )
        append_lines(
            primary_data / 'pg_hba.conf',
            [
                'host replication all 127.0.0.1/32 trust',
                'host all all 127.0.0.1/32 trust',
            ],
        )

        run(
            [
                str(pg_ctl),
                '-D',
                str(primary_data),
                '-l',
                str(primary_log),
                '-o',
                ' '.join(
                    [
                        f"-k '{primary_socket}'",
                        f'-p {primary_port}',
                        "-c listen_addresses='127.0.0.1'",
                        '-c wal_level=replica',
                        '-c max_wal_senders=5',
                        '-c hot_standby=on',
                    ]
                ),
                'start',
            ],
            env=env,
        )
        wait_ready(primary_dsn)

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                install_extension_via_sql(cur, bootstrap_sql, module_path)
                cur.execute(
                    '''
                    CREATE TABLE docs_live (
                        id int PRIMARY KEY,
                        token_ids int4[] NOT NULL
                    )
                    '''
                )
                cur.executemany(
                    'INSERT INTO docs_live (id, token_ids) VALUES (%s, %s)',
                    [
                        (1, [0, 4]),
                        (2, [1, 2]),
                        (3, [0]),
                        (4, [3]),
                    ],
                )
                cur.execute(
                    '''
                    CREATE TABLE docs_manual (
                        id int PRIMARY KEY,
                        token_ids int4[] NOT NULL
                    )
                    '''
                )
                cur.executemany(
                    'INSERT INTO docs_manual (id, token_ids) VALUES (%s, %s)',
                    [
                        (1, [0, 4]),
                        (2, [1]),
                        (3, [0]),
                        (4, [3]),
                    ],
                )
            conn.commit()

        run(
            [
                str(pg_basebackup),
                '-D',
                str(standby_data),
                '-R',
                '-X',
                'stream',
                '-h',
                '127.0.0.1',
                '-p',
                str(primary_port),
                '-U',
                role,
            ],
            env=env,
        )
        run(
            [
                str(pg_ctl),
                '-D',
                str(standby_data),
                '-l',
                str(standby_log),
                '-o',
                ' '.join(
                    [
                        f"-k '{standby_socket}'",
                        f'-p {standby_port}',
                        "-c listen_addresses='127.0.0.1'",
                        '-c hot_standby=on',
                    ]
                ),
                'start',
            ],
            env=env,
        )
        wait_ready(standby_dsn)

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    '''
                    CREATE INDEX docs_live_idx
                    ON docs_live USING psql_bm25s (token_ids)
                    WITH (
                        method = 'lucene',
                        idf_method = 'lucene',
                        auto_rebuild_threshold = 10
                    )
                    '''
                )
            conn.commit()

        def create_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    if not standby_index_exists(cur, 'docs_live_idx'):
                        return False
                    return (
                        top_hit(cur, 'docs_live_idx', 'docs_live', [0, 4])
                        == 1
                    )

        wait_until('standby create index', create_check)
        summary['checks']['create_index'] = 'ok'

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    'UPDATE docs_live SET token_ids = %s WHERE id = %s',
                    ([0, 4, 4, 4], 3),
                )
            conn.commit()

        def update_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    return (
                        top_hit(cur, 'docs_live_idx', 'docs_live', [0, 4])
                        == 3
                    )

        wait_until('standby update overlay', update_check)
        summary['checks']['update_index'] = 'ok'

        with psycopg.connect(standby_dsn) as conn:
            with conn.cursor() as cur:
                before_relf = relfilenode(cur, 'docs_live_idx')

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute('REINDEX INDEX docs_live_idx')
            conn.commit()

        def reindex_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    after_relf = relfilenode(cur, 'docs_live_idx')
                    if before_relf is None or after_relf is None:
                        return False
                    if before_relf == after_relf:
                        return False
                    hit = top_hit(cur, 'docs_live_idx', 'docs_live', [0, 4])
                    if hit != 3:
                        return False
                    return after_relf

        after_relf = wait_until('standby reindex', reindex_check)
        summary['checks']['reindex'] = {
            'before_relfilenode': before_relf,
            'after_relfilenode': after_relf,
        }

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    '''
                    CREATE INDEX docs_manual_idx
                    ON docs_manual USING psql_bm25s (token_ids)
                    WITH (
                        method = 'lucene',
                        idf_method = 'lucene',
                        consistency = 'manual'
                    )
                    '''
                )
            conn.commit()

        def manual_create_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    if not standby_index_exists(cur, 'docs_manual_idx'):
                        return False
                    return (
                        top_hit(cur, 'docs_manual_idx', 'docs_manual', [0, 4])
                        == 1
                    )

        wait_until('standby manual create index', manual_create_check)

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    'UPDATE docs_manual SET token_ids = %s WHERE id = %s',
                    ([0, 4, 4, 4], 2),
                )
            conn.commit()

        def stale_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    return (
                        index_is_stale(cur, 'docs_manual_idx')
                        and top_hit(
                            cur,
                            'docs_manual_idx',
                            'docs_manual',
                            [0, 4],
                        ) == 1
                    )

        wait_until('standby stale manual index', stale_check)

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    "SELECT public.psql_bm25s_index_refresh("
                    "'docs_manual_idx'::regclass)"
                )
            conn.commit()

        def refresh_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    return (
                        top_hit(cur, 'docs_manual_idx', 'docs_manual', [0, 4])
                        == 2
                    )

        wait_until('standby refresh index', refresh_check)
        summary['checks']['refresh_index'] = 'ok'

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute('DROP INDEX docs_live_idx')
            conn.commit()

        def drop_check():
            with psycopg.connect(standby_dsn) as conn:
                with conn.cursor() as cur:
                    return not standby_index_exists(cur, 'docs_live_idx')

        wait_until('standby drop index', drop_check)
        summary['checks']['drop_index'] = 'ok'

        summary_path = tempdir / 'summary.json'
        summary_path.write_text(
            json.dumps(summary, indent=2),
            encoding='utf-8',
        )
        print(summary_path)
        print(json.dumps(summary, indent=2))
    finally:
        try:
            run(
                [str(pg_ctl), '-D', str(standby_data), 'stop', '-m', 'fast'],
                env=env,
            )
        except Exception:
            pass
        try:
            run(
                [str(pg_ctl), '-D', str(primary_data), 'stop', '-m', 'fast'],
                env=env,
            )
        except Exception:
            pass


if __name__ == '__main__':
    main()
