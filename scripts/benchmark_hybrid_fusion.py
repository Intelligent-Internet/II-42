#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import statistics
import time
from pathlib import Path
from typing import Any

import psycopg
from psycopg import conninfo, sql


DEFAULT_OUTPUT = Path(
    'docs/performance/data/diagnostics/hybrid-fusion-current.json'
)
DEFAULT_QUERIES = ['bird', 'cat', 'bird OR cat', 'policy']
BENCHMARK_DB = 'psql_bm25s_hybrid_fusion'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Benchmark hybrid BM25/vector-like fusion overhead.'
    )
    parser.add_argument(
        '--dsn',
        default=os.environ.get('PSQL_BM25S_BENCH_DSN', 'dbname=postgres'),
        help='PostgreSQL DSN used for the local benchmark session.',
    )
    parser.add_argument(
        '--docs',
        type=int,
        default=20000,
        help='Synthetic document count to generate.',
    )
    parser.add_argument(
        '--k',
        type=int,
        default=10,
        help='Top-k returned by each benchmarked query.',
    )
    parser.add_argument(
        '--candidate-ks',
        default='100,500,1000,5000',
        help='Comma-separated candidate counts to benchmark.',
    )
    parser.add_argument(
        '--repeats',
        type=int,
        default=20,
        help='Repeats per query, variant, and candidate count.',
    )
    parser.add_argument(
        '--output',
        type=Path,
        default=DEFAULT_OUTPUT,
        help='JSON output path.',
    )
    return parser.parse_args()


