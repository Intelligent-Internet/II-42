from __future__ import annotations

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCHEMA = 'bm25_eventual_cancel_smoke'
STATE_RE = re.compile(
    r'pending_writes=(\d+), pending_deletes=(\d+), '
    r'delta_records=(\d+).*stale=([tf])'
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a staged-maintenance cancellation smoke test.'
    )
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--dbname', default='contrib_regression')
    parser.add_argument('--row-count', type=int, default=250000)
    parser.add_argument('--terminate-after-ms', type=int, default=0)
    parser.add_argument('--maintenance-timeout', type=int, default=120)
    return parser.parse_args()


def psql_cmd(args: argparse.Namespace, raw: bool = False) -> list[str]:
    cmd = [
        args.psql,
        '-X',
        '-d',
        args.dbname,
        '-v',
        'ON_ERROR_STOP=1',
    ]
    if raw:
        cmd.extend(['-Atq'])
    return cmd


def run_psql(
    args: argparse.Namespace,
    sql: str,
    raw: bool = False,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        psql_cmd(args, raw=raw),
        input=sql,
        text=True,
        cwd=REPO_ROOT,
        check=check,
        capture_output=True,
    )


def scalar_sql(args: argparse.Namespace, sql: str) -> str:
    result = run_psql(args, sql, raw=True)
    return result.stdout.strip()


def setup_schema(args: argparse.Namespace) -> None:
    run_psql(
        args,
        f'''
        DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;
        CREATE SCHEMA {SCHEMA};
        CREATE EXTENSION IF NOT EXISTS psql_bm25s;

        CREATE TABLE {SCHEMA}.docs (
            id int primary key,
            tokens text[] not null
        );

        INSERT INTO {SCHEMA}.docs
        SELECT
            g,
            ARRAY[
                'cancel',
                'base',
                'term' || (g % 101)::text,
                'topic' || (g % 997)::text,
                'chunk' || (g % 17)::text,
                'steady'
            ]
        FROM generate_series(1, {args.row_count}) g;

        CREATE INDEX docs_bm25_idx
            ON {SCHEMA}.docs USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1000000
            );

        INSERT INTO {SCHEMA}.docs VALUES
            ({args.row_count + 1}, ARRAY['cancel','pending','delta']);
        ''',
    )


def drop_schema(args: argparse.Namespace) -> None:
    run_psql(args, f'DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;', check=False)


def start_maintenance(args: argparse.Namespace) -> tuple[subprocess.Popen[str], int]:
    proc = subprocess.Popen(
        psql_cmd(args, raw=True),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=REPO_ROOT,
    )
    assert proc.stdin is not None
    assert proc.stdout is not None
    proc.stdin.write('SELECT pg_backend_pid();\n')
    proc.stdin.flush()
    pid_line = proc.stdout.readline().strip()
    if not pid_line:
        stderr = proc.stderr.read() if proc.stderr is not None else ''
        raise RuntimeError(f'could not read maintenance backend pid: {stderr}')
    proc.stdin.write(
        'SELECT public.psql_bm25s_index_try_maintain('
        f"'{SCHEMA}.docs_bm25_idx'::regclass);\n"
    )
    proc.stdin.flush()
    return proc, int(pid_line)


def terminate_backend(args: argparse.Namespace, pid: int) -> bool:
    value = scalar_sql(args, f'SELECT pg_terminate_backend({pid});')
    return value == 't'


def maintenance_state(args: argparse.Namespace) -> str:
    return scalar_sql(
        args,
        (
            "SELECT format('psql_bm25s_maintenance_state(rebuilds=%s, "
            "pending_writes=%s, pending_deletes=%s, delta_records=%s, "
            "delta_bytes=%s, stale=%s)', rebuilds, pending_writes, "
            "pending_deletes, delta_records, delta_bytes, stale) "
            'FROM public.psql_bm25s_index_details('
            f"'{SCHEMA}.docs_bm25_idx'::regclass);"
        ),
    )


def parse_state(state: str) -> tuple[int, int, int, str]:
    match = STATE_RE.search(state)
    if match is None:
        raise AssertionError(f'could not parse maintenance state: {state}')
    pending_writes, pending_deletes, delta_records, stale = match.groups()
    return int(pending_writes), int(pending_deletes), int(delta_records), stale


def assert_retryable_debt(state: str) -> None:
    pending_writes, pending_deletes, delta_records, _ = parse_state(state)
    if pending_writes + pending_deletes <= 0 and delta_records <= 0:
        raise AssertionError(
            'canceled maintenance did not leave retryable debt: '
            f'{state}'
        )


def assert_clean_state(state: str) -> None:
    pending_writes, pending_deletes, delta_records, stale = parse_state(state)
    if (
        pending_writes != 0 or
        pending_deletes != 0 or
        delta_records != 0 or
        stale != 'f'
    ):
        raise AssertionError(f'maintenance state is not clean: {state}')


def readable_hit_count(args: argparse.Namespace) -> int:
    value = scalar_sql(
        args,
        f'''
        SELECT count(*)
        FROM public.psql_bm25s_query_tokens(
            '{SCHEMA}.docs_bm25_idx'::regclass,
            ARRAY['base'],
            10,
            NULL
        );
        ''',
    )
    return int(value)


def maintain_until_clean(args: argparse.Namespace) -> str:
    deadline = time.monotonic() + args.maintenance_timeout
    last_result = ''

    while time.monotonic() < deadline:
        last_result = scalar_sql(
            args,
            (
                'SELECT public.psql_bm25s_index_try_maintain('
                f"'{SCHEMA}.docs_bm25_idx'::regclass);"
            ),
        )
        try:
            assert_clean_state(maintenance_state(args))
            return last_result
        except AssertionError:
            time.sleep(0.5)

    raise AssertionError(
        'maintenance did not converge before timeout: '
        f'{last_result}'
    )


def main() -> None:
    args = parse_args()
    setup_schema(args)
    proc: subprocess.Popen[str] | None = None

    try:
        proc, pid = start_maintenance(args)
        time.sleep(args.terminate_after_ms / 1000.0)
        terminated = terminate_backend(args, pid)
        if proc.stdin is not None:
            proc.stdin.close()
            proc.stdin = None
        stdout, stderr = proc.communicate(timeout=args.maintenance_timeout)

        if not terminated:
            raise AssertionError(f'pg_terminate_backend({pid}) returned false')
        if proc.returncode == 0:
            raise AssertionError(
                'maintenance completed before cancellation; '
                f'stdout={stdout}'
            )

        state_after_cancel = maintenance_state(args)
        assert_retryable_debt(state_after_cancel)

        hits = readable_hit_count(args)
        if hits <= 0:
            raise AssertionError('old base index was not readable')

        result = maintain_until_clean(args)
        final_state = maintenance_state(args)
        assert_clean_state(final_state)

        print(
            'query-first eventual cancellation smoke passed '
            f'(pid={pid}, after_cancel="{state_after_cancel}", '
            f'retry="{result}")'
        )
    finally:
        if proc is not None and proc.poll() is None:
            proc.kill()
            proc.wait()
        drop_schema(args)


if __name__ == '__main__':
    main()
