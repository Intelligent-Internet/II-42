#!/usr/bin/env python3
"""Compare source-ordered and cohesive term-local posting blocks."""

from __future__ import annotations

import argparse
import json
import math
import statistics
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np
from scipy import sparse


@dataclass
class TermBlocks:
    documents: np.ndarray
    offsets: np.ndarray
    summaries: sparse.csr_matrix
    summary_support: sparse.csr_matrix | None = None
    centroid_residual_norms: np.ndarray | None = None
    radii: np.ndarray | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--documents-npz', type=Path, required=True)
    parser.add_argument('--queries-npz', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--query-limit', type=int, default=64)
    parser.add_argument('--k', type=int, default=100)
    parser.add_argument('--query-cut', type=int, default=16)
    parser.add_argument('--document-cut', type=int, default=15)
    parser.add_argument('--block-size', type=int, default=64)
    parser.add_argument('--signature-bits', type=int, default=32)
    parser.add_argument('--centroid-cut', type=int, default=64)
    parser.add_argument('--summary-cut', type=int, default=64)
    parser.add_argument('--candidate-multiplier', type=int, default=8)
    parser.add_argument(
        '--execution',
        choices=('exact-bound', 'bounded-candidate'),
        default='exact-bound',
    )
    parser.add_argument(
        '--bound',
        choices=('coordinate-max', 'centroid-ball'),
        default='coordinate-max',
    )
    parser.add_argument('--accelerated-posting-mass', type=float, default=0.30)
    return parser.parse_args()


def load_csr(path: Path) -> sparse.csr_matrix:
    with np.load(path, allow_pickle=False) as payload:
        matrix = sparse.csr_matrix(
            (
                payload['data'],
                payload['indices'],
                payload['indptr'],
            ),
            shape=tuple(int(value) for value in payload['shape']),
        )
    matrix.sort_indices()
    return matrix


def splitmix64(values: np.ndarray) -> np.ndarray:
    result = values.astype(np.uint64, copy=True)
    result = np.add(result, np.uint64(0x9E3779B97F4A7C15))
    shifted = np.right_shift(result, np.uint64(30))
    result = np.multiply(
        np.bitwise_xor(result, shifted),
        np.uint64(0xBF58476D1CE4E5B9),
    )
    shifted = np.right_shift(result, np.uint64(27))
    result = np.multiply(
        np.bitwise_xor(result, shifted),
        np.uint64(0x94D049BB133111EB),
    )
    shifted = np.right_shift(result, np.uint64(31))
    return np.bitwise_xor(result, shifted)


def document_signatures(
    documents: sparse.csr_matrix,
    document_cut: int,
    signature_bits: int,
) -> np.ndarray:
    term_ids: list[np.ndarray] = []
    impacts: list[np.ndarray] = []
    document_ids: list[np.ndarray] = []

    for document in range(documents.shape[0]):
        start = int(documents.indptr[document])
        end = int(documents.indptr[document + 1])
        count = end - start
        if count == 0:
            continue
        local_ids = documents.indices[start:end]
        local_impacts = documents.data[start:end]
        if count > document_cut:
            selected = np.argpartition(
                -local_impacts,
                document_cut - 1,
            )[:document_cut]
            local_ids = local_ids[selected]
            local_impacts = local_impacts[selected]
        term_ids.append(local_ids.astype(np.int64, copy=False))
        impacts.append(local_impacts.astype(np.float64, copy=False))
        document_ids.append(
            np.full(len(local_ids), document, dtype=np.int64)
        )

    signatures = np.zeros(documents.shape[0], dtype=np.uint64)
    if not term_ids:
        return signatures
    flat_terms = np.concatenate(term_ids)
    flat_impacts = np.concatenate(impacts)
    flat_documents = np.concatenate(document_ids)
    term_hashes = splitmix64(
        np.arange(documents.shape[1], dtype=np.uint64)
        ^ np.uint64(0xD1B54A32D192ED03)
    )
    entry_hashes = term_hashes[flat_terms]
    for bit in range(signature_bits):
        hash_bits = np.bitwise_and(
            np.right_shift(entry_hashes, np.uint64(bit)),
            np.uint64(1),
        )
        signs = np.where(
            hash_bits != 0,
            1.0,
            -1.0,
        )
        projection = np.bincount(
            flat_documents,
            weights=flat_impacts * signs,
            minlength=documents.shape[0],
        )
        decision = np.greater_equal(projection, 0.0).astype(np.uint64)
        signatures = np.bitwise_or(
            signatures,
            np.left_shift(decision, np.uint64(bit)),
        )
    return signatures


def select_accelerated_terms(
    document_frequencies: np.ndarray,
    target_mass: float,
) -> tuple[np.ndarray, float]:
    posting_count = int(document_frequencies.sum())
    selected = np.zeros(len(document_frequencies), dtype=np.bool_)
    if posting_count == 0:
        return selected, 1.0
    term_ids = np.flatnonzero(document_frequencies)
    order = np.lexsort(
        (
            term_ids,
            -document_frequencies[term_ids].astype(np.int64),
        )
    )
    ordered_terms = term_ids[order]
    cumulative = np.cumsum(
        document_frequencies[ordered_terms],
        dtype=np.uint64,
    )
    target = math.ceil(target_mass * posting_count)
    count = int(np.searchsorted(cumulative, target)) + 1
    selected[ordered_terms[:count]] = True
    return selected, float(cumulative[count - 1] / posting_count)


def active_terms(
    queries: sparse.csr_matrix,
    selected_terms: np.ndarray,
    query_cut: int,
) -> tuple[list[np.ndarray], list[np.ndarray], np.ndarray]:
    per_query: list[np.ndarray] = []
    residual_per_query: list[np.ndarray] = []
    all_terms: list[np.ndarray] = []

    for query in range(queries.shape[0]):
        start = int(queries.indptr[query])
        end = int(queries.indptr[query + 1])
        ids = queries.indices[start:end]
        values = queries.data[start:end]
        cut = min(query_cut, len(ids))
        if cut == 0:
            active = np.empty(0, dtype=np.int64)
            residual = np.empty(0, dtype=np.int64)
        else:
            positions = np.argsort(-values, kind='stable')[:cut]
            cut_terms = ids[positions]
            accelerated = selected_terms[cut_terms]
            active = cut_terms[accelerated]
            residual = cut_terms[~accelerated]
        active = np.asarray(active, dtype=np.int64)
        residual = np.asarray(residual, dtype=np.int64)
        per_query.append(active)
        residual_per_query.append(residual)
        if len(active):
            all_terms.append(active)
    if not all_terms:
        return (
            per_query,
            residual_per_query,
            np.empty(0, dtype=np.int64),
        )
    return (
        per_query,
        residual_per_query,
        np.unique(np.concatenate(all_terms)),
    )


def retain_row_topk(
    matrix: sparse.csr_matrix,
    count: int,
) -> sparse.csr_matrix:
    data: list[np.ndarray] = []
    indices: list[np.ndarray] = []
    indptr = np.zeros(matrix.shape[0] + 1, dtype=np.int64)

    for row in range(matrix.shape[0]):
        start = int(matrix.indptr[row])
        end = int(matrix.indptr[row + 1])
        values = matrix.data[start:end]
        columns = matrix.indices[start:end]
        if len(values) > count:
            selected = np.argpartition(-values, count - 1)[:count]
            selected = selected[np.argsort(columns[selected])]
            values = values[selected]
            columns = columns[selected]
        data.append(values)
        indices.append(columns)
        indptr[row + 1] = indptr[row] + len(values)
    result = sparse.csr_matrix(
        (
            np.concatenate(data) if data else np.empty(0, np.float32),
            (
                np.concatenate(indices)
                if indices
                else np.empty(0, np.int32)
            ),
            indptr,
        ),
        shape=matrix.shape,
    )
    result.sort_indices()
    return result


def sparse_group_maxima(
    documents: sparse.csr_matrix,
    ordered_documents: np.ndarray,
    block_size: int,
) -> tuple[np.ndarray, sparse.csr_matrix]:
    block_count = math.ceil(len(ordered_documents) / block_size)
    offsets = np.minimum(
        np.arange(block_count + 1, dtype=np.int64) * block_size,
        len(ordered_documents),
    )
    selected = documents[ordered_documents].tocoo(copy=True)
    dimension = int(documents.shape[1])
    columns = np.array(selected.col, dtype=np.int64, copy=True)
    block_rows = np.floor_divide(
        np.array(selected.row, dtype=np.int64, copy=True),
        block_size,
    )
    keys = np.add(
        columns,
        np.multiply(block_rows, dimension),
    )
    order = np.argsort(keys, kind='stable')
    ordered_keys = keys[order]
    starts = np.flatnonzero(
        np.r_[True, ordered_keys[1:] != ordered_keys[:-1]]
    )
    maxima = np.maximum.reduceat(selected.data[order], starts)
    unique_keys = ordered_keys[starts]
    summary_rows = np.floor_divide(unique_keys, dimension)
    summary_columns = np.remainder(unique_keys, dimension)
    summary = sparse.csr_matrix(
        (
            maxima,
            (summary_rows, summary_columns),
        ),
        shape=(block_count, dimension),
    )
    summary.sort_indices()
    return offsets, summary


def sparse_group_centroid_ball(
    documents: sparse.csr_matrix,
    document_norms_squared: np.ndarray,
    ordered_documents: np.ndarray,
    block_size: int,
    centroid_cut: int,
) -> tuple[
    np.ndarray,
    sparse.csr_matrix,
    sparse.csr_matrix,
    np.ndarray,
    np.ndarray,
]:
    block_count = math.ceil(len(ordered_documents) / block_size)
    offsets = np.minimum(
        np.arange(block_count + 1, dtype=np.int64) * block_size,
        len(ordered_documents),
    )
    selected = documents[ordered_documents].tocsr(copy=False)
    coordinates = selected.tocoo(copy=True)
    dimension = int(documents.shape[1])
    columns = np.array(coordinates.col, dtype=np.int64, copy=True)
    block_rows = np.floor_divide(
        np.array(coordinates.row, dtype=np.int64, copy=True),
        block_size,
    )
    keys = np.add(columns, np.multiply(block_rows, dimension))
    order = np.argsort(keys, kind='stable')
    ordered_keys = keys[order]
    starts = np.flatnonzero(
        np.r_[True, ordered_keys[1:] != ordered_keys[:-1]]
    )
    sums = np.add.reduceat(coordinates.data[order], starts)
    unique_keys = ordered_keys[starts]
    summary_rows = np.floor_divide(unique_keys, dimension)
    summary_columns = np.remainder(unique_keys, dimension)
    block_sizes = np.diff(offsets)
    centroid_values = sums / block_sizes[summary_rows]
    full_centroids = sparse.csr_matrix(
        (
            centroid_values,
            (summary_rows, summary_columns),
        ),
        shape=(block_count, dimension),
    )
    full_centroids.sort_indices()
    radii = np.zeros(block_count, dtype=np.float64)
    residual_norms = np.zeros(block_count, dtype=np.float64)
    top_rows: list[np.ndarray] = []
    top_columns: list[np.ndarray] = []
    top_values: list[np.ndarray] = []

    for block in range(block_count):
        start = int(offsets[block])
        end = int(offsets[block + 1])
        centroid = full_centroids.getrow(block)
        centroid_norm_squared = float(centroid.multiply(centroid).sum())
        dots = np.asarray(
            (selected[start:end] @ centroid.transpose()).toarray()
        ).reshape(-1)
        distances_squared = (
            document_norms_squared[ordered_documents[start:end]]
            - 2.0 * dots
            + centroid_norm_squared
        )
        radii[block] = math.sqrt(
            max(0.0, float(distances_squared.max(initial=0.0)))
        )
        values = centroid.data
        columns = centroid.indices
        if len(values) > centroid_cut:
            positions = np.argpartition(
                -np.abs(values),
                centroid_cut - 1,
            )[:centroid_cut]
            positions = positions[np.argsort(columns[positions])]
            values = values[positions]
            columns = columns[positions]
        top_norm_squared = float(np.dot(values, values))
        residual_norms[block] = math.sqrt(
            max(0.0, centroid_norm_squared - top_norm_squared)
        )
        top_rows.append(np.full(len(values), block, dtype=np.int64))
        top_columns.append(columns.astype(np.int64, copy=False))
        top_values.append(values.astype(np.float64, copy=False))
    rows = np.concatenate(top_rows) if top_rows else np.empty(0, np.int64)
    columns = (
        np.concatenate(top_columns) if top_columns else np.empty(0, np.int64)
    )
    values = (
        np.concatenate(top_values) if top_values else np.empty(0, np.float64)
    )
    centroids = sparse.csr_matrix(
        (values, (rows, columns)),
        shape=(block_count, dimension),
    )
    centroids.sort_indices()
    support = centroids.copy()
    support.data.fill(1.0)
    return offsets, centroids, support, residual_norms, radii


def build_term_blocks(
    documents: sparse.csr_matrix,
    documents_csc: sparse.csc_matrix,
    terms: np.ndarray,
    signatures: np.ndarray,
    document_norms_squared: np.ndarray,
    block_size: int,
    centroid_cut: int,
    summary_cut: int,
    bound: str,
    execution: str,
    cohesive: bool,
) -> tuple[dict[int, TermBlocks], dict[str, float]]:
    result: dict[int, TermBlocks] = {}
    started = time.perf_counter()
    summary_entries = 0
    posting_references = 0
    block_count = 0

    for offset, term_id_value in enumerate(terms):
        term_id = int(term_id_value)
        start = int(documents_csc.indptr[term_id])
        end = int(documents_csc.indptr[term_id + 1])
        term_documents = documents_csc.indices[start:end].astype(
            np.int64,
            copy=True,
        )
        if cohesive:
            order = np.lexsort((term_documents, signatures[term_documents]))
            term_documents = term_documents[order]
        else:
            term_documents.sort()
        support = None
        residual_norms = None
        radii = None
        if bound == 'coordinate-max':
            offsets, summaries = sparse_group_maxima(
                documents,
                term_documents,
                block_size,
            )
        else:
            (
                offsets,
                summaries,
                support,
                residual_norms,
                radii,
            ) = sparse_group_centroid_ball(
                documents,
                document_norms_squared,
                term_documents,
                block_size,
                centroid_cut,
            )
        if execution == 'bounded-candidate':
            summaries = retain_row_topk(summaries, summary_cut)
        result[term_id] = TermBlocks(
            documents=term_documents,
            offsets=offsets,
            summaries=summaries,
            summary_support=support,
            centroid_residual_norms=residual_norms,
            radii=radii,
        )
        summary_entries += int(summaries.nnz)
        posting_references += len(term_documents)
        block_count += summaries.shape[0]
        if (offset + 1) % 32 == 0:
            print(
                json.dumps(
                    {
                        'phase': 'build_blocks',
                        'cohesive': cohesive,
                        'completed_terms': offset + 1,
                        'term_count': len(terms),
                    },
                    sort_keys=True,
                ),
                flush=True,
            )
    elapsed = time.perf_counter() - started
    return result, {
        'seconds': elapsed,
        'term_count': len(terms),
        'block_count': block_count,
        'posting_references': posting_references,
        'summary_entries': summary_entries,
        'estimated_summary_bytes': summary_entries * 8 + block_count * 8,
    }


def exact_topk(
    documents: sparse.csr_matrix,
    query: sparse.csr_matrix,
    k: int,
) -> tuple[np.ndarray, np.ndarray, float]:
    scores = np.asarray((documents @ query.transpose()).toarray()).reshape(-1)
    count = min(k, len(scores))
    candidates = np.argpartition(-scores, count - 1)[:count]
    order = np.lexsort((candidates, -scores[candidates]))
    topk = candidates[order]
    return scores, topk, float(scores[topk[-1]])


def evaluate_layout(
    documents: sparse.csr_matrix,
    queries: sparse.csr_matrix,
    per_query_terms: list[np.ndarray],
    blocks: dict[int, TermBlocks],
    k: int,
    bound: str,
) -> dict[str, Any]:
    posting_ratios: list[float] = []
    candidate_counts: list[int] = []
    summary_hit_ratios: list[float] = []
    topk_coverages: list[float] = []
    rows: list[dict[str, Any]] = []

    for query_id in range(queries.shape[0]):
        query = queries.getrow(query_id)
        query_norm_squared = float(query.multiply(query).sum())
        query_norm = math.sqrt(query_norm_squared)
        query_squared = query.copy()
        query_squared.data = np.square(query_squared.data)
        scores, topk, threshold = exact_topk(documents, query, k)
        query_terms = per_query_terms[query_id]
        flat_postings = 0
        visited_postings = 0
        summary_entries = 0
        summary_hits = 0
        visited_blocks = 0
        all_blocks = 0
        candidates: list[np.ndarray] = []

        for term_id_value in query_terms:
            term_id = int(term_id_value)
            term_blocks = blocks[term_id]
            summary_scores = np.asarray(
                (term_blocks.summaries @ query.transpose()).toarray()
            ).reshape(-1)
            if bound == 'coordinate-max':
                upper_bounds = summary_scores
            else:
                if (
                    term_blocks.summary_support is None
                    or term_blocks.centroid_residual_norms is None
                    or term_blocks.radii is None
                ):
                    raise RuntimeError('centroid-ball metadata is incomplete')
                covered_squared = np.asarray(
                    (
                        term_blocks.summary_support
                        @ query_squared.transpose()
                    ).toarray()
                ).reshape(-1)
                query_residual_norms = np.sqrt(
                    np.maximum(0.0, query_norm_squared - covered_squared)
                )
                upper_bounds = (
                    summary_scores
                    + query_residual_norms
                    * term_blocks.centroid_residual_norms
                    + query_norm * term_blocks.radii
                )
            keep = np.flatnonzero(
                upper_bounds + 1e-6 * max(1.0, abs(threshold)) >= threshold
            )
            flat_postings += len(term_blocks.documents)
            all_blocks += term_blocks.summaries.shape[0]
            summary_entries += int(term_blocks.summaries.nnz)
            summary_hits += int(
                term_blocks.summaries[:, query.indices].nnz
            )
            visited_blocks += len(keep)
            for block_id in keep:
                start = int(term_blocks.offsets[block_id])
                end = int(term_blocks.offsets[block_id + 1])
                block_documents = term_blocks.documents[start:end]
                visited_postings += len(block_documents)
                candidates.append(block_documents)
        candidate_documents = (
            np.unique(np.concatenate(candidates))
            if candidates
            else np.empty(0, dtype=np.int64)
        )
        candidate_set = set(int(value) for value in candidate_documents)
        active_topk = 0
        covered_topk = 0
        for document in topk:
            row_start = int(documents.indptr[document])
            row_end = int(documents.indptr[document + 1])
            row_terms = documents.indices[row_start:row_end]
            has_active_term = bool(
                np.intersect1d(
                    row_terms,
                    query_terms,
                    assume_unique=True,
                ).size
            )
            if has_active_term:
                active_topk += 1
                covered_topk += int(document in candidate_set)
        coverage = covered_topk / active_topk if active_topk else 1.0
        posting_ratio = (
            visited_postings / flat_postings if flat_postings else 0.0
        )
        hit_ratio = summary_hits / summary_entries if summary_entries else 0.0
        posting_ratios.append(posting_ratio)
        candidate_counts.append(len(candidate_documents))
        summary_hit_ratios.append(hit_ratio)
        topk_coverages.append(coverage)
        rows.append(
            {
                'query': query_id,
                'active_terms': len(query_terms),
                'kth_score': threshold,
                'blocks': all_blocks,
                'visited_blocks': visited_blocks,
                'flat_postings': flat_postings,
                'visited_postings': visited_postings,
                'posting_ratio': posting_ratio,
                'summary_entries': summary_entries,
                'summary_hits': summary_hits,
                'summary_hit_ratio': hit_ratio,
                'candidate_documents': len(candidate_documents),
                'active_topk_coverage': coverage,
                'exact_topk_score_floor': float(scores[topk[-1]]),
            }
        )
    return {
        'query_count': len(rows),
        'posting_ratio': aggregate(posting_ratios),
        'candidate_documents': aggregate(candidate_counts),
        'summary_hit_ratio': aggregate(summary_hit_ratios),
        'active_topk_coverage': aggregate(topk_coverages),
        'rows': rows,
    }


def residual_documents(
    documents_csc: sparse.csc_matrix,
    terms: np.ndarray,
) -> np.ndarray:
    postings: list[np.ndarray] = []

    for term_id_value in terms:
        term_id = int(term_id_value)
        start = int(documents_csc.indptr[term_id])
        end = int(documents_csc.indptr[term_id + 1])
        postings.append(documents_csc.indices[start:end])
    return (
        np.unique(np.concatenate(postings))
        if postings
        else np.empty(0, dtype=np.int64)
    )


def evaluate_bounded_layout(
    documents: sparse.csr_matrix,
    documents_csc: sparse.csc_matrix,
    queries: sparse.csr_matrix,
    per_query_terms: list[np.ndarray],
    residual_per_query: list[np.ndarray],
    blocks: dict[int, TermBlocks],
    k: int,
    candidate_multiplier: int,
) -> dict[str, Any]:
    overlaps: list[float] = []
    coverages: list[float] = []
    posting_ratios: list[float] = []
    candidate_counts: list[int] = []
    summary_hits: list[int] = []
    rows: list[dict[str, Any]] = []
    candidate_budget = k * candidate_multiplier

    for query_id in range(queries.shape[0]):
        query = queries.getrow(query_id)
        _scores, exact, _threshold = exact_topk(documents, query, k)
        ranked_blocks: list[tuple[float, int, int]] = []
        all_postings = 0
        all_summary_hits = 0

        for term_id_value in per_query_terms[query_id]:
            term_id = int(term_id_value)
            term_blocks = blocks[term_id]
            block_scores = np.asarray(
                (term_blocks.summaries @ query.transpose()).toarray()
            ).reshape(-1)
            all_postings += len(term_blocks.documents)
            all_summary_hits += int(
                term_blocks.summaries[:, query.indices].nnz
            )
            ranked_blocks.extend(
                (float(score), term_id, block_id)
                for block_id, score in enumerate(block_scores)
            )
        ranked_blocks.sort(key=lambda row: (-row[0], row[1], row[2]))
        accelerated: set[int] = set()
        visited_postings = 0
        visited_blocks = 0
        for _score, term_id, block_id in ranked_blocks:
            term_blocks = blocks[term_id]
            start = int(term_blocks.offsets[block_id])
            end = int(term_blocks.offsets[block_id + 1])
            visited_postings += end - start
            visited_blocks += 1
            accelerated.update(
                int(value) for value in term_blocks.documents[start:end]
            )
            if len(accelerated) >= candidate_budget:
                break
        residual = residual_documents(
            documents_csc,
            residual_per_query[query_id],
        )
        candidates = np.union1d(
            np.fromiter(accelerated, dtype=np.int64),
            residual,
        ).astype(np.int64, copy=False)
        candidate_scores = np.asarray(
            (documents[candidates] @ query.transpose()).toarray()
        ).reshape(-1)
        order = np.lexsort((candidates, -candidate_scores))
        ranking = candidates[order[:k]]
        exact_set = set(int(value) for value in exact)
        candidate_set = set(int(value) for value in candidates)
        overlap = len(exact_set & set(int(value) for value in ranking)) / len(
            exact_set
        )
        coverage = len(exact_set & candidate_set) / len(exact_set)
        posting_ratio = (
            visited_postings / all_postings if all_postings else 0.0
        )
        overlaps.append(overlap)
        coverages.append(coverage)
        posting_ratios.append(posting_ratio)
        candidate_counts.append(len(candidates))
        summary_hits.append(all_summary_hits)
        rows.append(
            {
                'query': query_id,
                'accelerated_terms': len(per_query_terms[query_id]),
                'residual_terms': len(residual_per_query[query_id]),
                'visited_blocks': visited_blocks,
                'flat_accelerated_postings': all_postings,
                'visited_accelerated_postings': visited_postings,
                'accelerated_posting_ratio': posting_ratio,
                'summary_hits': all_summary_hits,
                'accelerated_candidates': len(accelerated),
                'residual_candidates': len(residual),
                'union_candidates': len(candidates),
                'exact_topk_candidate_coverage': coverage,
                'topk_overlap': overlap,
            }
        )
    return {
        'query_count': len(rows),
        'candidate_budget': candidate_budget,
        'accelerated_posting_ratio': aggregate(posting_ratios),
        'candidate_documents': aggregate(candidate_counts),
        'summary_hits': aggregate(summary_hits),
        'exact_topk_candidate_coverage': aggregate(coverages),
        'topk_overlap': aggregate(overlaps),
        'rows': rows,
    }


def aggregate(values: list[float | int]) -> dict[str, float]:
    if not values:
        return {'mean': 0.0, 'median': 0.0, 'max': 0.0}
    return {
        'mean': float(statistics.fmean(values)),
        'median': float(statistics.median(values)),
        'max': float(max(values)),
    }


def main() -> int:
    args = parse_args()
    if args.query_limit <= 0 or args.k <= 0 or args.query_cut <= 0:
        raise ValueError('query-limit, k, and query-cut must be positive')
    if (
        args.document_cut <= 0
        or args.block_size <= 0
        or args.centroid_cut <= 0
        or args.summary_cut <= 0
        or args.candidate_multiplier <= 0
    ):
        raise ValueError(
            'document-cut, block-size, and centroid-cut must be positive'
        )
    if not 1 <= args.signature_bits <= 64:
        raise ValueError('signature-bits must be in [1, 64]')
    if not 0.0 < args.accelerated_posting_mass <= 1.0:
        raise ValueError('accelerated-posting-mass must be in (0, 1]')
    if args.execution == 'bounded-candidate' and args.bound != 'coordinate-max':
        raise ValueError('bounded-candidate uses truncated coordinate maxima')

    documents = load_csr(args.documents_npz)
    queries = load_csr(args.queries_npz)[: args.query_limit]
    if documents.shape[1] != queries.shape[1]:
        raise ValueError('document and query dimensions differ')
    if documents.data.size and float(documents.data.min()) < 0.0:
        raise ValueError('safe max summaries require nonnegative documents')
    if queries.data.size and float(queries.data.min()) < 0.0:
        raise ValueError('safe max summaries require nonnegative queries')
    documents_csc = documents.tocsc()
    frequencies = np.diff(documents_csc.indptr).astype(np.uint64)
    selected, actual_mass = select_accelerated_terms(
        frequencies,
        args.accelerated_posting_mass,
    )
    per_query_terms, residual_per_query, terms = active_terms(
        queries,
        selected,
        args.query_cut,
    )
    started = time.perf_counter()
    signatures = document_signatures(
        documents,
        args.document_cut,
        args.signature_bits,
    )
    signature_seconds = time.perf_counter() - started
    document_norms_squared = np.asarray(
        documents.multiply(documents).sum(axis=1)
    ).reshape(-1)
    source_blocks, source_build = build_term_blocks(
        documents,
        documents_csc,
        terms,
        signatures,
        document_norms_squared,
        args.block_size,
        args.centroid_cut,
        args.summary_cut,
        args.bound,
        args.execution,
        cohesive=False,
    )
    cohesive_blocks, cohesive_build = build_term_blocks(
        documents,
        documents_csc,
        terms,
        signatures,
        document_norms_squared,
        args.block_size,
        args.centroid_cut,
        args.summary_cut,
        args.bound,
        args.execution,
        cohesive=True,
    )
    if args.execution == 'exact-bound':
        source = evaluate_layout(
            documents,
            queries,
            per_query_terms,
            source_blocks,
            args.k,
            args.bound,
        )
        cohesive = evaluate_layout(
            documents,
            queries,
            per_query_terms,
            cohesive_blocks,
            args.k,
            args.bound,
        )
    else:
        source = evaluate_bounded_layout(
            documents,
            documents_csc,
            queries,
            per_query_terms,
            residual_per_query,
            source_blocks,
            args.k,
            args.candidate_multiplier,
        )
        cohesive = evaluate_bounded_layout(
            documents,
            documents_csc,
            queries,
            per_query_terms,
            residual_per_query,
            cohesive_blocks,
            args.k,
            args.candidate_multiplier,
        )
    result = {
        'contract': {
            'safe_bound': (
                'sum(q_j * max_{d in B}(x_dj))'
                if args.bound == 'coordinate-max'
                else 'q.c_top + ||q_res|| ||c_res|| + ||q|| radius'
            ),
            'nonnegative_required': True,
            'qrels_used': False,
            'product_lifecycle_changed': False,
        },
        'configuration': {
            'documents': documents.shape[0],
            'dimensions': documents.shape[1],
            'queries': queries.shape[0],
            'k': args.k,
            'query_cut': args.query_cut,
            'document_cut': args.document_cut,
            'block_size': args.block_size,
            'signature_bits': args.signature_bits,
            'centroid_cut': args.centroid_cut,
            'summary_cut': args.summary_cut,
            'candidate_multiplier': args.candidate_multiplier,
            'bound': args.bound,
            'execution': args.execution,
            'accelerated_posting_mass_target': (
                args.accelerated_posting_mass
            ),
            'accelerated_posting_mass_actual': actual_mass,
            'selected_terms': int(selected.sum()),
            'active_selected_terms': len(terms),
        },
        'signature_build_seconds': signature_seconds,
        'source_order': {
            'build': source_build,
            'evaluation': source,
        },
        'cohesive_simhash': {
            'build': cohesive_build,
            'evaluation': cohesive,
        },
        'ratios': {
            'median_posting_work': (
                cohesive[
                    'posting_ratio'
                    if args.execution == 'exact-bound'
                    else 'accelerated_posting_ratio'
                ]['median']
                / source[
                    'posting_ratio'
                    if args.execution == 'exact-bound'
                    else 'accelerated_posting_ratio'
                ]['median']
                if source[
                    'posting_ratio'
                    if args.execution == 'exact-bound'
                    else 'accelerated_posting_ratio'
                ]['median']
                else 0.0
            ),
            'mean_posting_work': (
                cohesive[
                    'posting_ratio'
                    if args.execution == 'exact-bound'
                    else 'accelerated_posting_ratio'
                ]['mean']
                / source[
                    'posting_ratio'
                    if args.execution == 'exact-bound'
                    else 'accelerated_posting_ratio'
                ]['mean']
                if source[
                    'posting_ratio'
                    if args.execution == 'exact-bound'
                    else 'accelerated_posting_ratio'
                ]['mean']
                else 0.0
            ),
            'summary_entries': (
                cohesive_build['summary_entries']
                / source_build['summary_entries']
                if source_build['summary_entries']
                else 0.0
            ),
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(result, indent=2, sort_keys=True) + '\n',
        encoding='utf-8',
    )
    if (
        args.execution == 'exact-bound'
        and source['active_topk_coverage']['mean'] < 1.0
    ):
        raise RuntimeError(
            'source-order safe bound lost active top-k; '
            f'see {args.output}'
        )
    if (
        args.execution == 'exact-bound'
        and cohesive['active_topk_coverage']['mean'] < 1.0
    ):
        raise RuntimeError(
            'cohesive safe bound lost active top-k; '
            f'see {args.output}'
        )
    print(json.dumps(result['ratios'], sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
