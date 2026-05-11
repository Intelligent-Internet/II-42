#!/usr/bin/env python3
from __future__ import annotations

import argparse
import concurrent.futures
import json
import statistics
import sys
import time
from pathlib import Path

import psycopg
from psycopg import conninfo

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from benchmark_filtered_ordered_must_query_only import (  # noqa: E402
    DEFAULT_DATASETS_DIR,
    PG_DSN,
    cleanup_state,
    prepare_state,
    read_state,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Measure fresh-backend shared generation cache behavior.'
    )
    parser.add_argument(
        '--repo-root',
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument(
        '--datasets-dir',
        type=Path,
        default=DEFAULT_DATASETS_DIR,
    )
    parser.add_argument('--dataset', default='nq')
    parser.add_argument('--top-k', type=int, default=10)
    parser.add_argument('--max-cases', type=int, default=250)
    parser.add_argument('--connections', type=int, default=8)
    parser.add_argument('--parallelism', type=int, default=1)
    parser.add_argument('--cases-per-connection', type=int, default=1)
    parser.add_argument(
        '--state-file',
        type=Path,
        default=Path('/tmp/psql_bm25s_generation_churn_state.json'),
    )
    parser.add_argument(
        '--keep-db',
        action='store_true',
        help='Keep the prepared benchmark database for debugging.',
    )
    return parser.parse_args()


def scalar(conn: psycopg.Connection, query: str) -> str:
    with conn.cursor() as cur:
        cur.execute(query)
        row = cur.fetchone()
    return '' if row is None else str(row[0])


def clear_generation_cache(conn: psycopg.Connection) -> int | None:
    with conn.cursor() as cur:
        try:
            cur.execute('SELECT public.psql_bm25s_generation_cache_clear()')
        except psycopg.errors.UndefinedFunction:
            conn.rollback()
            return None
        row = cur.fetchone()
    return int(row[0]) if row is not None else None


def generation_state(
    conn: psycopg.Connection,
    index_name: str,
) -> str | None:
    with conn.cursor() as cur:
        try:
            cur.execute(
                'SELECT public.psql_bm25s_generation_cache_state(%s::regclass)',
                (index_name,),
            )
        except psycopg.errors.UndefinedFunction:
            conn.rollback()
            return None
        row = cur.fetchone()
    return None if row is None else str(row[0])


def disable_autovacuum_for_churn(db_dsn: str, db_name: str) -> None:
    with psycopg.connect(db_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute(
                'ALTER TABLE bench.docs_tokens '
                'SET (autovacuum_enabled = false)'
            )

    with psycopg.connect(PG_DSN, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                SELECT pg_terminate_backend(pid)
                FROM pg_stat_activity
                WHERE datname = %s
                  AND query LIKE 'autovacuum:%%bench.docs_tokens%%'
                """,
                (db_name,),
            )


def run_one_backend(
    dsn: str,
    cases: list[dict[str, object]],
    top_k: int,
    application_name: str,
) -> float:
    sql_text = """
        SELECT count(*), coalesce(min(id), 0)
        FROM (
            SELECT id
            FROM bench.docs_tokens
            WHERE tokens @@ %s
            ORDER BY tokens <=> %s::text[] ASC
            LIMIT %s
        ) hits
    """
    t0 = time.perf_counter()
    with psycopg.connect(
        conninfo.make_conninfo(dsn, application_name=application_name),
        autocommit=True,
    ) as conn:
        with conn.cursor() as cur:
            cur.execute('SET enable_seqscan = off')
            cur.execute('SET enable_bitmapscan = off')
            for case in cases:
                cur.execute(
                    sql_text,
                    (
                        case['filter_query'],
                        case['order_query_tokens'],
                        top_k,
                    ),
                )
                cur.fetchone()
            cur.execute('RESET enable_bitmapscan')
            cur.execute('RESET enable_seqscan')
    return (time.perf_counter() - t0) * 1000.0


def summarize(values: list[float]) -> dict[str, float]:
    return {
        'count': len(values),
        'min_ms': min(values),
        'max_ms': max(values),
        'mean_ms': statistics.mean(values),
        'median_ms': statistics.median(values),
        'p95_ms': (
            statistics.quantiles(values, n=20)[18]
            if len(values) >= 20 else max(values)
        ),
    }


def main() -> int:
    args = parse_args()
    repo_root = args.repo_root.resolve()
    state_file = args.state_file.resolve()

    prepare_state(
        repo_root=repo_root,
        dataset=args.dataset,
        datasets_dir=args.datasets_dir.resolve(),
        top_k=args.top_k,
        max_cases=args.max_cases,
        state_file=state_file,
    )
    state = read_state(state_file)
    db_name = str(state['db_name'])
    db_dsn = conninfo.make_conninfo(PG_DSN, dbname=db_name)
    disable_autovacuum_for_churn(db_dsn, db_name)
    cases = list(state['cases'])[:args.cases_per_connection]
    index_name = 'bench.docs_tokens_bm25_idx'
    latencies_ms: list[float] = []
    cleared: int | None = None
    before_state: str | None = None
    after_state: str | None = None

    try:
        with psycopg.connect(db_dsn, autocommit=True) as conn:
            cleared = clear_generation_cache(conn)
            before_state = generation_state(conn, index_name)

        if args.parallelism <= 1:
            for i in range(args.connections):
                latency_ms = run_one_backend(
                    dsn=db_dsn,
                    cases=cases,
                    top_k=int(state['top_k']),
                    application_name=f'psql_bm25s_generation_churn_{i}',
                )
                latencies_ms.append(latency_ms)
        else:
            with concurrent.futures.ThreadPoolExecutor(
                max_workers=args.parallelism
            ) as executor:
                futures = [
                    executor.submit(
                        run_one_backend,
                        db_dsn,
                        cases,
                        int(state['top_k']),
                        f'psql_bm25s_generation_churn_{i}',
                    )
                    for i in range(args.connections)
                ]
                for future in concurrent.futures.as_completed(futures):
                    latencies_ms.append(future.result())

        with psycopg.connect(db_dsn, autocommit=True) as conn:
            after_state = generation_state(conn, index_name)

        print(json.dumps({
            'dataset': args.dataset,
            'documents': state['stats']['documents'],
            'build_bytes': state['build_bytes'],
            'build_ms': state['build_ms'],
            'connections': args.connections,
            'parallelism': args.parallelism,
            'cases_per_connection': args.cases_per_connection,
            'cache_clear_result': cleared,
            'state_before': before_state,
            'state_after': after_state,
            'first_query_ms': latencies_ms,
            'summary': summarize(latencies_ms),
        }, indent=2, sort_keys=True))
    finally:
        if not args.keep_db:
            cleanup_state(repo_root, state_file)
        if state_file.exists() and not args.keep_db:
            state_file.unlink()

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
