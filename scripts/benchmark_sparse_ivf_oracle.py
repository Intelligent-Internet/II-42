#!/usr/bin/env python3
"""Evaluate a compact global-IVF oracle for P2 sparse vectors."""

from __future__ import annotations

import argparse
import json
import statistics
import time
from pathlib import Path
from typing import Any

import numpy as np
from scipy import sparse
from sklearn.cluster import MiniBatchKMeans
from sklearn.decomposition import TruncatedSVD
from sklearn.preprocessing import normalize

from benchmark_semantic_error_budget import (
    load_qrels,
    mean_metric,
    percentile,
    query_metrics,
)
from benchmark_seismic_npz_oracle import load_csr, load_ids


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--documents-npz', type=Path, required=True)
    parser.add_argument('--queries-npz', type=Path, required=True)
    parser.add_argument('--document-ids', type=Path, required=True)
    parser.add_argument('--query-ids', type=Path, required=True)
    parser.add_argument('--qrels-json', type=Path, required=True)
    parser.add_argument('--clusters', type=int, default=1024)
    parser.add_argument('--projection-dim', type=int, default=32)
    parser.add_argument('--candidate-multiplier', type=int, default=4)
    parser.add_argument('--k', type=int, default=100)
    parser.add_argument('--summary-energy', type=float, default=0.1)
    parser.add_argument('--query-limit', type=int, default=0)
    parser.add_argument('--output', type=Path)
    return parser.parse_args()


def topk_offsets(scores: np.ndarray, k: int) -> list[int]:
    count = min(k, scores.size)
    if count == 0:
        return []
    unordered = np.argpartition(-scores, count - 1)[:count]
    return sorted(
        (int(offset) for offset in unordered),
        key=lambda offset: (-float(scores[offset]), offset),
    )


def exact_rankings(
    documents: sparse.csr_matrix,
    queries: sparse.csr_matrix,
    query_ids: list[str],
    k: int,
) -> dict[str, list[int]]:
    rankings: dict[str, list[int]] = {}
    for offset, query_id in enumerate(query_ids):
        product = documents @ queries.getrow(offset).transpose()
        scores = np.asarray(product.toarray()).reshape(-1)
        rankings[query_id] = topk_offsets(scores, k)
    return rankings


def build_summaries(
    documents: sparse.csr_matrix,
    labels: np.ndarray,
    cluster_count: int,
    energy: float,
) -> tuple[sparse.csr_matrix, list[np.ndarray]]:
    rows: list[int] = []
    columns: list[int] = []
    values: list[float] = []
    members: list[np.ndarray] = []
    for cluster_id in range(cluster_count):
        document_offsets = np.flatnonzero(labels == cluster_id)
        members.append(document_offsets.astype(np.int32, copy=False))
        if document_offsets.size == 0:
            continue
        maximum = documents[document_offsets].max(axis=0)
        maximum = sparse.coo_matrix(maximum)
        order = np.argsort(-maximum.data, kind='stable')
        ordered_values = maximum.data[order]
        keep = ordered_values.size
        if ordered_values.size > 0 and energy < 1.0:
            threshold = float(ordered_values.sum()) * energy
            keep = int(
                np.searchsorted(
                    np.cumsum(ordered_values),
                    threshold,
                    side='left',
                )
                + 1
            )
        selected = order[:keep]
        rows.extend([cluster_id] * keep)
        columns.extend(int(value) for value in maximum.col[selected])
        values.extend(float(value) for value in maximum.data[selected])
    summaries = sparse.csr_matrix(
        (values, (rows, columns)),
        shape=(cluster_count, documents.shape[1]),
        dtype=np.float32,
    )
    summaries.sort_indices()
    return summaries, members


def metric_summary(
    rankings: dict[str, list[tuple[str, float]]],
    qrels: dict[str, dict[str, float]],
) -> dict[str, float]:
    rows = [
        query_metrics(ranking, qrels.get(query_id, {}))
        for query_id, ranking in rankings.items()
    ]
    return {
        name: mean_metric(rows, name)
        for name in (
            'ndcg_at_10',
            'map_at_100',
            'recall_at_100',
            'mrr_at_20',
        )
    }


