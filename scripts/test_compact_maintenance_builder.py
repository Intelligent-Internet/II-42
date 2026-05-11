#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
import os
import re
import shutil
import socket
import subprocess
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
ITEM_POINTER_BYTES = 6
MB = 1024 * 1024
STANDARD_HEADROOM_NUM = 3
STANDARD_HEADROOM_DEN = 5
COMPACT_HEADROOM_NUM = 3
COMPACT_HEADROOM_DEN = 4


def ceil_div(value: int, divisor: int) -> int:
    return (value + divisor - 1) // divisor


def headroom_budget_bytes(estimate: int, numerator: int, denominator: int) -> int:
    return ceil_div(estimate * denominator, numerator)


def headroom_admitted(
    estimate: int,
    budget_bytes: int,
    numerator: int,
    denominator: int,
) -> bool:
    return estimate <= (budget_bytes * numerator) // denominator


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run low-memory maintenance builder smoke checks.'
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
    parser.add_argument(
        '--builder',
        choices=('compact', 'spill'),
        default='compact',
        help='Low-memory builder path to force with the memory budget.',
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
) -> str:
    return run(psql_cmd(args, pgdata, port), input_sql=sql).stdout.strip()


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
        conf.write("psql_bm25s.shared_generation_cache_size = '16MB'\n")
        conf.write("psql_bm25s.preload_timer_interval_ms = '1h'\n")
        conf.write("psql_bm25s.maintenance_timer_interval_ms = '1h'\n")


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


def payload_from_state(state: str) -> int:
    match = re.search(r'index_bytes=([0-9]+), docs=([0-9]+)', state)
    if match is None:
        raise AssertionError(f'could not parse generation state: {state}')
    index_bytes = int(match.group(1))
    docs = int(match.group(2))
    return index_bytes + docs * ITEM_POINTER_BYTES


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
        SELECT gs, ARRAY[
            'compact',
            'maintenance',
            'token-' || (gs % 1000)::text,
            'doc-' || gs::text
        ]
        FROM generate_series(1, 30000) gs;
        CREATE INDEX docs_bm25_idx
            ON docs USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1
            );
        ''',
    )
    state = psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    )
    payload_bytes = payload_from_state(state)
    standard_estimate = payload_bytes * 6
    compact_estimate = payload_bytes * 4
    spill_estimate = payload_bytes * 2
    if args.builder == 'compact':
        target_budget_bytes = headroom_budget_bytes(
            compact_estimate,
            COMPACT_HEADROOM_NUM,
            COMPACT_HEADROOM_DEN,
        )
    else:
        target_budget_bytes = spill_estimate
    budget_mb = math.ceil(target_budget_bytes / MB)
    budget_bytes = budget_mb * MB
    standard_admitted = headroom_admitted(
        standard_estimate,
        budget_bytes,
        STANDARD_HEADROOM_NUM,
        STANDARD_HEADROOM_DEN,
    )
    compact_admitted = headroom_admitted(
        compact_estimate,
        budget_bytes,
        COMPACT_HEADROOM_NUM,
        COMPACT_HEADROOM_DEN,
    )
    spill_admitted = spill_estimate <= budget_bytes
    if args.builder == 'compact':
        budget_matches = compact_admitted and not standard_admitted
    else:
        budget_matches = spill_admitted and not compact_admitted
    if not budget_matches:
        raise AssertionError(
            'test corpus did not produce the requested builder budget window: '
            f'builder={args.builder} payload={payload_bytes} '
            f'spill={spill_estimate} compact={compact_estimate} '
            f'standard={standard_estimate} budget={budget_bytes} '
            f'standard_admitted={standard_admitted} '
            f'compact_admitted={compact_admitted} '
            f'spill_admitted={spill_admitted}'
        )

    psql(
        args,
        pgdata,
        port,
        f'''
        ALTER SYSTEM SET psql_bm25s.maintenance_rebuild_memory_budget =
            '{budget_mb}MB';
        SELECT pg_reload_conf();
        INSERT INTO docs VALUES
            (30001, ARRAY['compact', 'maintenance', 'fresh']);
        ''',
    )
    result = psql(
        args,
        pgdata,
        port,
        '''
        SELECT result
        FROM public.psql_bm25s_index_maintain_due(10)
        WHERE index_oid = 'docs_bm25_idx'::regclass;
        ''',
    )
    expected_builder = f'builder={args.builder}'
    if 'maintained=true' not in result or expected_builder not in result:
        raise AssertionError(
            f'{args.builder} builder was not selected: {result}; '
            f'payload={payload_bytes} budget_mb={budget_mb}'
        )

    psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_preload('docs_bm25_idx');",
    )
    rows = psql(
        args,
        pgdata,
        port,
        '''
        SELECT count(*)
        FROM public.psql_bm25s_query(
            'docs_bm25_idx',
            'fresh',
            10,
            NULL
        ) h
        JOIN docs d ON d.ctid = h.ctid
        WHERE d.id = 30001;
        ''',
    )
    if rows != '1':
        raise AssertionError(f'compact rebuild did not publish fresh row: {rows}')


def main() -> None:
    args = parse_args()
    tmpdir = Path(tempfile.mkdtemp(
        prefix='psql-bm25s-compact-builder-',
        dir='/tmp',
    ))
    pgdata = tmpdir / 'pgdata'
    port = free_port()

    try:
        init_cluster(args, pgdata, port)
        start_cluster(args, pgdata)
        run_smoke(args, pgdata, port)
    finally:
        stop_cluster(args, pgdata)
        if args.keep:
            print(f'kept temporary cluster at {tmpdir}')
        else:
            shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == '__main__':
    main()
