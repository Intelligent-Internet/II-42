#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import shutil
import socket
import subprocess
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run shared-preload generation cache smoke checks.'
    )
    parser.add_argument(
        '--bindir',
        default=os.environ.get('PG_BINDIR', '/usr/bin'),
        help='PostgreSQL bin directory containing initdb, pg_ctl, and psql.',
    )
    parser.add_argument(
        '--cache-mb',
        type=int,
        default=64,
        help='Main shared-memory generation cache size for the temp cluster.',
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
    env: dict[str, str] | None = None,
    cwd: Path = REPO_ROOT,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        input=input_sql,
        text=True,
        cwd=cwd,
        env=env,
        check=True,
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
) -> str:
    result = run(psql_cmd(args, pgdata, port), input_sql=sql)
    return result.stdout.strip()


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
        conf.write("shared_preload_libraries = 'psql_bm25s'\n")
        conf.write(
            f"psql_bm25s.shared_generation_cache_size = '{args.cache_mb}MB'\n"
        )

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


def require_state_value(state: str, key: str, expected: str) -> None:
    marker = f'{key}={expected}'

    if marker not in state:
        raise AssertionError(f'missing {marker} in state: {state}')


def parse_size(state: str, key: str) -> int:
    match = re.search(rf'{re.escape(key)}=([0-9]+)', state)

    if match is None:
        raise AssertionError(f'missing {key} in state: {state}')
    return int(match.group(1))


def run_smoke(args: argparse.Namespace, pgdata: Path, port: int) -> None:
    psql(
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
        SELECT gs, ARRAY['shared', 'preload', 'generation', gs::text]
        FROM generate_series(1, 1000) gs;
        CREATE INDEX docs_bm25_idx
            ON docs USING psql_bm25s (tokens);
        ''',
    )

    configured = psql(
        args,
        pgdata,
        port,
        'SHOW psql_bm25s.shared_generation_cache_size;',
    )
    if configured != f'{args.cache_mb}MB':
        raise AssertionError(
            'shared preload GUC was not applied: '
            f'configured={configured!r}'
        )

    preload_state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_preload('docs_bm25_idx');",
    )
    if 'tier=shared_preload' not in preload_state:
        raise AssertionError(
            'preload did not warm shared-preload tier: '
            f'{preload_state}'
        )

    first_hits = psql(
        args,
        pgdata,
        port,
        '''
        SELECT count(*)
        FROM public.psql_bm25s_query(
            'docs_bm25_idx'::regclass,
            'shared',
            10,
            NULL,
            true,
            NULL,
            false,
            false
        ) h
        WHERE h.score > 0;
        ''',
    )
    if first_hits != '10':
        raise AssertionError(f'unexpected first backend hits: {first_hits}')

    state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    )
    require_state_value(state, 'share_required', 'true')
    require_state_value(state, 'shared_preload_available', 'true')
    require_state_value(state, 'shared_preload_entries', '1')
    require_state_value(state, 'shared_preload_ready_entries', '1')
    require_state_value(state, 'shared_preload_refcounted_entries', '0')
    if parse_size(state, 'shared_preload_used') <= 0:
        raise AssertionError(f'shared preload arena was not used: {state}')

    cleared = psql(
        args,
        pgdata,
        port,
        'SELECT public.psql_bm25s_generation_cache_clear();',
    )
    if int(cleared) <= 0:
        raise AssertionError(f'cache clear did not clear anything: {cleared}')

    cleared_state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    )
    require_state_value(cleared_state, 'shared_preload_entries', '0')
    require_state_value(cleared_state, 'shared_preload_ready_entries', '0')
    require_state_value(cleared_state, 'shared_preload_used', '0')

    reloaded_state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_preload('docs_bm25_idx');",
    )
    if 'tier=shared_preload' not in reloaded_state:
        raise AssertionError(
            'preload after clear did not reuse the shared-preload arena: '
            f'{reloaded_state}'
        )

    second_hits = psql(
        args,
        pgdata,
        port,
        '''
        SELECT count(*)
        FROM public.psql_bm25s_query(
            'docs_bm25_idx'::regclass,
            'preload',
            10,
            NULL,
            true,
            NULL,
            false,
            false
        ) h
        WHERE h.score > 0;
        ''',
    )
    if second_hits != '10':
        raise AssertionError(f'unexpected second backend hits: {second_hits}')

    final_state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    )
    require_state_value(final_state, 'shared_preload_refcounted_entries', '0')


def main() -> None:
    args = parse_args()
    workdir = Path(tempfile.mkdtemp(
        prefix='psql_bm25s_shared_preload_',
        dir='/tmp',
    ))
    pgdata = workdir / 'pgdata'
    port = free_port()

    try:
        init_cluster(args, pgdata, port)
        run_smoke(args, pgdata, port)
        print('shared preload generation cache smoke passed')
    finally:
        stop_cluster(args, pgdata)
        if not args.keep:
            shutil.rmtree(workdir, ignore_errors=True)
        else:
            print(f'kept temporary cluster at {workdir}')


if __name__ == '__main__':
    main()
