from __future__ import annotations

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCHEMA = 'bm25_eventual_background_smoke'
STATE_RE = re.compile(
    r'pending_writes=(\d+), pending_deletes=(\d+), '
    r'delta_records=(\d+).*stale=([tf])'
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a query-triggered background maintenance smoke test.'
    )
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--dbname', default='contrib_regression')
    parser.add_argument('--row-count', type=int, default=20000)
    parser.add_argument('--timeout', type=int, default=180)
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


def run_psql(args: argparse.Namespace, sql: str, raw: bool = False) -> str:
    result = subprocess.run(
        psql_cmd(args, raw=raw),
        input=sql,
        text=True,
        cwd=REPO_ROOT,
        check=True,
        capture_output=True,
    )
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
                'background',
                'base',
                'term' || (g % 101)::text,
                'topic' || (g % 997)::text,
                'chunk' || (g % 17)::text
            ]
        FROM generate_series(1, {args.row_count}) g;

        CREATE INDEX docs_bm25_idx
            ON {SCHEMA}.docs USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1
            );

        INSERT INTO {SCHEMA}.docs VALUES
            ({args.row_count + 1}, ARRAY['background','wake','delta']);
        ''',
    )


def drop_schema(args: argparse.Namespace) -> None:
    run_psql(args, f'DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;')


def maintenance_state(args: argparse.Namespace) -> str:
    return run_psql(
        args,
        (
            "SELECT format('psql_bm25s_maintenance_state(rebuilds=%s, "
            "pending_writes=%s, pending_deletes=%s, delta_records=%s, "
            "delta_bytes=%s, stale=%s)', rebuilds, pending_writes, "
            "pending_deletes, delta_records, delta_bytes, stale) "
            'FROM public.psql_bm25s_index_details('
            f"'{SCHEMA}.docs_bm25_idx'::regclass);"
        ),
        raw=True,
    )


def parse_state(state: str) -> tuple[int, int, int, str]:
    match = STATE_RE.search(state)
    if match is None:
        raise AssertionError(f'could not parse maintenance state: {state}')
    pending_writes, pending_deletes, delta_records, stale = match.groups()
    return int(pending_writes), int(pending_deletes), int(delta_records), stale


def trigger_query(args: argparse.Namespace) -> None:
    run_psql(
        args,
        f'''
        SELECT count(*)
        FROM public.psql_bm25s_query_tokens(
            '{SCHEMA}.docs_bm25_idx'::regclass,
            ARRAY['wake'],
            10,
            NULL
        ) h
        JOIN {SCHEMA}.docs d ON d.ctid = h.ctid
        WHERE d.id = {args.row_count + 1};
        ''',
        raw=True,
    )


def wait_until_clean(args: argparse.Namespace) -> str:
    deadline = time.monotonic() + args.timeout
    last_state = maintenance_state(args)

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
            return last_state
        time.sleep(0.5)
        last_state = maintenance_state(args)

    raise AssertionError(
        'background maintenance did not converge before timeout: '
        f'{last_state}'
    )


def main() -> None:
    args = parse_args()
    setup_schema(args)

    try:
        initial_state = maintenance_state(args)
        pending_writes, _, delta_records, stale = parse_state(initial_state)
        if pending_writes > 0 or delta_records > 0 or stale == 't':
            trigger_query(args)
        final_state = wait_until_clean(args)
        print('initial_state=' + initial_state)
        print('final_state=' + final_state)
    finally:
        drop_schema(args)


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        sys.exit(1)
