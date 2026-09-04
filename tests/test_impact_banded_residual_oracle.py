from __future__ import annotations

import importlib.util
from pathlib import Path
import sys

import numpy as np
from scipy import sparse


SCRIPT = (
    Path(__file__).resolve().parents[1]
    / 'scripts'
    / 'benchmark_impact_banded_residual_oracle.py'
)
SPEC = importlib.util.spec_from_file_location(
    'benchmark_impact_banded_residual_oracle',
    SCRIPT,
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


def make_term(term_id: int, weight: float, rows, impacts):
    postings = MODULE.PostingList(
        documents=np.asarray(rows, dtype=np.int64),
        impacts=np.asarray(impacts, dtype=np.float64),
    )
    ordered = MODULE.impact_order(postings)
    return MODULE.TermBlock(
        term_id=term_id,
        query_weight=weight,
        documents=ordered.documents,
        impacts=ordered.impacts,
    )


def test_exact_global_frontier_matches_full_union() -> None:
    scores = np.asarray([0.9, 0.7, 0.6, 0.4, 0.2], dtype=np.float64)
    terms = [
        make_term(1, 1.0, [0, 2, 4], [0.9, 0.6, 0.2]),
        make_term(2, 1.0, [1, 3], [0.7, 0.4]),
    ]
    baseline_state = MODULE.ExactTopK(scores, 2)
    baseline = MODULE.full_residual_union(baseline_state, terms)
    frontier_state = MODULE.ExactTopK(scores, 2)
    frontier = MODULE.global_impact_frontier(frontier_state, terms, 0.0)

    assert np.array_equal(frontier.top_documents, baseline.top_documents)
    assert frontier.decoded_postings < baseline.decoded_postings


def test_exact_block_frontiers_match_full_union() -> None:
    scores = np.asarray(
        [0.9, 0.8, 0.3, 0.2, 0.7, 0.6, 0.1, 0.05],
        dtype=np.float64,
    )
    terms = [
        make_term(1, 1.0, [0, 2, 4, 6], [0.9, 0.3, 0.7, 0.1]),
        make_term(2, 1.0, [1, 3, 5, 7], [0.8, 0.2, 0.6, 0.05]),
    ]
    blocks = MODULE.query_blocks(terms, 2)
    baseline_state = MODULE.ExactTopK(scores, 2)
    baseline = MODULE.full_residual_union(baseline_state, terms)

    block_state = MODULE.ExactTopK(scores, 2)
    block = MODULE.block_only_frontier(block_state, blocks, 0.0)
    impact_state = MODULE.ExactTopK(scores, 2)
    impact = MODULE.block_impact_frontier(impact_state, blocks, 0.0)

    assert np.array_equal(block.top_documents, baseline.top_documents)
    assert np.array_equal(impact.top_documents, baseline.top_documents)
    assert impact.decoded_postings <= block.decoded_postings


def test_upward_u8_bounds_preserve_exact_lazy_topk() -> None:
    scores = np.asarray([0.91, 0.72, 0.63, 0.54, 0.45], dtype=np.float64)
    terms = [
        make_term(0, 0.71, [0, 1, 3], [0.83, 0.61, 0.19]),
        make_term(1, 0.43, [1, 2, 4], [0.92, 0.47, 0.11]),
    ]
    bounded = MODULE.query_blocks(terms, 2, impact_bits=8)
    for original in terms:
        term_bounds = [
            block
            for block_terms in bounded.values()
            for block in block_terms
            if block.term_id == original.term_id
        ]
        by_document = {
            int(document): float(impact)
            for block in term_bounds
            for document, impact in zip(
                block.documents,
                block.impacts,
                strict=True,
            )
        }
        for document, impact in zip(
            original.documents,
            original.impacts,
            strict=True,
        ):
            assert by_document[int(document)] >= float(impact)

    state = MODULE.ExactTopK(scores, 2)
    result = MODULE.block_impact_batched_frontier(
        state,
        bounded,
        0.0,
        16,
    )
    assert np.array_equal(result.top_documents, MODULE.topk(scores, 2))


def test_quantized_sole_authorities_preserve_support() -> None:
    impacts = np.asarray([1.0, 0.2, 1e-5], dtype=np.float64)

    for bits in (8, 16):
        quantized = MODULE.quantize_impacts_nearest(impacts, bits)

        assert np.all(np.isfinite(quantized))
        assert np.all(quantized > 0.0)
        assert quantized.shape == impacts.shape


def test_fixed_u8_authority_is_value_local_and_idempotent() -> None:
    impacts = np.asarray(
        [1e-6, 1.0 / 256.0, 0.05, 0.125, 1.234, 5.0, 100.0],
        dtype=np.float64,
    )

    quantized = MODULE.quantize_impacts_fixed_u8(impacts)
    requantized = MODULE.quantize_impacts_fixed_u8(quantized)

    assert np.array_equal(quantized, requantized)
    assert np.all(quantized > 0.0)
    assert quantized[0] == 2.0 / 255.0
    assert quantized[-1] == 2.0


def test_quantized_sole_authority_scores() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [1.0, 0.0],
                [0.5, 0.5],
                [0.0, 1.0],
            ],
            dtype=np.float64,
        )
    )
    query_ids = np.asarray([0, 1], dtype=np.int64)
    query_weights = np.asarray([1.0, 0.5], dtype=np.float64)

    for bits in (8, 16):
        scores = MODULE.sole_authority_scores(
            documents.tocsc(),
            query_ids,
            query_weights,
            bits,
        )

        assert np.array_equal(MODULE.topk(scores, 3), np.asarray([0, 1, 2]))


