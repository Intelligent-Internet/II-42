from __future__ import annotations

import os
import shutil
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


def resolve_pg_config() -> str:
    candidates = [
        os.environ.get('PG_CONFIG'),
        shutil.which('pg_config'),
        '/usr/bin/pg_config',
    ]

    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate

    raise FileNotFoundError('could not locate pg_config')


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


def maintenance_state(cur: psycopg.Cursor, index_name: str) -> str:
    cur.execute(
        f'''
        SELECT format(
            'psql_bm25s_maintenance_state(rebuilds=%s, '
            'pending_writes=%s, pending_deletes=%s, delta_records=%s, '
            'delta_bytes=%s, stale=%s)',
            rebuilds,
            pending_writes,
            pending_deletes,
            delta_records,
            delta_bytes,
            stale
        )
        FROM public.psql_bm25s_index_details(%s::regclass)
        ''',
        (index_name,),
    )
    value = cur.fetchone()[0]
    if isinstance(value, bytes):
        return value.decode('utf-8')
    return str(value)


def top_hit_id(cur: psycopg.Cursor, index_name: str, table_name: str) -> int:
    cur.execute(
        f'''
        SELECT d.id
        FROM public.psql_bm25s_query_ids(
            %s::regclass,
            ARRAY[0,4]::int4[],
            1,
            NULL
        ) h
        JOIN {table_name} d ON d.ctid = h.ctid
        ORDER BY h.score DESC, d.id
        LIMIT 1
        ''',
        (index_name,),
    )
    return int(cur.fetchone()[0])


def matching_ids(
    cur: psycopg.Cursor,
    index_name: str,
    table_name: str,
    query_ids: list[int],
) -> list[int]:
    cur.execute(
        f'''
        SELECT d.id
        FROM public.psql_bm25s_query_ids(
            %s::regclass,
            %s::int4[],
            8,
            NULL
        ) h
        JOIN {table_name} d ON d.ctid = h.ctid
        ORDER BY h.score DESC, d.id
        ''',
        (index_name, query_ids),
    )
    return [int(row[0]) for row in cur.fetchall()]


def wait_for_standby_insert(
    dsn: str,
    timeout_s: float = 15.0,
) -> tuple[str, int]:
    deadline = time.time() + timeout_s
    last_error: str | None = None

    while time.time() < deadline:
        try:
            with psycopg.connect(dsn) as conn:
                with conn.cursor() as cur:
                    state = maintenance_state(cur, 'docs_bm25_idx')
                    hit = top_hit_id(cur, 'docs_bm25_idx', 'docs')
                    if (
                        'pending_writes=1' in state and
                        'delta_records=1' in state and
                        hit == 5
                    ):
                        return state, hit
        except Exception as exc:  # pragma: no cover
            last_error = str(exc)
        time.sleep(0.25)

    raise AssertionError(
        f'standby did not reach expected state; last_error={last_error}'
    )


def wait_for_standby_update(
    dsn: str,
    timeout_s: float = 15.0,
) -> tuple[str, int]:
    deadline = time.time() + timeout_s
    last_error: str | None = None

    while time.time() < deadline:
        try:
            with psycopg.connect(dsn) as conn:
                with conn.cursor() as cur:
                    state = maintenance_state(cur, 'docs_update_bm25_idx')
                    hit = top_hit_id(
                        cur,
                        'docs_update_bm25_idx',
                        'docs_update',
                    )
                    if (
                        'pending_writes=1' in state and
                        'delta_records=1' in state and
                        hit == 1
                    ):
                        return state, hit
        except Exception as exc:  # pragma: no cover
            last_error = str(exc)
        time.sleep(0.25)

    raise AssertionError(
        f'standby update state not ready; last_error={last_error}'
    )


def wait_for_standby_delete(
    dsn: str,
    timeout_s: float = 15.0,
) -> tuple[str, list[int]]:
    deadline = time.time() + timeout_s
    last_error: str | None = None

    while time.time() < deadline:
        try:
            with psycopg.connect(dsn) as conn:
                with conn.cursor() as cur:
                    state = maintenance_state(cur, 'docs_delete_bm25_idx')
                    ids = matching_ids(
                        cur,
                        'docs_delete_bm25_idx',
                        'docs_delete',
                        [1, 2],
                    )
                    if (
                        'pending_deletes=1' in state and
                        'delta_records=1' in state and
                        2 not in ids
                    ):
                        return state, ids
        except Exception as exc:  # pragma: no cover
            last_error = str(exc)
        time.sleep(0.25)

    raise AssertionError(
        f'standby delete state not ready; last_error={last_error}'
    )


