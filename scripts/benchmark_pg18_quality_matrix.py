#!/usr/bin/env python3
"""Run BEIR relevance metrics for the PG18 benchmark matrix.

The throughput matrix intentionally fetches aggregate rows only. This script is
separate because relevance metrics need the ranked document identifiers, which
changes client materialization overhead and should not be mixed with QPS data.
"""

from __future__ import annotations

import argparse
import gc
import json
import math
import os
import sys
import time
import traceback
from collections.abc import Callable
from pathlib import Path
from typing import Any

import bm25s
import psycopg
from psycopg import conninfo

from benchmark_beir_official import (
    combine_doc_text,
    ensure_extension,
    merge_cqadupstack,
    read_jsonl_map,
    tokenize_dataset,
)
from benchmark_pg18_matrix import (
    DEFAULT_DATASETS_DIR,
    DEFAULT_DB_PREFIX,
    OFFICIAL_ORDER,
    decode_tokens,
    docs_text_rows,
    joined_query_text,
)


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = (
    ROOT
    / 'docs/performance/data/diagnostics/'
    / 'pg18-beir-quality-matrix-current-2026-05-06.json'
)

QUALITY_TOP_K = 100
ENGINE_ORDER = [
    'python_reference_bm25s',
    'psql_bm25s_ids',
    'psql_bm25s_text',
    'pg_search',
    'vchord_bm25',
]


def benchmark_admin_dsn(
    env_name: str | None = None,
) -> str:
    env_dsn = os.environ.get(env_name) if env_name else None
    base_dsn = os.environ.get('PSQL_BM25S_BENCH_DSN', 'dbname=postgres')
    if env_dsn:
        base_dsn = env_dsn
    params = conninfo.conninfo_to_dict(base_dsn)
    if not params.get('dbname'):
        params['dbname'] = 'postgres'
    return conninfo.make_conninfo(**params)


def make_database_dsn(
    admin_dsn: str,
    db_name: str,
    application_name: str,
) -> str:
    return conninfo.make_conninfo(
        admin_dsn,
        dbname=db_name,
        application_name=application_name,
    )


