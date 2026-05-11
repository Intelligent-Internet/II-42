from __future__ import annotations

import argparse
import json
import os
import re
import statistics
import threading
import time
from dataclasses import asdict
from dataclasses import dataclass

import psycopg


DEFAULT_DB_NAME = 'psql_bm25s_eventual_backfill_bench'
DEFAULT_ADMIN_DSN = os.environ.get(
    'PSQL_BM25S_BENCH_DSN',
    'dbname=postgres',
)
STATE_RE = re.compile(
    r'pending_writes=(\d+), pending_deletes=(\d+), '
    r'delta_records=(\d+)'
)


@dataclass
class QuerySummary:
    count: int
    timeouts: int
    errors: int
    avg_ms: float | None
    p50_ms: float | None
    p95_ms: float | None
    p99_ms: float | None
    qps: float


@dataclass
class ModeSummary:
    mode: str
    reloptions: str
    writer_ms: float
    maintenance_ms: float | None
    maintenance_converged: bool
    before_state: str
    after_write_state: str
    final_state: str
    queries: QuerySummary


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            'Benchmark query-first eventual maintenance during a '
            'non-indexed-column backfill.'
        )
    )
    parser.add_argument('--admin-dsn', default=DEFAULT_ADMIN_DSN)
    parser.add_argument('--db-name', default=DEFAULT_DB_NAME)
    parser.add_argument('--doc-count', type=int, default=50000)
    parser.add_argument('--update-count', type=int, default=5000)
    parser.add_argument('--query-k', type=int, default=20)
    parser.add_argument('--statement-timeout-ms', type=int, default=2000)
    parser.add_argument('--query-sleep-ms', type=int, default=5)
    parser.add_argument('--convergence-timeout-ms', type=int, default=30000)
    parser.add_argument('--poll-interval-ms', type=int, default=250)
    parser.add_argument('--output')
    return parser.parse_args()


def database_dsn(admin_dsn: str, db_name: str) -> str:
    return psycopg.conninfo.make_conninfo(admin_dsn, dbname=db_name)