def test_error_ratio_has_explicit_score_bound() -> None:
    scores = np.asarray([1.0, 0.99, 0.8, 0.79], dtype=np.float64)
    terms = [make_term(1, 1.0, [0, 1, 2, 3], scores)]
    exact_state = MODULE.ExactTopK(scores, 1)
    exact = MODULE.global_impact_frontier(exact_state, terms, 0.0)
    bounded_state = MODULE.ExactTopK(scores, 1)
    bounded = MODULE.global_impact_frontier(bounded_state, terms, 0.25)

    assert np.array_equal(exact.top_documents, np.asarray([0]))
    assert np.array_equal(bounded.top_documents, np.asarray([0]))
    assert bounded.decoded_postings <= exact.decoded_postings


def test_product_seed_caps_each_selected_term() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [1.0, 0.0],
                [0.9, 0.1],
                [0.8, 0.2],
                [0.0, 1.0],
            ],
            dtype=np.float64,
        )
    )
    columns = documents.tocsc()
    selected = np.asarray([True, True], dtype=np.bool_)
    seed = MODULE.selected_seed_documents(
        columns,
        documents,
        np.asarray([0, 1], dtype=np.int64),
        np.asarray([1.0, 1.0], dtype=np.float64),
        selected,
        1.0,
        1,
        2,
    )

    assert np.array_equal(seed, np.asarray([0, 3]))


def test_publication_shape_compares_current_authority() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [1.0, 0.0],
                [0.8, 0.4],
                [0.0, 0.3],
                [0.2, 0.0],
            ],
            dtype=np.float64,
        )
    )
    shape = MODULE.publication_shape(
        documents.tocsc(),
        np.asarray([False, False], dtype=np.bool_),
        8,
    )

    assert shape['current_packed_estimate']['bytes'] > 0
    assert shape['compact_f32']['total_bytes'] > 0
    assert (
        shape['compact_f16']['total_bytes']
        < shape['compact_f32']['total_bytes']
    )
    assert (
        shape['compact_u8']['posting_width']
        < shape['compact_f16']['posting_width']
    )