def evaluate(
    documents: sparse.csr_matrix,
    queries: sparse.csr_matrix,
    document_ids: list[str],
    query_ids: list[str],
    qrels: dict[str, dict[str, float]],
    exact: dict[str, list[int]],
    cluster_scores: np.ndarray,
    members: list[np.ndarray],
    candidate_k: int,
    k: int,
) -> dict[str, Any]:
    latencies: list[float] = []
    candidate_counts: list[int] = []
    overlaps: list[float] = []
    coverages: list[float] = []
    rankings: dict[str, list[tuple[str, float]]] = {}
    for query_offset, query_id in enumerate(query_ids):
        started = time.perf_counter()
        ordered_clusters = np.argsort(
            -cluster_scores[:, query_offset],
            kind='stable',
        )
        selected: list[np.ndarray] = []
        selected_count = 0
        for cluster_id in ordered_clusters:
            cluster_members = members[int(cluster_id)]
            if cluster_members.size == 0:
                continue
            selected.append(cluster_members)
            selected_count += cluster_members.size
            if selected_count >= candidate_k:
                break
        candidates = np.unique(np.concatenate(selected))
        product = (
            documents[candidates]
            @ queries.getrow(query_offset).transpose()
        )
        scores = np.asarray(product.toarray()).reshape(-1)
        local_top = topk_offsets(scores, k)
        result = [int(candidates[offset]) for offset in local_top]
        latencies.append((time.perf_counter() - started) * 1000.0)
        candidate_counts.append(int(candidates.size))
        exact_set = set(exact[query_id])
        overlaps.append(len(set(result) & exact_set) / len(exact_set))
        coverages.append(len(set(candidates) & exact_set) / len(exact_set))
        rankings[query_id] = [
            (document_ids[document], float(scores[local_offset]))
            for document, local_offset in zip(result, local_top, strict=True)
        ]
    return {
        'latency_ms': {
            'p50': statistics.median(latencies),
            'p95': percentile(latencies, 0.95),
        },
        'candidate_count': {
            'p50': statistics.median(candidate_counts),
            'p95': percentile(candidate_counts, 0.95),
        },
        'mean_topk_overlap': statistics.fmean(overlaps),
        'mean_exact_topk_candidate_coverage': statistics.fmean(coverages),
        'metrics': metric_summary(rankings, qrels),
    }


def evaluate_exact_bounds(
    documents: sparse.csr_matrix,
    queries: sparse.csr_matrix,
    document_ids: list[str],
    query_ids: list[str],
    qrels: dict[str, dict[str, float]],
    exact: dict[str, list[int]],
    upper_bounds: np.ndarray,
    members: list[np.ndarray],
    k: int,
) -> dict[str, Any]:
    latencies: list[float] = []
    candidate_counts: list[int] = []
    overlaps: list[float] = []
    rankings: dict[str, list[tuple[str, float]]] = {}
    for query_offset, query_id in enumerate(query_ids):
        started = time.perf_counter()
        ordered_clusters = np.argsort(
            -upper_bounds[:, query_offset],
            kind='stable',
        )
        opened_documents: list[np.ndarray] = []
        opened_scores: list[np.ndarray] = []
        threshold = -np.inf
        for cluster_id in ordered_clusters:
            bound = float(upper_bounds[int(cluster_id), query_offset])
            if opened_scores and bound < threshold:
                break
            cluster_members = members[int(cluster_id)]
            if cluster_members.size == 0:
                continue
            product = (
                documents[cluster_members]
                @ queries.getrow(query_offset).transpose()
            )
            scores = np.asarray(product.toarray()).reshape(-1)
            opened_documents.append(cluster_members)
            opened_scores.append(scores)
            all_scores = np.concatenate(opened_scores)
            if all_scores.size >= k:
                threshold = float(
                    np.partition(all_scores, all_scores.size - k)[
                        all_scores.size - k
                    ]
                )
        candidates = np.concatenate(opened_documents)
        scores = np.concatenate(opened_scores)
        local_top = topk_offsets(scores, k)
        result = [int(candidates[offset]) for offset in local_top]
        latencies.append((time.perf_counter() - started) * 1000.0)
        candidate_counts.append(int(candidates.size))
        exact_set = set(exact[query_id])
        overlaps.append(len(set(result) & exact_set) / len(exact_set))
        rankings[query_id] = [
            (document_ids[document], float(scores[local_offset]))
            for document, local_offset in zip(result, local_top, strict=True)
        ]
    return {
        'latency_ms': {
            'p50': statistics.median(latencies),
            'p95': percentile(latencies, 0.95),
        },
        'candidate_count': {
            'p50': statistics.median(candidate_counts),
            'p95': percentile(candidate_counts, 0.95),
        },
        'mean_topk_overlap': statistics.fmean(overlaps),
        'metrics': metric_summary(rankings, qrels),
    }