def summarize_latencies(latencies_ms: list[float]) -> dict[str, float]:
    ordered = sorted(latencies_ms)
    return {
        'count': len(ordered),
        'mean_ms': statistics.fmean(ordered),
        'p50_ms': ordered[len(ordered) // 2],
        'min_ms': ordered[0],
        'max_ms': ordered[-1],
    }


def drop_database_if_exists(cur: psycopg.Cursor[Any], db_name: str) -> None:
    cur.execute(
        """
        SELECT pg_terminate_backend(pid)
        FROM pg_stat_activity
        WHERE datname = %s
          AND pid <> pg_backend_pid()
        """,
        (db_name,),
    )
    cur.execute(
        sql.SQL('DROP DATABASE IF EXISTS {}').format(sql.Identifier(db_name))
    )


def setup_schema(cur: psycopg.Cursor[Any], docs: int) -> None:
    cur.execute('CREATE EXTENSION psql_bm25s')
    cur.execute('CREATE SCHEMA bench')
    cur.execute(
        """
        CREATE TABLE bench.docs (
            id integer PRIMARY KEY,
            title_tokens text[] NOT NULL,
            body_tokens text[] NOT NULL
        )
        """
    )
    cur.execute(
        """
        INSERT INTO bench.docs (id, title_tokens, body_tokens)
        SELECT
            i,
            ARRAY[
                CASE WHEN i %% 2 = 0 THEN 'bird' ELSE 'cat' END,
                CASE WHEN i %% 3 = 0 THEN 'policy' ELSE 'fruit' END,
                CASE WHEN i %% 5 = 0 THEN 'science' ELSE 'animal' END
            ],
            ARRAY[
                CASE WHEN i %% 2 = 0 THEN 'bird' ELSE 'dog' END,
                CASE WHEN i %% 3 = 0 THEN 'cat' ELSE 'fish' END,
                CASE WHEN i %% 7 = 0 THEN 'policy' ELSE 'forest' END,
                CASE WHEN i %% 11 = 0 THEN 'science' ELSE 'garden' END
            ]
        FROM generate_series(1, %s) AS g(i)
        """,
        (docs,),
    )
    cur.execute(
        """
        CREATE INDEX title_tokens_bm25_idx
        ON bench.docs USING psql_bm25s (title_tokens)
        WITH (
            method = 'lucene',
            idf_method = 'lucene',
            consistency = 'manual',
            create_empty_token = true
        )
        """
    )
    cur.execute(
        """
        CREATE INDEX body_tokens_bm25_idx
        ON bench.docs USING psql_bm25s (body_tokens)
        WITH (
            method = 'lucene',
            idf_method = 'lucene',
            consistency = 'manual',
            create_empty_token = true
        )
        """
    )
    cur.execute('ANALYZE bench.docs')


def vector_candidate_sql(candidate_k: int) -> str:
    return f"""
        SELECT public.psql_bm25s_hybrid_vector_candidate(
            'embedding',
            d.ctid,
            (
                abs(
                    ((d.id * 37 + length(%(query)s::text)) %% 997)
                    - 113
                )::float8 / 997.0
            )::real,
            row_number() OVER (
                ORDER BY
                    abs(
                        ((d.id * 37 + length(%(query)s::text)) %% 997)
                        - 113
                    )::float8 / 997.0,
                    d.id
            )::int4,
            1.0,
            'minmax'
        ) AS c
        FROM bench.docs AS d
        ORDER BY
            abs(
                ((d.id * 37 + length(%(query)s::text)) %% 997)
                - 113
            )::float8 / 997.0,
            d.id
        LIMIT {candidate_k}
    """


def sql_variants(
    k: int,
    candidate_k: int,
) -> dict[str, str]:
    vector_sql = vector_candidate_sql(candidate_k)
    variants = {
        'bm25_weighted_queries': f"""
            WITH hits AS (
                SELECT *
                FROM public.psql_bm25s_fusion_query_weighted(
                    ARRAY[
                        public.psql_bm25s_fusion_weighted_query(
                            'bench.title_tokens_bm25_idx'::regclass,
                            %(query)s::text,
                            2.0
                        ),
                        public.psql_bm25s_fusion_weighted_query(
                            'bench.body_tokens_bm25_idx'::regclass,
                            %(query)s::text,
                            1.0
                        )
                    ]::public.psql_bm25s_result_fusion_weighted_query[],
                    {k},
                    {candidate_k},
                    NULL::real[]
                )
            )
            SELECT
                array_agg(doc_id ORDER BY score DESC, ctid::text),
                array_agg(score ORDER BY score DESC, ctid::text)
            FROM hits
        """,
        'hybrid_rrf_two_bm25': f"""
            WITH title_candidates AS (
                SELECT c
                FROM public.psql_bm25s_hybrid_bm25_candidates(
                    'title',
                    'bench.title_tokens_bm25_idx'::regclass,
                    %(query)s::text,
                    2.0,
                    {candidate_k}
                ) AS c
            ),
            body_candidates AS (
                SELECT c
                FROM public.psql_bm25s_hybrid_bm25_candidates(
                    'body',
                    'bench.body_tokens_bm25_idx'::regclass,
                    %(query)s::text,
                    1.0,
                    {candidate_k}
                ) AS c
            ),
            hits AS (
                SELECT *
                FROM public.psql_bm25s_hybrid_fuse_candidates(
                    ARRAY(
                        SELECT c FROM title_candidates
                        UNION ALL
                        SELECT c FROM body_candidates
                    ),
                    {k},
                    'rrf'
                )
            )
            SELECT
                array_agg(d.id ORDER BY h.score DESC, d.id),
                array_agg(h.score ORDER BY h.score DESC, d.id)
            FROM hits AS h
            JOIN bench.docs AS d ON d.ctid = h.ctid
        """,
        'hybrid_rrf_bm25_vector': f"""
            WITH title_candidates AS (
                SELECT c
                FROM public.psql_bm25s_hybrid_bm25_candidates(
                    'title',
                    'bench.title_tokens_bm25_idx'::regclass,
                    %(query)s::text,
                    2.0,
                    {candidate_k}
                ) AS c
            ),
            body_candidates AS (
                SELECT c
                FROM public.psql_bm25s_hybrid_bm25_candidates(
                    'body',
                    'bench.body_tokens_bm25_idx'::regclass,
                    %(query)s::text,
                    1.0,
                    {candidate_k}
                ) AS c
            ),
            vector_candidates AS (
                {vector_sql}
            ),
            hits AS (
                SELECT *
                FROM public.psql_bm25s_hybrid_fuse_candidates(
                    ARRAY(
                        SELECT c FROM title_candidates
                        UNION ALL
                        SELECT c FROM body_candidates
                        UNION ALL
                        SELECT c FROM vector_candidates
                    ),
                    {k},
                    'rrf'
                )
            )
            SELECT
                array_agg(d.id ORDER BY h.score DESC, d.id),
                array_agg(h.score ORDER BY h.score DESC, d.id)
            FROM hits AS h
            JOIN bench.docs AS d ON d.ctid = h.ctid
        """,
        'hybrid_score_minmax_bm25_vector': f"""
            WITH title_candidates AS (
                SELECT c
                FROM public.psql_bm25s_hybrid_bm25_candidates(
                    'title',
                    'bench.title_tokens_bm25_idx'::regclass,
                    %(query)s::text,
                    2.0,
                    {candidate_k},
                    'minmax'
                ) AS c
            ),
            body_candidates AS (
                SELECT c
                FROM public.psql_bm25s_hybrid_bm25_candidates(
                    'body',
                    'bench.body_tokens_bm25_idx'::regclass,
                    %(query)s::text,
                    1.0,
                    {candidate_k},
                    'minmax'
                ) AS c
            ),
            vector_candidates AS (
                {vector_sql}
            ),
            hits AS (
                SELECT *
                FROM public.psql_bm25s_hybrid_fuse_candidates(
                    ARRAY(
                        SELECT c FROM title_candidates
                        UNION ALL
                        SELECT c FROM body_candidates
                        UNION ALL
                        SELECT c FROM vector_candidates
                    ),
                    {k},
                    'score'
                )
            )
            SELECT
                array_agg(d.id ORDER BY h.score DESC, d.id),
                array_agg(h.score ORDER BY h.score DESC, d.id)
            FROM hits AS h
            JOIN bench.docs AS d ON d.ctid = h.ctid
        """,
    }
    return variants


def benchmark_variant(
    cur: psycopg.Cursor[Any],
    sql_text: str,
    queries: list[str],
    repeats: int,
) -> dict[str, Any]:
    latencies_ms: list[float] = []
    first_results: dict[str, dict[str, list[Any]]] = {}

    for query_text in queries:
        for attempt in range(repeats):
            started = time.perf_counter()
            cur.execute(sql_text, {'query': query_text})
            row = cur.fetchone()
            elapsed_ms = (time.perf_counter() - started) * 1000.0
            latencies_ms.append(elapsed_ms)
            if attempt == 0:
                first_results[query_text] = {
                    'doc_ids': row[0] or [],
                    'scores': row[1] or [],
                }

    return {
        'latency_ms': summarize_latencies(latencies_ms),
        'query_results': first_results,
    }


def parse_candidate_ks(raw_value: str) -> list[int]:
    values = [int(item.strip()) for item in raw_value.split(',') if item.strip()]
    if not values:
        raise ValueError('at least one candidate count is required')
    if any(value <= 0 for value in values):
        raise ValueError('candidate counts must be positive')
    return values


def main() -> None:
    args = parse_args()
    candidate_ks = parse_candidate_ks(args.candidate_ks)
    queries = list(DEFAULT_QUERIES)

    results: dict[str, Any] = {
        'docs': args.docs,
        'queries': queries,
        'k': args.k,
        'candidate_ks': candidate_ks,
        'repeats': args.repeats,
        'variants': {},
    }

    admin_dsn = conninfo.make_conninfo(
        args.dsn,
        dbname='postgres',
        application_name='psql_bm25s_hybrid_admin',
    )
    benchmark_dsn = conninfo.make_conninfo(
        args.dsn,
        dbname=BENCHMARK_DB,
        application_name='psql_bm25s_hybrid_bench',
    )

    with psycopg.connect(admin_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            drop_database_if_exists(cur, BENCHMARK_DB)
            cur.execute(
                sql.SQL('CREATE DATABASE {}').format(
                    sql.Identifier(BENCHMARK_DB)
                )
            )

    try:
        with psycopg.connect(benchmark_dsn, autocommit=True) as conn:
            with conn.cursor() as cur:
                setup_schema(cur, args.docs)
                for candidate_k in candidate_ks:
                    key = str(candidate_k)
                    results['variants'][key] = {}
                    for name, sql_text in sql_variants(
                        args.k,
                        candidate_k,
                    ).items():
                        results['variants'][key][name] = benchmark_variant(
                            cur,
                            sql_text,
                            queries,
                            args.repeats,
                        )
    finally:
        with psycopg.connect(admin_dsn, autocommit=True) as conn:
            with conn.cursor() as cur:
                drop_database_if_exists(cur, BENCHMARK_DB)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2) + '\n', 'utf-8')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
