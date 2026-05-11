from __future__ import annotations

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCHEMA = 'bm25_eventual_tail_smoke'
STALE_RESULT_RE = re.compile(
    r'stale=true, pending_writes=(\d+), pending_deletes=(\d+)'
)
PENDING_RESULT_RE = re.compile(
    r'pending_writes=(\d+), pending_deletes=(\d+)'
)
TAIL_RESULT_RE = re.compile(r'tail_carried=true, tail_records=(\d+)')
STATE_RE = re.compile(
    r'pending_writes=(\d+), pending_deletes=(\d+), '
    r'delta_records=(\d+).*stale=([tf])'
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a query-first eventual delta-tail smoke test.'
    )
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--dbname', default='contrib_regression')
    parser.add_argument('--row-count', type=int, default=250000)
    parser.add_argument('--insert-attempts', type=int, default=200)
    parser.add_argument('--insert-sleep-ms', type=int, default=20)
    parser.add_argument('--start-delay-ms', type=int, default=50)
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
) -> str:
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

        CREATE TABLE {SCHEMA}.worker_warmup (
            id int primary key,
            tokens text[] not null
        );

        INSERT INTO {SCHEMA}.worker_warmup VALUES
            (1, ARRAY['warmup','base']);

        CREATE INDEX worker_warmup_bm25_idx
            ON {SCHEMA}.worker_warmup USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1000000
            );

        INSERT INTO {SCHEMA}.worker_warmup VALUES
            (2, ARRAY['warmup','delta']);

        SELECT pg_sleep(1);
        DROP TABLE {SCHEMA}.worker_warmup;

        CREATE TABLE {SCHEMA}.docs (
            id int primary key,
            tokens text[] not null,
            marker int not null default 0
        );

        INSERT INTO {SCHEMA}.docs (id, tokens)
        SELECT
            g,
            ARRAY[
                'tail',
                'base',
                'term' || (g % 101)::text,
                'topic' || (g % 997)::text,
                'chunk' || (g % 17)::text,
                'steady'
            ]
        FROM generate_series(1, {args.row_count}) g;

        CREATE INDEX docs_marker_idx ON {SCHEMA}.docs (marker);

        CREATE INDEX docs_bm25_idx
            ON {SCHEMA}.docs USING psql_bm25s (tokens)
            WITH (
                consistency = 'eventual',
                auto_rebuild_threshold = 1000000
            );

        INSERT INTO {SCHEMA}.docs (id, tokens) VALUES
            ({args.row_count + 1}, ARRAY['tail','initial','delta']);

        SELECT public.psql_bm25s_index_try_maintain(
            '{SCHEMA}.docs_bm25_idx'::regclass
        );

        INSERT INTO {SCHEMA}.docs (id, tokens) VALUES
            ({args.row_count + 2}, ARRAY['tail','trigger','delta']);
        ''',
    )


def drop_schema(args: argparse.Namespace) -> None:
    run_psql(args, f'DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;')


def start_maintenance(args: argparse.Namespace) -> subprocess.Popen[str]:
    sql = (
        'SELECT public.psql_bm25s_index_try_maintain('
        f"'{SCHEMA}.docs_bm25_idx'::regclass);"
    )
    return subprocess.Popen(
        psql_cmd(args, raw=True) + ['-c', sql],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=REPO_ROOT,
    )


def insert_while_running(
    args: argparse.Namespace,
    maintenance: subprocess.Popen[str],
) -> int:
    inserted = 0
    sleep_s = args.insert_sleep_ms / 1000.0

    for i in range(1, args.insert_attempts + 1):
        if maintenance.poll() is not None:
            break
        doc_id = args.row_count + 1000 + i
        marker_id = ((i - 1) % args.row_count) + 1
        run_psql(
            args,
            f'''
            INSERT INTO {SCHEMA}.docs (id, tokens) VALUES
                ({doc_id}, ARRAY['tail','carried','delta','{i}']);
            UPDATE {SCHEMA}.docs
            SET marker = marker + 1
            WHERE id = {marker_id};
            ''',
            raw=True,
        )
        inserted = i
        if sleep_s > 0:
            time.sleep(sleep_s)

    return inserted


def wait_maintenance(
    maintenance: subprocess.Popen[str],
    timeout: int,
) -> str:
    stdout, stderr = maintenance.communicate(timeout=timeout)
    if maintenance.returncode != 0:
        sys.stderr.write(stdout)
        sys.stderr.write(stderr)
        raise RuntimeError(
            f'maintenance failed with exit code {maintenance.returncode}'
        )
    return stdout.strip()


def stale_pending_count(result: str) -> int:
    match = STALE_RESULT_RE.search(result)
    if match is None:
        return 0
    pending_writes, pending_deletes = map(int, match.groups())
    return pending_writes + pending_deletes


def pending_count(result: str) -> int:
    match = PENDING_RESULT_RE.search(result)
    if match is None:
        return 0
    pending_writes, pending_deletes = map(int, match.groups())
    return pending_writes + pending_deletes


def carried_tail_count(result: str) -> int:
    match = TAIL_RESULT_RE.search(result)
    if match is None:
        return 0
    return int(match.group(1))


def maintain_until_clean(args: argparse.Namespace) -> str:
    deadline = time.monotonic() + args.maintenance_timeout
    last_result = ''

    while time.monotonic() < deadline:
        last_result = run_psql(
            args,
            (
                'SELECT public.psql_bm25s_index_try_maintain('
                f"'{SCHEMA}.docs_bm25_idx'::regclass);"
            ),
            raw=True,
        )
        try:
            final_state = maintenance_state(args)
            assert_clean_state(final_state)
            return last_result
        except AssertionError:
            time.sleep(0.5)

    raise AssertionError(
        'maintenance did not converge before timeout: '
        f'{last_result}'
    )


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


def state_pending_count(state: str) -> int:
    match = STATE_RE.search(state)
    if match is None:
        raise AssertionError(f'could not parse maintenance state: {state}')
    pending_writes, pending_deletes, delta_records, _stale = match.groups()
    return int(pending_writes) + int(pending_deletes) + int(delta_records)


def assert_clean_state(state: str) -> None:
    match = STATE_RE.search(state)
    if match is None:
        raise AssertionError(f'could not parse maintenance state: {state}')
    pending_writes, pending_deletes, delta_records, stale = match.groups()
    if (
        int(pending_writes) != 0 or
        int(pending_deletes) != 0 or
        int(delta_records) != 0 or
        stale != 'f'
    ):
        raise AssertionError(f'maintenance state is not clean: {state}')


def carried_hit_count(args: argparse.Namespace) -> int:
    value = run_psql(
        args,
        f'''
        SELECT count(*)
        FROM public.psql_bm25s_query_tokens(
            '{SCHEMA}.docs_bm25_idx'::regclass,
            ARRAY['carried'],
            20,
            NULL
        ) h
        JOIN {SCHEMA}.docs d ON d.ctid = h.ctid
        WHERE d.id > {args.row_count + 1000};
        ''',
        raw=True,
    )
    return int(value)


def main() -> None:
    args = parse_args()
    setup_schema(args)
    maintenance: subprocess.Popen[str] | None = None

    try:
        maintenance = start_maintenance(args)
        if args.start_delay_ms > 0:
            time.sleep(args.start_delay_ms / 1000.0)
        inserted = insert_while_running(args, maintenance)
        first_result = wait_maintenance(
            maintenance,
            args.maintenance_timeout,
        )
        maintenance = None

        stale_pending = stale_pending_count(first_result)
        pending = pending_count(first_result)
        carried_tail = carried_tail_count(first_result)
        post_first_state = maintenance_state(args)
        post_first_pending = state_pending_count(post_first_state)
        if (
            inserted > 0 and
            pending <= 0 and
            stale_pending <= 0 and
            carried_tail <= 0 and
            post_first_pending <= 0 and
            'lock_busy' not in first_result
        ):
            raise AssertionError(
                'staged publish did not preserve concurrent delta tail; '
                f'inserted_during_maintenance={inserted}, '
                f'result={first_result}, state={post_first_state}'
            )

        second_result = maintain_until_clean(args)
        final_state = maintenance_state(args)
        assert_clean_state(final_state)

        hits = carried_hit_count(args)
        if hits <= 0:
            raise AssertionError('carried documents were not searchable')

        print(
            'query-first eventual tail/convergence smoke passed '
            f'(inserted={inserted}, pending={pending}, '
            f'stale_pending={stale_pending}, '
            f'post_first_pending={post_first_pending}, '
            f'carried_tail={carried_tail}, second="{second_result}")'
        )
    finally:
        if maintenance is not None and maintenance.poll() is None:
            maintenance.kill()
            maintenance.wait()
        drop_schema(args)


if __name__ == '__main__':
    main()