def reset_database(args: argparse.Namespace) -> None:
    with psycopg.connect(args.admin_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute(f'DROP DATABASE IF EXISTS "{args.db_name}"')
            cur.execute(f'CREATE DATABASE "{args.db_name}"')


def setup_mode_table(
    cur: psycopg.Cursor,
    table_name: str,
    reloptions: str,
    doc_count: int,
) -> None:
    cur.execute(f'DROP TABLE IF EXISTS {table_name} CASCADE')
    cur.execute(
        f'''
        CREATE TABLE {table_name} (
            id int primary key,
            marker int not null,
            title_tokens text[] not null,
            abstract text
        )
        '''
    )
    cur.execute(
        f'''
        INSERT INTO {table_name}
        SELECT
            g,
            g,
            CASE
                WHEN g % 7 = 0 THEN ARRAY['cancer','therapy','study']
                WHEN g % 11 = 0 THEN ARRAY['genome','trial','patient']
                ELSE ARRAY['bird','ecology','field']
            END,
            NULL::text
        FROM generate_series(1, {doc_count}) g
        '''
    )
    cur.execute(f'CREATE INDEX {table_name}_marker_idx ON {table_name}(marker)')
    cur.execute(
        f'''
        CREATE INDEX {table_name}_bm25_idx
            ON {table_name} USING psql_bm25s (title_tokens)
            WITH ({reloptions})
        '''
    )


def setup_database(args: argparse.Namespace) -> None:
    dsn = database_dsn(args.admin_dsn, args.db_name)
    eager_options = (
        "method = 'lucene', idf_method = 'lucene', "
        'auto_rebuild_threshold = 0'
    )
    eventual_options = (
        "method = 'lucene', idf_method = 'lucene', "
        "consistency = 'eventual', "
        'auto_rebuild_threshold = 1000000, '
        'query_overlay_max_records = 50000, '
        'query_overlay_max_bytes = 16777216'
    )

    with psycopg.connect(dsn) as conn:
        with conn.cursor() as cur:
            cur.execute('CREATE EXTENSION psql_bm25s')
            setup_mode_table(
                cur,
                'docs_eager',
                eager_options,
                args.doc_count,
            )
            setup_mode_table(
                cur,
                'docs_eventual',
                eventual_options,
                args.doc_count,
            )
        conn.commit()


def maintenance_state(cur: psycopg.Cursor, index_name: str) -> str:
    cur.execute(
        '''
        SELECT format(
            'psql_bm25s_maintenance_state(rebuilds=%%s, '
            'pending_writes=%%s, pending_deletes=%%s, delta_records=%%s, '
            'delta_bytes=%%s, stale=%%s)',
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
    return str(cur.fetchone()[0])


def parse_state(state: str) -> tuple[int, int, int]:
    match = STATE_RE.search(state)
    if match is None:
        raise RuntimeError(f'could not parse maintenance state: {state}')
    return tuple(map(int, match.groups()))


def state_is_clean(state: str) -> bool:
    pending_writes, pending_deletes, delta_records = parse_state(state)
    return (
        pending_writes == 0
        and pending_deletes == 0
        and delta_records == 0
        and 'stale=false' in state
    )


def wait_for_background_convergence(
    cur: psycopg.Cursor,
    index_name: str,
    timeout_ms: int,
    poll_interval_ms: int,
) -> tuple[bool, float, str]:
    started = time.perf_counter()
    deadline = started + (timeout_ms / 1000.0)
    last_state = maintenance_state(cur, index_name)

    while time.perf_counter() < deadline:
        if state_is_clean(last_state):
            elapsed_ms = (time.perf_counter() - started) * 1000.0
            return True, elapsed_ms, last_state
        time.sleep(poll_interval_ms / 1000.0)
        last_state = maintenance_state(cur, index_name)

    elapsed_ms = (time.perf_counter() - started) * 1000.0
    return False, elapsed_ms, last_state


def query_worker(
    dsn: str,
    table_name: str,
    query_k: int,
    statement_timeout_ms: int,
    query_sleep_ms: int,
    stop_event: threading.Event,
    latencies_ms: list[float],
    counters: dict[str, int],
    fatal_errors: list[str],
) -> None:
    try:
        with psycopg.connect(dsn) as conn:
            with conn.cursor() as cur:
                cur.execute(
                    f"SET statement_timeout = '{int(statement_timeout_ms)}ms'"
                )
                while not stop_event.is_set():
                    start = time.perf_counter()
                    try:
                        cur.execute(
                            '''
                            SELECT count(*)
                            FROM public.psql_bm25s_query_tokens(
                                %s::regclass,
                                ARRAY['cancer'],
                                %s,
                                NULL
                            )
                            ''',
                            (f'{table_name}_bm25_idx', query_k),
                        )
                        cur.fetchone()
                        latencies_ms.append(
                            (time.perf_counter() - start) * 1000.0
                        )
                    except psycopg.errors.QueryCanceled:
                        conn.rollback()
                        counters['timeouts'] += 1
                    except Exception:
                        conn.rollback()
                        counters['errors'] += 1
                    if query_sleep_ms > 0:
                        time.sleep(query_sleep_ms / 1000.0)
    except Exception as exc:
        fatal_errors.append(repr(exc))
        stop_event.set()


def summarize_queries(
    latencies_ms: list[float],
    counters: dict[str, int],
    elapsed_s: float,
) -> QuerySummary:
    if latencies_ms:
        sorted_values = sorted(latencies_ms)
        p95_index = max(int(len(sorted_values) * 0.95) - 1, 0)
        p99_index = max(int(len(sorted_values) * 0.99) - 1, 0)
        avg_ms = statistics.mean(latencies_ms)
        p50_ms = statistics.median(latencies_ms)
        p95_ms = sorted_values[p95_index]
        p99_ms = sorted_values[p99_index]
    else:
        avg_ms = None
        p50_ms = None
        p95_ms = None
        p99_ms = None

    return QuerySummary(
        count=len(latencies_ms),
        timeouts=counters['timeouts'],
        errors=counters['errors'],
        avg_ms=avg_ms,
        p50_ms=p50_ms,
        p95_ms=p95_ms,
        p99_ms=p99_ms,
        qps=(len(latencies_ms) / elapsed_s) if elapsed_s > 0 else 0.0,
    )


def run_mode(
    args: argparse.Namespace,
    table_name: str,
    mode_name: str,
    reloptions: str,
) -> ModeSummary:
    dsn = database_dsn(args.admin_dsn, args.db_name)
    stop_event = threading.Event()
    latencies_ms: list[float] = []
    counters = {'timeouts': 0, 'errors': 0}
    fatal_errors: list[str] = []
    worker = threading.Thread(
        target=query_worker,
        args=(
            dsn,
            table_name,
            args.query_k,
            args.statement_timeout_ms,
            args.query_sleep_ms,
            stop_event,
            latencies_ms,
            counters,
            fatal_errors,
        ),
    )

    with psycopg.connect(dsn) as conn:
        with conn.cursor() as cur:
            index_name = f'{table_name}_bm25_idx'
            before_state = maintenance_state(cur, index_name)
            worker_start = time.perf_counter()
            worker.start()
            time.sleep(0.25)

            writer_start = time.perf_counter()
            cur.execute(
                f'''
                UPDATE {table_name}
                SET abstract = 'backfilled abstract ' || id::text
                WHERE id <= %s
                ''',
                (args.update_count,),
            )
            conn.commit()
            writer_ms = (time.perf_counter() - writer_start) * 1000.0

            stop_event.set()
            worker.join(timeout=args.statement_timeout_ms / 1000.0 + 5.0)
            if fatal_errors:
                raise RuntimeError(
                    f'query worker failed for {table_name}: '
                    + '; '.join(fatal_errors)
                )
            elapsed_s = time.perf_counter() - worker_start
            after_write_state = maintenance_state(cur, index_name)

            maintenance_ms = None
            maintenance_converged = True
            final_state = after_write_state
            if mode_name == 'eventual':
                (
                    maintenance_converged,
                    maintenance_ms,
                    final_state,
                ) = wait_for_background_convergence(
                    cur,
                    index_name,
                    args.convergence_timeout_ms,
                    args.poll_interval_ms,
                )
            else:
                final_state = maintenance_state(cur, index_name)

    return ModeSummary(
        mode=mode_name,
        reloptions=reloptions,
        writer_ms=writer_ms,
        maintenance_ms=maintenance_ms,
        maintenance_converged=maintenance_converged,
        before_state=before_state,
        after_write_state=after_write_state,
        final_state=final_state,
        queries=summarize_queries(latencies_ms, counters, elapsed_s),
    )


def main() -> None:
    args = parse_args()
    reset_database(args)
    setup_database(args)

    eager_options = (
        "method = 'lucene', idf_method = 'lucene', "
        'auto_rebuild_threshold = 0'
    )
    eventual_options = (
        "method = 'lucene', idf_method = 'lucene', "
        "consistency = 'eventual', "
        'auto_rebuild_threshold = 1000000, '
        'query_overlay_max_records = 50000, '
        'query_overlay_max_bytes = 16777216'
    )

    modes = [
        run_mode(args, 'docs_eager', 'eager', eager_options),
        run_mode(args, 'docs_eventual', 'eventual', eventual_options),
    ]
    output = {
        'config': {
            'db_name': args.db_name,
            'doc_count': args.doc_count,
            'update_count': args.update_count,
            'query_k': args.query_k,
            'statement_timeout_ms': args.statement_timeout_ms,
            'query_sleep_ms': args.query_sleep_ms,
            'convergence_timeout_ms': args.convergence_timeout_ms,
            'poll_interval_ms': args.poll_interval_ms,
        },
        'modes': [asdict(mode) for mode in modes],
    }

    rendered = json.dumps(output, indent=2, sort_keys=True)
    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(rendered)
            f.write('\n')
    else:
        print(rendered)


if __name__ == '__main__':
    main()