def main() -> None:
    pg_config = resolve_pg_config()
    bindir = Path(run([pg_config, '--bindir']).strip())
    initdb = bindir / 'initdb'
    pg_ctl = bindir / 'pg_ctl'
    pg_basebackup = bindir / 'pg_basebackup'

    tempdir = Path(
        tempfile.mkdtemp(prefix='psql_bm25s_v2_repl_', dir='/tmp')
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
    env = os.environ.copy()
    env.setdefault('LC_ALL', 'C')

    try:
        run(
            [
                str(initdb),
                '-D',
                str(primary_data),
                '-U',
                'postgres',
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
                " ".join(
                    [
                        f"-k '{primary_socket}'",
                        f"-p {primary_port}",
                        "-c listen_addresses='127.0.0.1'",
                        "-c wal_level=replica",
                        "-c max_wal_senders=5",
                        "-c hot_standby=on",
                    ]
                ),
                'start',
            ],
            env=env,
        )

        primary_dsn = (
            f'dbname=postgres user=postgres host={primary_socket} '
            f'port={primary_port}'
        )
        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute('CREATE EXTENSION psql_bm25s')
                cur.execute(
                    '''
                    CREATE TABLE docs (
                        id int PRIMARY KEY,
                        token_ids int4[] NOT NULL
                    )
                    '''
                )
                cur.executemany(
                    'INSERT INTO docs (id, token_ids) VALUES (%s, %s)',
                    [
                        (1, [0, 0, 1]),
                        (2, [1, 2]),
                        (3, [0, 2, 2]),
                        (4, [3]),
                    ],
                )
                cur.execute(
                    '''
                    CREATE INDEX docs_bm25_idx
                        ON docs USING psql_bm25s (token_ids)
                        WITH (
                            method = 'bm25+',
                            idf_method = 'bm25+',
                            auto_rebuild_threshold = 10
                        )
                    '''
                )
                cur.execute(
                    '''
                    CREATE TABLE docs_delete (
                        id int PRIMARY KEY,
                        token_ids int4[] NOT NULL
                    )
                    '''
                )
                cur.executemany(
                    'INSERT INTO docs_delete (id, token_ids) VALUES (%s, %s)',
                    [
                        (1, [0, 0, 1]),
                        (2, [1, 2]),
                        (3, [0, 2, 2]),
                        (4, [3]),
                    ],
                )
                cur.execute(
                    '''
                    CREATE INDEX docs_delete_bm25_idx
                        ON docs_delete USING psql_bm25s (token_ids)
                        WITH (
                            method = 'bm25+',
                            idf_method = 'bm25+',
                            auto_rebuild_threshold = 10
                        )
                    '''
                )
                cur.execute(
                    '''
                    CREATE TABLE docs_update (
                        id int PRIMARY KEY,
                        token_ids int4[] NOT NULL
                    )
                    '''
                )
                cur.executemany(
                    'INSERT INTO docs_update (id, token_ids) VALUES (%s, %s)',
                    [
                        (1, [0, 0, 1]),
                        (2, [1, 2]),
                        (3, [0, 2, 2]),
                        (4, [3]),
                    ],
                )
                cur.execute(
                    '''
                    CREATE INDEX docs_update_bm25_idx
                        ON docs_update USING psql_bm25s (token_ids)
                        WITH (
                            method = 'bm25+',
                            idf_method = 'bm25+',
                            auto_rebuild_threshold = 10
                        )
                    '''
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
                'postgres',
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
                " ".join(
                    [
                        f"-k '{standby_socket}'",
                        f"-p {standby_port}",
                        "-c listen_addresses='127.0.0.1'",
                        "-c hot_standby=on",
                    ]
                ),
                'start',
            ],
            env=env,
        )

        with psycopg.connect(primary_dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    'INSERT INTO docs (id, token_ids) VALUES (%s, %s)',
                    (5, [0, 4]),
                )
                cur.execute(
                    'UPDATE docs_update SET token_ids = %s WHERE id = %s',
                    ([0, 4], 1),
                )
                cur.execute('DELETE FROM docs_delete WHERE id = 2')
                conn.commit()
                state = maintenance_state(cur, 'docs_bm25_idx')
                assert 'pending_writes=1' in state, state
                assert 'delta_records=1' in state, state
                update_state = maintenance_state(cur, 'docs_update_bm25_idx')
                assert 'pending_writes=1' in update_state, update_state
                assert 'delta_records=1' in update_state, update_state
        with psycopg.connect(primary_dsn, autocommit=True) as conn:
            with conn.cursor() as cur:
                cur.execute('VACUUM docs_delete')
                delete_state = maintenance_state(cur, 'docs_delete_bm25_idx')
                assert 'pending_deletes=1' in delete_state, delete_state
                assert 'delta_records=1' in delete_state, delete_state

        standby_dsn = (
            f'dbname=postgres user=postgres host={standby_socket} '
            f'port={standby_port}'
        )
        state, hit = wait_for_standby_insert(standby_dsn)
        assert 'pending_writes=1' in state, state
        assert 'delta_records=1' in state, state
        assert hit == 5
        update_state, update_hit = wait_for_standby_update(standby_dsn)
        assert 'pending_writes=1' in update_state, update_state
        assert 'delta_records=1' in update_state, update_state
        assert update_hit == 1
        delete_state, ids = wait_for_standby_delete(standby_dsn)
        assert 'pending_deletes=1' in delete_state, delete_state
        assert 'delta_records=1' in delete_state, delete_state
        assert 2 not in ids

        run(
            [str(pg_ctl), '-D', str(standby_data), 'stop', '-m', 'fast'],
            env=env,
        )
        run(
            [str(pg_ctl), '-D', str(primary_data), 'stop', '-m', 'fast'],
            env=env,
        )
    finally:
        shutil.rmtree(tempdir, ignore_errors=True)


if __name__ == '__main__':
    main()