def test_lazy_frontiers_reduce_exact_forward_resolution() -> None:
    scores = np.asarray(
        [1.0, 0.9, 0.8, 0.2, 0.1, 0.05],
        dtype=np.float64,
    )
    terms = [
        make_term(1, 1.0, [0, 2, 4], [1.0, 0.8, 0.1]),
        make_term(2, 1.0, [1, 3, 5], [0.9, 0.2, 0.05]),
    ]
    eager_state = MODULE.ExactTopK(scores, 2)
    eager = MODULE.global_impact_frontier(eager_state, terms, 0.0)
    lazy_state = MODULE.ExactTopK(scores, 2)
    lazy = MODULE.global_impact_lazy_frontier(lazy_state, terms, 0.0)
    blocks = MODULE.query_blocks(terms, 2)
    block_state = MODULE.ExactTopK(scores, 2)
    block_lazy = MODULE.block_impact_lazy_frontier(
        block_state,
        blocks,
        0.0,
    )

    assert np.array_equal(lazy.top_documents, eager.top_documents)
    assert np.array_equal(block_lazy.top_documents, eager.top_documents)
    assert lazy.admitted_documents <= eager.admitted_documents
    assert block_lazy.admitted_documents <= eager.admitted_documents

    for batch in (16, 0):
        batch_state = MODULE.ExactTopK(scores, 2)
        batch_lazy = MODULE.block_impact_batched_frontier(
            batch_state,
            blocks,
            0.0,
            batch,
        )
        assert np.array_equal(batch_lazy.top_documents,
                              eager.top_documents)


def test_document_order_continues_after_a_skippable_block() -> None:
    scores = np.asarray([0.5, 0.1, 1.0], dtype=np.float64)
    terms = [make_term(1, 1.0, [1, 2], [0.1, 1.0])]
    state = MODULE.ExactTopK(scores, 1)
    state.offer(0)

    result = MODULE.block_impact_batched_frontier(
        state,
        MODULE.query_blocks(terms, 1),
        0.0,
        1,
        order_by_upper_bound=False,
    )

    assert np.array_equal(result.top_documents, np.asarray([2]))


def test_randomized_lazy_frontiers_are_exact() -> None:
    rng = np.random.default_rng(42)
    for _case in range(100):
        document_count = 24
        scores = np.zeros(document_count, dtype=np.float64)
        terms = []
        for term_id in range(1, 7):
            selected = np.flatnonzero(rng.random(document_count) < 0.35)
            if selected.size == 0:
                continue
            impacts = rng.random(selected.size) + 0.01
            weight = float(rng.random() + 0.1)
            scores[selected] += impacts * weight
            terms.append(make_term(term_id, weight, selected, impacts))
        baseline_state = MODULE.ExactTopK(scores, 5)
        baseline = MODULE.full_residual_union(baseline_state, terms)
        global_state = MODULE.ExactTopK(scores, 5)
        global_lazy = MODULE.global_impact_lazy_frontier(
            global_state,
            terms,
            0.0,
        )
        block_state = MODULE.ExactTopK(scores, 5)
        block_lazy = MODULE.block_impact_lazy_frontier(
            block_state,
            MODULE.query_blocks(terms, 2),
            0.0,
        )
        batched_state = MODULE.ExactTopK(scores, 5)
        batched_lazy = MODULE.block_impact_batched_frontier(
            batched_state,
            MODULE.query_blocks(terms, 2),
            0.0,
            0,
        )
        document_order_state = MODULE.ExactTopK(scores, 5)
        document_order = MODULE.block_impact_batched_frontier(
            document_order_state,
            MODULE.query_blocks(terms, 2),
            0.0,
            16,
            order_by_upper_bound=False,
        )
        hierarchical_state = MODULE.ExactTopK(scores, 5)
        hierarchical = MODULE.hierarchical_block_impact_frontier(
            hierarchical_state,
            MODULE.query_blocks(terms, 1),
            1,
            3,
            0.0,
            16,
        )

        assert np.array_equal(global_lazy.top_documents,
                              baseline.top_documents)
        assert np.array_equal(block_lazy.top_documents,
                              baseline.top_documents)
        assert np.array_equal(batched_lazy.top_documents,
                              baseline.top_documents)
        assert np.array_equal(document_order.top_documents,
                              baseline.top_documents)
        assert np.array_equal(hierarchical.top_documents,
                              baseline.top_documents)
