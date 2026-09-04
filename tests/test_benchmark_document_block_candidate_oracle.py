from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
from scipy import sparse

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))

from benchmark_document_block_candidate_oracle import (  # noqa: E402
    accumulate_block_maxima,
    block_candidates,
)


def test_accumulate_block_maxima_preserves_cross_term_bounds() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [1.0, 0.0],
                [0.5, 2.0],
                [0.0, 3.0],
                [4.0, 1.0],
                [2.0, 0.0],
            ],
            dtype=np.float32,
        )
    )
    scores, postings, refs = accumulate_block_maxima(
        documents.tocsc(),
        np.asarray([0, 1], dtype=np.int32),
        np.asarray([2.0, 1.0], dtype=np.float32),
        block_count=3,
        block_shift=1,
    )

    assert postings == 7
    assert refs == 5
    np.testing.assert_allclose(scores, [4.0, 11.0, 4.0])


def test_block_candidates_returns_complete_physical_blocks() -> None:
    candidates, selected = block_candidates(
        np.asarray([1.0, 5.0, 3.0], dtype=np.float32),
        document_count=10,
        target_documents=5,
        block_shift=2,
    )

    assert selected == 2
    assert set(candidates) == set(range(4, 10))
