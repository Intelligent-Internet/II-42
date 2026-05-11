from __future__ import annotations

import os
import shutil
import socket
import subprocess
import tempfile
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


def main() -> None:
    pg_config = resolve_pg_config()
    bindir = Path(run([pg_config, '--bindir']).strip())
    initdb = bindir / 'initdb'
    pg_ctl = bindir / 'pg_ctl'

    tempdir = Path(tempfile.mkdtemp(prefix='psql_bm25s_v2_restart_'))
    data_dir = tempdir / 'data'
    socket_dir = tempdir / 'socket'
    logfile = tempdir / 'postgres.log'
    socket_dir.mkdir()
    port = find_free_port()
    env = os.environ.copy()
    env.setdefault('LC_ALL', 'C')

    try:
        run(
            [
                str(initdb),
                '-D',
                str(data_dir),
                '-U',
                'postgres',
                '-A',
                'trust',
                '--no-locale',
            ],
            env=env,
        )

        run(
            [
                str(pg_ctl),
                '-D',
                str(data_dir),
                '-l',
                str(logfile),
                '-o',
                f"-k '{socket_dir}' -p {port} -c listen_addresses=''",
                'start',
            ],
            env=env,
        )

        dsn = f'dbname=postgres user=postgres host={socket_dir} port={port}'
        with psycopg.connect(dsn) as conn:
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
                    'INSERT INTO docs (id, token_ids) VALUES (%s, %s)',
                    (5, [0, 4]),
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
                cur.execute(
                    'UPDATE docs_update SET token_ids = %s WHERE id = %s',
                    ([0, 4], 1),
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
                cur.execute('DELETE FROM docs_delete WHERE id = 2')
                conn.commit()
                conn.autocommit = True
                cur.execute('VACUUM docs_delete')
                conn.autocommit = False

                state = maintenance_state(cur, 'docs_bm25_idx')
                assert 'rebuilds=1' in state, state
                assert 'pending_writes=1' in state, state
                assert 'delta_records=1' in state, state
                update_state = maintenance_state(cur, 'docs_update_bm25_idx')
                assert 'rebuilds=1' in update_state, update_state
                assert 'pending_writes=1' in update_state, update_state
                assert 'delta_records=1' in update_state, update_state
                delete_state = maintenance_state(cur, 'docs_delete_bm25_idx')
                assert 'rebuilds=1' in delete_state, delete_state
                assert 'pending_deletes=1' in delete_state, delete_state
                assert 'delta_records=1' in delete_state, delete_state

        run([str(pg_ctl), '-D', str(data_dir), 'stop', '-m', 'fast'], env=env)
        run(
            [
                str(pg_ctl),
                '-D',
                str(data_dir),
                '-l',
                str(logfile),
                '-o',
                f"-k '{socket_dir}' -p {port} -c listen_addresses=''",
                'start',
            ],
            env=env,
        )

        with psycopg.connect(dsn) as conn:
            with conn.cursor() as cur:
                state = maintenance_state(cur, 'docs_bm25_idx')
                assert 'rebuilds=1' in state, state
                assert 'pending_writes=1' in state, state
                assert 'delta_records=1' in state, state
                assert top_hit_id(cur, 'docs_bm25_idx', 'docs') == 5
                state = maintenance_state(cur, 'docs_bm25_idx')
                assert 'rebuilds=1' in state, state
                assert 'pending_writes=1' in state, state
                assert 'delta_records=1' in state, state

                update_state = maintenance_state(cur, 'docs_update_bm25_idx')
                assert 'rebuilds=1' in update_state, update_state
                assert 'pending_writes=1' in update_state, update_state
                assert 'delta_records=1' in update_state, update_state
                assert top_hit_id(
                    cur,
                    'docs_update_bm25_idx',
                    'docs_update',
                ) == 1
                update_state = maintenance_state(cur, 'docs_update_bm25_idx')
                assert 'rebuilds=1' in update_state, update_state
                assert 'pending_writes=1' in update_state, update_state
                assert 'delta_records=1' in update_state, update_state

                delete_state = maintenance_state(cur, 'docs_delete_bm25_idx')
                assert 'rebuilds=1' in delete_state, delete_state
                assert 'pending_deletes=1' in delete_state, delete_state
                assert 'delta_records=1' in delete_state, delete_state
                assert 2 not in matching_ids(
                    cur,
                    'docs_delete_bm25_idx',
                    'docs_delete',
                    [1, 2],
                )
                delete_state = maintenance_state(cur, 'docs_delete_bm25_idx')
                assert 'rebuilds=1' in delete_state, delete_state
                assert 'pending_deletes=1' in delete_state, delete_state
                assert 'delta_records=1' in delete_state, delete_state

        run([str(pg_ctl), '-D', str(data_dir), 'stop', '-m', 'fast'], env=env)
    finally:
        shutil.rmtree(tempdir, ignore_errors=True)


if __name__ == '__main__':
    main()
