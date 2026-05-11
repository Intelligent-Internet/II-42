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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run shared-preload standby auto-preload smoke checks.'
    )
    parser.add_argument(
        '--bindir',
        default=os.environ.get('PG_BINDIR', '/usr/bin'),
        help='PostgreSQL bin directory containing initdb, pg_ctl, and psql.',
    )
    parser.add_argument(
        '--timeout',
        type=int,
        default=45,
        help='Seconds to wait for standby replay and auto preload.',
    )
    parser.add_argument(
        '--keep',
        action='store_true',
        help='Keep the temporary clusters for debugging.',
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
    cwd: Path = REPO_ROOT,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        input=input_sql,
        text=True,
        cwd=cwd,
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


def psql(args: argparse.Namespace, pgdata: Path, port: int, sql: str) -> str:
    result = run(psql_cmd(args, pgdata, port), input_sql=sql)
    return result.stdout.strip()


def init_primary(args: argparse.Namespace, pgdata: Path, port: int) -> None:
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
        conf.write('wal_level = replica\n')
        conf.write('max_wal_senders = 5\n')
        conf.write('max_replication_slots = 5\n')
        conf.write('hot_standby = on\n')
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


def setup_primary(args: argparse.Namespace, pgdata: Path, port: int) -> None:
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
        SELECT gs, ARRAY['standby', 'preload', gs::text]
        FROM generate_series(1, 2000) gs;
        CREATE INDEX docs_bm25_idx
            ON docs USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1,
                auto_preload = 1
            );
        ''',
    )


def start_standby(
    args: argparse.Namespace,
    primary_pgdata: Path,
    primary_port: int,
    standby_pgdata: Path,
    standby_port: int,
) -> None:
    bindir = Path(args.bindir)

    run([
        str(bindir / 'pg_basebackup'),
        '-D',
        str(standby_pgdata),
        '-h',
        str(primary_pgdata),
        '-p',
        str(primary_port),
        '-U',
        os.environ.get('USER', 'postgres'),
        '-X',
        'stream',
        '-R',
    ])
    with (standby_pgdata / 'postgresql.conf').open('a', encoding='utf-8') as conf:
        conf.write(f"\nport = {standby_port}\n")
        conf.write(f"unix_socket_directories = '{standby_pgdata}'\n")
        conf.write('hot_standby = on\n')
        conf.write('psql_bm25s.maintenance_worker_limit = 1\n')
        conf.write("psql_bm25s.maintenance_timer_interval_ms = '1000ms'\n")

    run([
        str(bindir / 'pg_ctl'),
        '-D',
        str(standby_pgdata),
        '-l',
        str(standby_pgdata / 'postgres.log'),
        '-w',
        'start',
    ])


def state_value(cache_state: str, key: str) -> str:
    match = re.search(rf'{re.escape(key)}=([^,)]*)', cache_state)
    if match is None:
        raise AssertionError(f'missing {key} in state: {cache_state}')
    return match.group(1)


def cache_state(args: argparse.Namespace, pgdata: Path, port: int) -> str:
    return psql(
        args,
        pgdata,
        port,
        "SELECT public.psql_bm25s_generation_cache_state('docs_bm25_idx');",
    )


def wait_for_standby_resident(
    args: argparse.Namespace,
    standby_pgdata: Path,
    standby_port: int,
    expected_epoch: int,
) -> str:
    deadline = time.monotonic() + args.timeout
    last_state = ''

    while time.monotonic() < deadline:
        try:
            last_state = cache_state(args, standby_pgdata, standby_port)
            epoch = int(state_value(last_state, 'cache_epoch'))
            resident = state_value(last_state, 'shared_preload_resident')
            obsolete = state_value(last_state, 'shared_preload_obsolete_entries')
            if epoch >= expected_epoch and resident == 'true' and obsolete == '0':
                return last_state
        except subprocess.CalledProcessError:
            pass
        time.sleep(0.5)

    raise AssertionError(
        'standby did not auto-preload the expected generation: '
        f'expected_epoch={expected_epoch} last_state={last_state}'
    )


def primary_epoch(args: argparse.Namespace, pgdata: Path, port: int) -> int:
    return int(state_value(cache_state(args, pgdata, port), 'cache_epoch'))


def rebuild_on_primary(args: argparse.Namespace, pgdata: Path, port: int) -> int:
    psql(
        args,
        pgdata,
        port,
        '''
        INSERT INTO docs VALUES (2001, ARRAY['standby','preload','updated']);
        SELECT public.psql_bm25s_index_maintain('docs_bm25_idx');
        CHECKPOINT;
        ''',
    )
    return primary_epoch(args, pgdata, port)


def main() -> None:
    args = parse_args()
    workdir = Path(tempfile.mkdtemp(
        prefix='psql_bm25s_shared_standby_',
        dir='/tmp',
    ))
    primary_pgdata = workdir / 'primary'
    standby_pgdata = workdir / 'standby'
    primary_port = free_port()
    standby_port = free_port()

    try:
        init_primary(args, primary_pgdata, primary_port)
        setup_primary(args, primary_pgdata, primary_port)
        start_standby(
            args,
            primary_pgdata,
            primary_port,
            standby_pgdata,
            standby_port,
        )
        initial_epoch = primary_epoch(args, primary_pgdata, primary_port)
        initial_state = wait_for_standby_resident(
            args,
            standby_pgdata,
            standby_port,
            initial_epoch,
        )
        updated_epoch = rebuild_on_primary(args, primary_pgdata, primary_port)
        updated_state = wait_for_standby_resident(
            args,
            standby_pgdata,
            standby_port,
            updated_epoch,
        )
        print('initial_standby_state=' + initial_state)
        print('updated_standby_state=' + updated_state)
        print('shared preload standby auto_preload smoke passed')
    finally:
        stop_cluster(args, standby_pgdata)
        stop_cluster(args, primary_pgdata)
        if not args.keep:
            shutil.rmtree(workdir, ignore_errors=True)
        else:
            print(f'kept temporary clusters at {workdir}')


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        sys.exit(1)
