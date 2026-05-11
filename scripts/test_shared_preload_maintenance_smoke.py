#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
STATE_RE = re.compile(
    r'pending_writes=(\d+), pending_deletes=(\d+), '
    r'delta_records=(\d+), delta_bytes=\d+, stale=([tf])'
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a shared-preload maintenance supervisor smoke test.'
    )
    parser.add_argument(
        '--bindir',
        default=os.environ.get('PG_BINDIR', '/usr/bin'),
        help='PostgreSQL bin directory containing initdb, pg_ctl, and psql.',
    )
    parser.add_argument(
        '--row-count',
        type=int,
        default=5000,
        help='Rows to build before creating the eventual index.',
    )
    parser.add_argument(
        '--timeout',
        type=int,
        default=30,
        help='Seconds to wait for background convergence.',
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
    cwd: Path = REPO_ROOT,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        input=input_sql,
        text=True,
        cwd=cwd,
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


def psql(args: argparse.Namespace, pgdata: Path, port: int, sql: str) -> str:
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
        conf.write("psql_bm25s.shared_generation_cache_size = '64MB'\n")
        conf.write('psql_bm25s.maintenance_worker_limit = 1\n')
        conf.write("psql_bm25s.maintenance_timer_interval_ms = '1000ms'\n")

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
    subprocess.run(
        [
            str(Path(args.bindir) / 'pg_ctl'),
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


def setup(args: argparse.Namespace, pgdata: Path, port: int) -> None:
    psql(
        args,
        pgdata,
        port,
        f'''
        CREATE EXTENSION psql_bm25s;
        CREATE TABLE docs (
            id int primary key,
            tokens text[] not null
        );

        INSERT INTO docs
        SELECT
            g,
            ARRAY[
                'shared',
                'maintenance',
                'term' || (g % 101)::text,
                'topic' || (g % 997)::text
            ]
        FROM generate_series(1, {args.row_count}) g;

        CREATE INDEX docs_bm25_idx
            ON docs USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1,
                auto_preload = 1
            );

        INSERT INTO docs VALUES
            ({args.row_count + 1}, ARRAY['shared','maintenance','delta']);
        ''',
    )


def maintenance_state(
    args: argparse.Namespace,
    pgdata: Path,
    port: int,
) -> str:
    return psql(
        args,
        pgdata,
        port,
        '''
        SELECT format(
            'rebuilds=%s, pending_writes=%s, pending_deletes=%s, '
            'delta_records=%s, delta_bytes=%s, stale=%s',
            rebuilds,
            pending_writes,
            pending_deletes,
            delta_records,
            delta_bytes,
            stale
        )
        FROM public.psql_bm25s_index_details('docs_bm25_idx'::regclass);
        ''',
    )


def parse_state(state: str) -> tuple[int, int, int, str]:
    match = STATE_RE.search(state)
    if match is None:
        raise AssertionError(f'could not parse maintenance state: {state}')
    pending_writes, pending_deletes, delta_records, stale = match.groups()
    return (
        int(pending_writes),
        int(pending_deletes),
        int(delta_records),
        stale,
    )


def cache_state(args: argparse.Namespace, pgdata: Path, port: int) -> str:
    return psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    )


def state_value(cache_state_text: str, key: str) -> str:
    match = re.search(rf'{re.escape(key)}=([^,)]*)', cache_state_text)
    if match is None:
        raise AssertionError(
            f'missing {key} in cache state: {cache_state_text}'
        )
    return match.group(1)


def wait_until_current_generation_resident(
    args: argparse.Namespace,
    pgdata: Path,
    port: int,
) -> str:
    deadline = time.monotonic() + args.timeout
    last_state = ''

    while time.monotonic() < deadline:
        last_state = cache_state(args, pgdata, port)
        if (
            state_value(last_state, 'shared_preload_resident') == 'true' and
            state_value(last_state, 'shared_preload_obsolete_entries') == '0'
        ):
            return last_state
        time.sleep(0.5)

    raise AssertionError(
        'maintained generation did not become shared-preload resident: '
        f'{last_state}'
    )


def wait_until_clean(
    args: argparse.Namespace,
    pgdata: Path,
    port: int,
) -> tuple[str, str]:
    deadline = time.monotonic() + args.timeout
    initial_state = maintenance_state(args, pgdata, port)
    last_state = initial_state

    while time.monotonic() < deadline:
        pending_writes, pending_deletes, delta_records, stale = parse_state(
            last_state
        )
        if (
            pending_writes == 0 and
            pending_deletes == 0 and
            delta_records == 0 and
            stale == 'f'
        ):
            return initial_state, last_state
        time.sleep(0.5)
        last_state = maintenance_state(args, pgdata, port)

    raise AssertionError(
        'shared-preload maintenance did not converge before timeout: '
        f'{last_state}'
    )


def main() -> None:
    args = parse_args()
    workdir = Path(tempfile.mkdtemp(
        prefix='psql_bm25s_shared_maintenance_',
        dir='/tmp',
    ))
    pgdata = workdir / 'pgdata'
    port = free_port()

    try:
        init_cluster(args, pgdata, port)
        setup(args, pgdata, port)
        initial_state, final_state = wait_until_clean(args, pgdata, port)
        final_cache_state = wait_until_current_generation_resident(
            args,
            pgdata,
            port,
        )
        print('initial_state=' + initial_state)
        print('final_state=' + final_state)
        print('final_cache_state=' + final_cache_state)
        print('shared preload maintenance smoke passed')
    finally:
        stop_cluster(args, pgdata)
        if not args.keep:
            shutil.rmtree(workdir, ignore_errors=True)
        else:
            print(f'kept temporary cluster at {workdir}')


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        sys.exit(1)