def ensure_local_database(
    admin_dsn: str,
    db_name: str,
) -> None:
    with psycopg.connect(admin_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute(
                'SELECT EXISTS ('
                'SELECT 1 FROM pg_database WHERE datname = %s'
                ')',
                (db_name,),
            )
            if cur.fetchone()[0]:
                return
            cur.execute(f'CREATE DATABASE "{db_name}" TEMPLATE template0')


def relation_exists(
    cur: psycopg.Cursor[Any],
    name: str,
) -> bool:
    cur.execute('SELECT to_regclass(%s) IS NOT NULL', (name,))
    return bool(cur.fetchone()[0])


def table_row_count(
    cur: psycopg.Cursor[Any],
    table_name: str,
) -> int | None:
    if not relation_exists(cur, table_name):
        return None
    cur.execute(f'SELECT count(*) FROM {table_name}')
    return int(cur.fetchone()[0])


def extension_available(
    admin_dsn: str,
    extension_name: str,
) -> bool:
    with psycopg.connect(admin_dsn) as conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                SELECT EXISTS (
                    SELECT 1
                    FROM pg_available_extensions
                    WHERE name = %s
                )
                """,
                (extension_name,),
            )
            return bool(cur.fetchone()[0])


def read_qrels(
    path: Path,
) -> dict[str, dict[str, float]]:
    qrels: dict[str, dict[str, float]] = {}
    with path.open('r', encoding='utf-8') as handle:
        header = next(handle, '').rstrip('\n').split('\t')
        if len(header) < 3:
            raise ValueError(f'Invalid qrels header in {path}')
        for line in handle:
            parts = line.rstrip('\n').split('\t')
            if len(parts) < 3:
                continue
            query_id, corpus_id, score_text = parts[:3]
            score = float(score_text)
            qrels.setdefault(query_id, {})
            current = qrels[query_id].get(corpus_id)
            if current is None or score > current:
                qrels[query_id][corpus_id] = score
    return qrels


def load_quality_dataset(
    dataset: str,
    datasets_dir: Path,
) -> tuple[
    list[str],
    list[str],
    list[str],
    list[str],
    dict[str, dict[str, float]],
    int,
]:
    data_path = datasets_dir / dataset
    if not data_path.exists():
        raise FileNotFoundError(
            f'Local dataset cache is missing {dataset}: {data_path}'
        )
    if dataset == 'cqadupstack':
        merge_cqadupstack(data_path)

    corpus_path = data_path / 'corpus.jsonl'
    queries_path = data_path / 'queries.jsonl'
    qrels_path = data_path / 'qrels' / 'test.tsv'
    if not corpus_path.exists():
        raise FileNotFoundError(f'Missing corpus.jsonl for {dataset}')
    if not queries_path.exists():
        raise FileNotFoundError(f'Missing queries.jsonl for {dataset}')
    if not qrels_path.exists():
        raise FileNotFoundError(f'Missing test qrels for {dataset}')

    corpus = read_jsonl_map(corpus_path, lambda row: combine_doc_text(row))
    queries = read_jsonl_map(queries_path, lambda row: row['text'])
    qrels = read_qrels(qrels_path)

    eval_query_ids: list[str] = []
    eval_query_texts: list[str] = []
    for query_id, query_text in queries.items():
        if query_id not in qrels:
            continue
        eval_query_ids.append(query_id)
        eval_query_texts.append(query_text)

    return (
        list(corpus.keys()),
        list(corpus.values()),
        eval_query_ids,
        eval_query_texts,
        qrels,
        len(queries),
    )


def dcg(
    relevances: list[float],
) -> float:
    return sum(
        ((2.0 ** relevance) - 1.0) / math.log2(rank + 2.0)
        for rank, relevance in enumerate(relevances)
    )


def ndcg_at(
    ranked_ids: list[str],
    qrel: dict[str, float],
    k: int,
) -> float | None:
    positives = [score for score in qrel.values() if score > 0.0]
    if not positives:
        return None
    observed = [qrel.get(doc_id, 0.0) for doc_id in ranked_ids[:k]]
    ideal = sorted(positives, reverse=True)[:k]
    ideal_score = dcg(ideal)
    if ideal_score <= 0.0:
        return None
    return dcg(observed) / ideal_score


def average_precision_at(
    ranked_ids: list[str],
    qrel: dict[str, float],
    k: int,
) -> float | None:
    relevant = {doc_id for doc_id, score in qrel.items() if score > 0.0}
    if not relevant:
        return None
    hits = 0
    precision_sum = 0.0
    for rank, doc_id in enumerate(ranked_ids[:k], start=1):
        if doc_id not in relevant:
            continue
        hits += 1
        precision_sum += hits / rank
    return precision_sum / min(len(relevant), k)


def recall_at(
    ranked_ids: list[str],
    qrel: dict[str, float],
    k: int,
) -> float | None:
    relevant = {doc_id for doc_id, score in qrel.items() if score > 0.0}
    if not relevant:
        return None
    hits = len(set(ranked_ids[:k]) & relevant)
    return hits / len(relevant)


def precision_at(
    ranked_ids: list[str],
    qrel: dict[str, float],
    k: int,
) -> float | None:
    relevant = {doc_id for doc_id, score in qrel.items() if score > 0.0}
    if not relevant:
        return None
    hits = len(set(ranked_ids[:k]) & relevant)
    return hits / k


def evaluate_rankings(
    query_ids: list[str],
    qrels: dict[str, dict[str, float]],
    fetch_ranked_ids: Callable[[int], list[str]],
) -> dict[str, Any]:
    sums = {
        'ndcg@10': 0.0,
        'map@100': 0.0,
        'recall@100': 0.0,
        'precision@10': 0.0,
    }
    counts = {key: 0 for key in sums}
    started = time.perf_counter()

    for query_index, query_id in enumerate(query_ids):
        ranked_ids = fetch_ranked_ids(query_index)
        qrel = qrels[query_id]
        values = {
            'ndcg@10': ndcg_at(ranked_ids, qrel, 10),
            'map@100': average_precision_at(ranked_ids, qrel, 100),
            'recall@100': recall_at(ranked_ids, qrel, 100),
            'precision@10': precision_at(ranked_ids, qrel, 10),
        }
        for key, value in values.items():
            if value is None:
                continue
            sums[key] += value
            counts[key] += 1

    elapsed_ms = (time.perf_counter() - started) * 1000.0
    metrics = {
        key: (sums[key] / counts[key] if counts[key] else math.nan)
        for key in sums
    }
    return {
        'metrics': metrics,
        'evaluated_queries': len(query_ids),
        'metric_query_counts': counts,
        'query_ms': elapsed_ms,
        'qps': (
            len(query_ids) / (elapsed_ms / 1000.0)
            if elapsed_ms > 0.0
            else math.nan
        ),
    }


def ensure_psql_ids_index(
    dataset: str,
    corpus_id_tokens: list[list[int]],
    db_prefix: str,
    rebuild: bool,
) -> str:
    db_name = f'{db_prefix}psql_bm25s_{dataset.replace("-", "_")}'
    admin_dsn = benchmark_admin_dsn('PSQL_BM25S_MATRIX_PSQL_BM25S_DSN')
    ensure_local_database(admin_dsn, db_name)
    db_dsn = make_database_dsn(admin_dsn, db_name, 'psql_bm25s_quality')

    with psycopg.connect(db_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            ensure_extension(cur, reuse_db=True)
            ready = (
                not rebuild
                and table_row_count(cur, 'bench.docs_ids') == len(corpus_id_tokens)
                and relation_exists(cur, 'bench.docs_ids_bm25_idx')
            )
            if ready:
                return db_dsn

            cur.execute('DROP SCHEMA IF EXISTS bench CASCADE')
            cur.execute('CREATE SCHEMA bench')
            cur.execute(
                'CREATE TABLE bench.docs_ids ('
                'id integer PRIMARY KEY, '
                'token_ids int4[] NOT NULL'
                ')'
            )
            with cur.copy(
                'COPY bench.docs_ids (id, token_ids) FROM STDIN'
            ) as copy:
                for doc_id, token_ids in enumerate(
                    corpus_id_tokens,
                    start=1,
                ):
                    copy.write_row((doc_id, token_ids))
            cur.execute(
                """
                CREATE INDEX docs_ids_bm25_idx
                ON bench.docs_ids USING psql_bm25s (token_ids)
                WITH (
                    method = 'lucene',
                    idf_method = 'lucene',
                    k1 = 1.5,
                    b = 0.75,
                    delta = 0.5,
                    create_empty_token = true,
                    consistency = 'manual'
                )
                """
            )
    return db_dsn


def ensure_psql_text_index(
    dataset: str,
    corpus_id_tokens: list[list[int]],
    vocab_by_id: list[str],
    db_prefix: str,
    rebuild: bool,
) -> str:
    db_name = f'{db_prefix}psql_bm25s_text_{dataset.replace("-", "_")}'
    admin_dsn = benchmark_admin_dsn('PSQL_BM25S_MATRIX_PSQL_BM25S_DSN')
    ensure_local_database(admin_dsn, db_name)
    db_dsn = make_database_dsn(admin_dsn, db_name, 'psql_bm25s_quality')

    with psycopg.connect(db_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            ensure_extension(cur, reuse_db=True)
            ready = (
                not rebuild
                and table_row_count(cur, 'bench.docs_tokens')
                == len(corpus_id_tokens)
                and relation_exists(cur, 'bench.docs_tokens_bm25_idx')
            )
            if ready:
                return db_dsn

            cur.execute('DROP SCHEMA IF EXISTS bench CASCADE')
            cur.execute('CREATE SCHEMA bench')
            cur.execute(
                'CREATE TABLE bench.docs_tokens ('
                'id integer PRIMARY KEY, '
                'tokens text[] NOT NULL'
                ')'
            )
            with cur.copy(
                'COPY bench.docs_tokens (id, tokens) FROM STDIN'
            ) as copy:
                for doc_id, token_ids in enumerate(
                    corpus_id_tokens,
                    start=1,
                ):
                    copy.write_row(
                        (doc_id, decode_tokens(token_ids, vocab_by_id))
                    )
            cur.execute(
                """
                CREATE INDEX docs_tokens_bm25_idx
                ON bench.docs_tokens USING psql_bm25s (tokens)
                WITH (
                    method = 'lucene',
                    idf_method = 'lucene',
                    k1 = 1.5,
                    b = 0.75,
                    delta = 0.5,
                    consistency = 'manual'
                )
                """
            )
    return db_dsn


def ensure_pg_search_index(
    dataset: str,
    corpus_id_tokens: list[list[int]],
    vocab_by_id: list[str],
    db_prefix: str,
    rebuild: bool,
) -> str:
    db_name = f'{db_prefix}pg_search_{dataset.replace("-", "_")}'
    admin_dsn = benchmark_admin_dsn('PSQL_BM25S_MATRIX_PG_SEARCH_DSN')
    ensure_local_database(admin_dsn, db_name)
    db_dsn = make_database_dsn(admin_dsn, db_name, 'pg_search_quality')
    rows = docs_text_rows(corpus_id_tokens, vocab_by_id)

    with psycopg.connect(db_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute('CREATE EXTENSION IF NOT EXISTS pg_search')
            ready = (
                not rebuild
                and table_row_count(cur, 'bench.docs') == len(corpus_id_tokens)
                and relation_exists(cur, 'bench.docs_body_bm25_idx')
            )
            if ready:
                return db_dsn

            cur.execute('DROP SCHEMA IF EXISTS bench CASCADE')
            cur.execute('CREATE SCHEMA bench')
            cur.execute(
                'CREATE TABLE bench.docs ('
                'id integer PRIMARY KEY, '
                'body text NOT NULL'
                ')'
            )
            with cur.copy('COPY bench.docs (id, body) FROM STDIN') as copy:
                for row in rows:
                    copy.write_row(row)
            cur.execute(
                """
                CREATE INDEX docs_body_bm25_idx
                ON bench.docs USING bm25 (id, body)
                WITH (key_field = id)
                """
            )
    return db_dsn


def ensure_vchord_index(
    dataset: str,
    corpus_id_tokens: list[list[int]],
    db_prefix: str,
    rebuild: bool,
) -> str:
    db_name = f'{db_prefix}vchord_bm25_{dataset.replace("-", "_")}'
    admin_dsn = benchmark_admin_dsn('PSQL_BM25S_MATRIX_VCHORD_BM25_DSN')
    if not extension_available(admin_dsn, 'vchord_bm25'):
        raise RuntimeError('extension vchord_bm25 is not available locally')
    ensure_local_database(admin_dsn, db_name)
    db_dsn = make_database_dsn(admin_dsn, db_name, 'vchord_bm25_quality')

    with psycopg.connect(db_dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute('CREATE EXTENSION IF NOT EXISTS vchord_bm25')
            cur.execute(
                "SELECT set_config('search_path', 'public,bm25_catalog', false)"
            )
            ready = (
                not rebuild
                and table_row_count(cur, 'bench.docs_raw')
                == len(corpus_id_tokens)
                and relation_exists(cur, 'bench.docs_embedding_bm25_idx')
            )
            if ready:
                return db_dsn

            cur.execute('DROP SCHEMA IF EXISTS bench CASCADE')
            cur.execute('CREATE SCHEMA bench')
            cur.execute(
                'CREATE TABLE bench.docs_raw ('
                'id integer PRIMARY KEY, '
                'token_ids int4[] NOT NULL'
                ')'
            )
            cur.execute(
                'CREATE TABLE bench.docs ('
                'id integer PRIMARY KEY, '
                'embedding bm25vector NOT NULL'
                ')'
            )
            with cur.copy(
                'COPY bench.docs_raw (id, token_ids) FROM STDIN'
            ) as copy:
                for doc_id, token_ids in enumerate(
                    corpus_id_tokens,
                    start=1,
                ):
                    copy.write_row((doc_id, token_ids))
            cur.execute(
                """
                INSERT INTO bench.docs (id, embedding)
                SELECT id, token_ids::bm25vector
                FROM bench.docs_raw
                """
            )
            cur.execute(
                """
                CREATE INDEX docs_embedding_bm25_idx
                ON bench.docs USING bm25 (embedding bm25_ops)
                """
            )
    return db_dsn


def evaluate_python_reference(
    corpus_ids: list[str],
    corpus_tokenized: Any,
    query_ids: list[str],
    query_id_tokens: list[list[int]],
) -> dict[str, Any]:
    retriever = bm25s.BM25(method='lucene', idf_method='lucene')
    build_started = time.perf_counter()
    retriever.index(corpus_tokenized, leave_progress=False)
    build_ms = (time.perf_counter() - build_started) * 1000.0

    batch_size = 512
    ranked_by_query: dict[int, list[str]] = {}
    query_started = time.perf_counter()
    for start_idx in range(0, len(query_id_tokens), batch_size):
        batch = query_id_tokens[start_idx:start_idx + batch_size]
        results = retriever.retrieve(
            batch,
            corpus=corpus_ids,
            k=QUALITY_TOP_K,
            return_as='tuple',
            n_threads=1,
        )
        for offset, docs in enumerate(results.documents):
            ranked_by_query[start_idx + offset] = [str(doc_id) for doc_id in docs]
    query_ms = (time.perf_counter() - query_started) * 1000.0

    def fetch(query_index: int) -> list[str]:
        return ranked_by_query.get(query_index, [])

    result = evaluate_rankings(query_ids, CURRENT_QRELS, fetch)
    result['build_ms'] = build_ms
    result['query_ms'] = query_ms
    result['qps'] = (
        len(query_ids) / (query_ms / 1000.0)
        if query_ms > 0.0
        else math.nan
    )
    return result


def evaluate_psql_ids(
    db_dsn: str,
    corpus_ids: list[str],
    query_ids: list[str],
    query_id_tokens: list[list[int]],
) -> dict[str, Any]:
    query_sql = """
        SELECT docs.id, hits.score
        FROM public.psql_bm25s_query_ids(
            'bench.docs_ids_bm25_idx'::regclass,
            %s::int4[],
            %s::int4,
            NULL
        ) AS hits
        JOIN bench.docs_ids AS docs ON docs.ctid = hits.ctid
        ORDER BY hits.score DESC, docs.id
    """
    with psycopg.connect(db_dsn) as conn:
        with conn.cursor() as cur:
            def fetch(query_index: int) -> list[str]:
                token_ids = query_id_tokens[query_index]
                if not token_ids:
                    return []
                cur.execute(query_sql, (token_ids, QUALITY_TOP_K))
                return [corpus_ids[int(row[0]) - 1] for row in cur.fetchall()]

            return evaluate_rankings(query_ids, CURRENT_QRELS, fetch)


def evaluate_psql_text(
    db_dsn: str,
    corpus_ids: list[str],
    query_ids: list[str],
    query_id_tokens: list[list[int]],
    vocab_by_id: list[str],
) -> dict[str, Any]:
    query_sql = """
        SELECT docs.id, hits.score
        FROM public.psql_bm25s_query_tokens(
            'bench.docs_tokens_bm25_idx'::regclass,
            %s::text[],
            %s::int4,
            NULL
        ) AS hits
        JOIN bench.docs_tokens AS docs ON docs.ctid = hits.ctid
        ORDER BY hits.score DESC, docs.id
    """
    with psycopg.connect(db_dsn) as conn:
        with conn.cursor() as cur:
            def fetch(query_index: int) -> list[str]:
                token_ids = query_id_tokens[query_index]
                if not token_ids:
                    return []
                tokens = decode_tokens(token_ids, vocab_by_id)
                cur.execute(query_sql, (tokens, QUALITY_TOP_K))
                return [corpus_ids[int(row[0]) - 1] for row in cur.fetchall()]

            return evaluate_rankings(query_ids, CURRENT_QRELS, fetch)


def evaluate_pg_search(
    db_dsn: str,
    corpus_ids: list[str],
    query_ids: list[str],
    query_id_tokens: list[list[int]],
    vocab_by_id: list[str],
) -> dict[str, Any]:
    query_sql = """
        SELECT id, pdb.score(id) AS score
        FROM bench.docs
        WHERE body @@@ %s
        ORDER BY score DESC, id
        LIMIT %s
    """
    with psycopg.connect(db_dsn) as conn:
        with conn.cursor() as cur:
            def fetch(query_index: int) -> list[str]:
                query_text = joined_query_text(
                    query_id_tokens[query_index],
                    vocab_by_id,
                )
                if not query_text:
                    return []
                cur.execute(query_sql, (query_text, QUALITY_TOP_K))
                return [corpus_ids[int(row[0]) - 1] for row in cur.fetchall()]

            return evaluate_rankings(query_ids, CURRENT_QRELS, fetch)


def evaluate_vchord(
    db_dsn: str,
    corpus_ids: list[str],
    query_ids: list[str],
    query_id_tokens: list[list[int]],
) -> dict[str, Any]:
    query_sql = """
        SELECT id, embedding <&> to_bm25query(
            'bench.docs_embedding_bm25_idx'::regclass,
            %s::int4[]::bm25vector
        ) AS rank
        FROM bench.docs
        ORDER BY rank, id
        LIMIT %s
    """
    with psycopg.connect(db_dsn) as conn:
        with conn.cursor() as cur:
            cur.execute(
                "SELECT set_config('search_path', 'public,bm25_catalog', false)"
            )

            def fetch(query_index: int) -> list[str]:
                token_ids = query_id_tokens[query_index]
                if not token_ids:
                    return []
                cur.execute(query_sql, (token_ids, QUALITY_TOP_K))
                return [corpus_ids[int(row[0]) - 1] for row in cur.fetchall()]

            return evaluate_rankings(query_ids, CURRENT_QRELS, fetch)


CURRENT_QRELS: dict[str, dict[str, float]] = {}


def run_dataset(
    dataset: str,
    datasets_dir: Path,
    db_prefix: str,
    paths: list[str],
    rebuild: bool,
    skip_unavailable: bool,
) -> dict[str, Any]:
    global CURRENT_QRELS

    started = time.perf_counter()
    (
        corpus_ids,
        corpus_texts,
        eval_query_ids,
        eval_query_texts,
        qrels,
        total_queries,
    ) = load_quality_dataset(dataset, datasets_dir)
    CURRENT_QRELS = qrels
    corpus_tokenized, query_id_tokens, vocab_by_id = tokenize_dataset(
        corpus_texts,
        eval_query_texts,
    )
    corpus_id_tokens = corpus_tokenized.ids

    result: dict[str, Any] = {
        'dataset': dataset,
        'stats': {
            'documents': len(corpus_ids),
            'queries': total_queries,
            'qrels_queries': len(qrels),
            'evaluated_queries': len(eval_query_ids),
            'tokens': sum(len(ids) for ids in corpus_id_tokens),
            'query_tokens': sum(len(ids) for ids in query_id_tokens),
            'vocab_size': len(corpus_tokenized.vocab),
        },
    }

    def run_path(path: str) -> None:
        print(f'[{dataset}] {path}', flush=True)
        if path == 'python_reference_bm25s':
            result[path] = evaluate_python_reference(
                corpus_ids,
                corpus_tokenized,
                eval_query_ids,
                query_id_tokens,
            )
            return
        if path == 'psql_bm25s_ids':
            db_dsn = ensure_psql_ids_index(
                dataset,
                corpus_id_tokens,
                db_prefix,
                rebuild,
            )
            result[path] = evaluate_psql_ids(
                db_dsn,
                corpus_ids,
                eval_query_ids,
                query_id_tokens,
            )
            return
        if path == 'psql_bm25s_text':
            db_dsn = ensure_psql_text_index(
                dataset,
                corpus_id_tokens,
                vocab_by_id,
                db_prefix,
                rebuild,
            )
            result[path] = evaluate_psql_text(
                db_dsn,
                corpus_ids,
                eval_query_ids,
                query_id_tokens,
                vocab_by_id,
            )
            return
        if path == 'pg_search':
            db_dsn = ensure_pg_search_index(
                dataset,
                corpus_id_tokens,
                vocab_by_id,
                db_prefix,
                rebuild,
            )
            result[path] = evaluate_pg_search(
                db_dsn,
                corpus_ids,
                eval_query_ids,
                query_id_tokens,
                vocab_by_id,
            )
            return
        if path == 'vchord_bm25':
            db_dsn = ensure_vchord_index(
                dataset,
                corpus_id_tokens,
                db_prefix,
                rebuild,
            )
            result[path] = evaluate_vchord(
                db_dsn,
                corpus_ids,
                eval_query_ids,
                query_id_tokens,
            )
            return
        raise ValueError(f'unknown path: {path}')

    for path in paths:
        try:
            run_path(path)
        except Exception as exc:
            if not skip_unavailable:
                raise
            result[path] = {
                'error': str(exc),
                'traceback': traceback.format_exc(),
            }

    result['wall_time_s'] = time.perf_counter() - started

    CURRENT_QRELS = {}
    del corpus_ids
    del corpus_texts
    del eval_query_ids
    del eval_query_texts
    del corpus_tokenized
    del query_id_tokens
    del vocab_by_id
    gc.collect()
    return result


def load_existing_results(
    output_path: Path,
) -> dict[str, Any]:
    if output_path.exists():
        return json.loads(output_path.read_text(encoding='utf-8'))
    return {
        'created_at': time.strftime('%Y-%m-%d %H:%M:%S'),
        'quality_top_k': QUALITY_TOP_K,
        'metrics': ['ndcg@10', 'map@100', 'recall@100', 'precision@10'],
        'paths': [],
        'results': {},
    }


def persist_results(
    output_path: Path,
    payload: dict[str, Any],
) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(payload, indent=2, sort_keys=True),
        encoding='utf-8',
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Compute BEIR quality metrics for local PG18 BM25 engines.'
    )
    parser.add_argument(
        '--datasets',
        nargs='*',
        default=OFFICIAL_ORDER,
        choices=OFFICIAL_ORDER,
        help='Datasets to evaluate. Defaults to the full BEIR matrix.',
    )
    parser.add_argument(
        '--datasets-dir',
        type=Path,
        default=DEFAULT_DATASETS_DIR,
    )
    parser.add_argument(
        '--output',
        type=Path,
        default=DEFAULT_OUTPUT,
    )
    parser.add_argument(
        '--db-prefix',
        default=DEFAULT_DB_PREFIX,
    )
    parser.add_argument(
        '--paths',
        nargs='+',
        default=ENGINE_ORDER,
        choices=ENGINE_ORDER,
    )
    parser.add_argument(
        '--resume',
        action='store_true',
    )
    parser.add_argument(
        '--rebuild',
        action='store_true',
        help='Rebuild benchmark tables and indexes even when they exist.',
    )
    parser.add_argument(
        '--skip-unavailable',
        action='store_true',
        help='Record path errors and continue with the next engine.',
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    payload = load_existing_results(args.output)
    existing_paths = payload.get('paths', [])
    if not isinstance(existing_paths, list):
        existing_paths = []
    payload['paths'] = [
        path
        for path in ENGINE_ORDER
        if path in existing_paths or path in args.paths
    ]
    payload['datasets_dir'] = str(args.datasets_dir)
    payload['db_prefix'] = args.db_prefix
    payload['quality_top_k'] = QUALITY_TOP_K
    payload['metrics'] = [
        'ndcg@10',
        'map@100',
        'recall@100',
        'precision@10',
    ]

    for dataset in args.datasets:
        existing = payload['results'].get(dataset)
        merge_existing = False
        if args.resume and existing and 'error' not in existing:
            missing = [
                path
                for path in args.paths
                if path not in existing
            ]
            if not missing:
                continue
            run_paths = missing
            merge_existing = True
        else:
            run_paths = args.paths
        try:
            result = run_dataset(
                dataset,
                args.datasets_dir,
                args.db_prefix,
                run_paths,
                args.rebuild,
                args.skip_unavailable,
            )
            if (
                merge_existing
                and existing
            ):
                merged = dict(existing)
                for path in run_paths:
                    if path in result:
                        merged[path] = result[path]
                merged['wall_time_s'] = (
                    float(existing.get('wall_time_s', 0.0)) +
                    float(result.get('wall_time_s', 0.0))
                )
                payload['results'][dataset] = merged
            else:
                payload['results'][dataset] = result
        except Exception as exc:
            error_payload = {
                'dataset': dataset,
                'error': str(exc),
                'traceback': traceback.format_exc(),
            }
            if (
                merge_existing
                and existing
                and 'error' not in existing
            ):
                merged = dict(existing)
                resume_errors = dict(merged.get('resume_errors', {}))
                resume_errors[','.join(run_paths)] = error_payload
                merged['resume_errors'] = resume_errors
                payload['results'][dataset] = merged
            else:
                payload['results'][dataset] = error_payload
        persist_results(args.output, payload)

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