def main() -> int:
    args = parse_args()
    documents = load_csr(args.documents_npz)
    queries = load_csr(args.queries_npz)
    if args.query_limit > 0:
        queries = queries[: args.query_limit]
    document_ids = load_ids(args.document_ids, 0)
    query_ids = load_ids(args.query_ids, args.query_limit)
    qrels = load_qrels(args.qrels_json)
    exact = exact_rankings(documents, queries, query_ids, args.k)

    build_started = time.perf_counter()
    svd = TruncatedSVD(
        n_components=args.projection_dim,
        n_iter=5,
        random_state=42,
    )
    projected_documents = normalize(svd.fit_transform(documents))
    clusterer = MiniBatchKMeans(
        n_clusters=args.clusters,
        batch_size=4096,
        max_iter=100,
        n_init=1,
        random_state=42,
        reassignment_ratio=0.0,
    )
    labels = clusterer.fit_predict(projected_documents)
    summaries, members = build_summaries(
        documents,
        labels,
        args.clusters,
        args.summary_energy,
    )
    build_seconds = time.perf_counter() - build_started
    projected_queries = normalize(svd.transform(queries))
    centroid_scores = clusterer.cluster_centers_ @ projected_queries.T
    summary_scores = summaries @ queries.transpose()
    summary_scores = np.asarray(summary_scores.toarray())
    candidate_k = args.k * args.candidate_multiplier
    variants = {
        'projected_centroid': evaluate(
            documents,
            queries,
            document_ids,
            query_ids,
            qrels,
            exact,
            centroid_scores,
            members,
            candidate_k,
            args.k,
        ),
        'sparse_max_summary': evaluate(
            documents,
            queries,
            document_ids,
            query_ids,
            qrels,
            exact,
            summary_scores,
            members,
            candidate_k,
            args.k,
        ),
    }
    if args.summary_energy == 1.0:
        variants['exact_sparse_max_bound'] = evaluate_exact_bounds(
            documents,
            queries,
            document_ids,
            query_ids,
            qrels,
            exact,
            summary_scores,
            members,
            args.k,
        )
    estimated_bytes = (
        documents.shape[0] * 4
        + (args.clusters + 1) * 4
        + summaries.nnz * 5
        + args.clusters * 8
    )
    result = {
        'document_count': documents.shape[0],
        'query_count': queries.shape[0],
        'build_seconds': build_seconds,
        'clusters': args.clusters,
        'projection_dim': args.projection_dim,
        'summary_energy': args.summary_energy,
        'summary_nnz': summaries.nnz,
        'estimated_accelerator_bytes': estimated_bytes,
        'variants': variants,
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    if args.output is not None:
        args.output.write_text(
            json.dumps(result, indent=2, sort_keys=True) + '\n',
            encoding='utf-8',
        )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
