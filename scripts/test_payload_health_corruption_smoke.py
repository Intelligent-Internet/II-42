#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import socket
import subprocess
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
BLCKSZ = 8192


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run payload-health corruption and rebuild smoke checks.'
    )
    parser.add_argument(
        '--bindir',
        default='/opt/homebrew/opt/postgresql@18/bin',
        help='PostgreSQL bin directory containing initdb, pg_ctl, and psql.',
    )
    parser.add_argument(
        '--keep',
        action='store_true',
        help='Keep the temporary cluster for debugging.',
    )
    return parser.parse_args()


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(('127.0.0.1', 0))
        return int(sock.getsockname()[1])


def run(
    cmd: list[str],
    *,
    input_sql: str | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        input=input_sql,
        text=True,
        cwd=REPO_ROOT,
        check=check,
        capture_output=True,
    )


def psql_cmd(args: argparse.Namespace, pgdata: Path, port: int) -> list[str]:
    return [
        str(Path(args.bindir) / 'psql'),
        '-X',
        '-Atq',
        '-h',
        str(pgdata),
        '-p',
        str(port),
        '-d',
        'postgres',
        '-v',
        'ON_ERROR_STOP=1',
    ]


def psql(
    args: argparse.Namespace,
    pgdata: Path,
    port: int,
    sql: str,
    *,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    return run(
        psql_cmd(args, pgdata, port),
        input_sql=sql,
        check=check,
    )


def init_cluster(args: argparse.Namespace, pgdata: Path, port: int) -> None:
    bindir = Path(args.bindir)

    run([
        str(bindir / 'initdb'),
        '-D',
        str(pgdata),
        '-A',
        'trust',
        '-U',
        os.environ.get('USER', 'postgres'),
    ])
    with (pgdata / 'postgresql.conf').open('a', encoding='utf-8') as conf:
        conf.write("\nlisten_addresses = ''\n")
        conf.write(f"unix_socket_directories = '{pgdata}'\n")
        conf.write(f'port = {port}\n')
        conf.write("shared_preload_libraries = ''\n")


def start_cluster(args: argparse.Namespace, pgdata: Path) -> None:
    bindir = Path(args.bindir)

    run([
        str(bindir / 'pg_ctl'),
        '-D',
        str(pgdata),
        '-l',
        str(pgdata / 'postgres.log'),
        '-w',
        'start',
    ])


def stop_cluster(args: argparse.Namespace, pgdata: Path) -> None:
    bindir = Path(args.bindir)

    subprocess.run(
        [
            str(bindir / 'pg_ctl'),
            '-D',
            str(pgdata),
            '-m',
            'fast',
            '-w',
            'stop',
        ],
        text=True,
        cwd=REPO_ROOT,
        check=False,
        capture_output=True,
    )


def run_smoke(args: argparse.Namespace, pgdata: Path, port: int) -> None:
    setup = psql(
        args,
        pgdata,
        port,
        '''
        CREATE EXTENSION psql_bm25s;
        CREATE TABLE docs (
            id int primary key,
            tokens text[] not null
        );
        INSERT INTO docs
        SELECT gs, ARRAY['health', 'payload', 'doc', gs::text]
        FROM generate_series(1, 2000) gs;
        CREATE INDEX docs_bm25_idx
            ON docs USING psql_bm25s (tokens)
            WITH (consistency = 'eventual');
        SELECT pg_relation_filepath('docs_bm25_idx'::regclass);
        CHECKPOINT;
        ''',
    )
    relpath = setup.stdout.strip().splitlines()[-1]

    stop_cluster(args, pgdata)
    index_path = pgdata / relpath
    with index_path.open('r+b') as index_file:
        index_file.truncate(BLCKSZ)
    start_cluster(args, pgdata)

    state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    ).stdout.strip()
    if 'payload_health=corrupt' not in state:
        raise AssertionError(f'corrupt payload was not detected: {state}')
    if 'rebuild_required=true' not in state:
        raise AssertionError(f'rebuild_required was not reported: {state}')

    query = psql(
        args,
        pgdata,
        port,
        '''
        SELECT count(*)
        FROM public.psql_bm25s_query_tokens(
            'docs_bm25_idx'::regclass,
            ARRAY['health'],
            10,
            NULL
        );
        ''',
        check=False,
    )
    if query.returncode == 0:
        raise AssertionError('corrupt payload query unexpectedly succeeded')
    if 'corrupt psql_bm25s index payload' not in query.stderr:
        raise AssertionError(
            'corrupt payload did not fail fast with the expected error: '
            f'{query.stderr}'
        )

    repair = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_index_try_maintain('docs_bm25_idx');",
    ).stdout.strip()
    if 'maintained=true' not in repair:
        raise AssertionError(f'corrupt payload was not rebuilt: {repair}')

    repaired_state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    ).stdout.strip()
    if 'payload_health=ok' not in repaired_state:
        raise AssertionError(f'repaired payload is still unhealthy: {repaired_state}')

    hits = psql(
        args,
        pgdata,
        port,
        '''
        SELECT count(*)
        FROM public.psql_bm25s_query_tokens(
            'docs_bm25_idx'::regclass,
            ARRAY['health'],
            10,
            NULL
        );
        ''',
    ).stdout.strip()
    if hits != '10':
        raise AssertionError(f'unexpected hits after repair: {hits}')


def main() -> None:
    args = parse_args()
    workdir = Path(tempfile.mkdtemp(
        prefix='psql_bm25s_payload_health_',
        dir='/tmp',
    ))
    pgdata = workdir / 'pgdata'
    port = free_port()

    try:
        init_cluster(args, pgdata, port)
        start_cluster(args, pgdata)
        run_smoke(args, pgdata, port)
        print('payload health corruption smoke passed')
    finally:
        stop_cluster(args, pgdata)
        if not args.keep:
            shutil.rmtree(workdir, ignore_errors=True)
        else:
            print(f'kept temporary cluster at {workdir}')


if __name__ == '__main__':
    main()
