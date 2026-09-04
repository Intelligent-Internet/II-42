from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import numpy as np
from scipy import sparse


SCRIPTS = Path(__file__).resolve().parents[1] / 'scripts'
sys.path.insert(0, str(SCRIPTS))
SCRIPT = SCRIPTS / 'benchmark_retained_impact_oracle.py'
SPEC = importlib.util.spec_from_file_location(
    'benchmark_retained_impact_oracle',
    SCRIPT,
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_global_threshold_preserves_budget_and_per_list_cap() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [10, 1, 8],
                [9, 7, 0],
                [0, 5, 6],
            ],
            dtype=np.float32,
        )
    )
    selected = np.ones(3, dtype=np.bool_)

    retained, _tails, counts, threshold = (
        MODULE.build_global_threshold_matrix(
            documents,
            selected,
            retained_count=1,
            max_fraction=2.0,
        )
    )

    assert retained.nnz == 3
    assert counts.tolist() == [2, 0, 1]
    assert threshold == 8.0
    assert retained.toarray().tolist() == [
        [10.0, 0.0, 8.0],
        [9.0, 0.0, 0.0],
        [0.0, 0.0, 0.0],
    ]


def test_global_threshold_trims_boundary_ties_deterministically() -> None:
    documents = sparse.csr_matrix(
        np.ones((3, 2), dtype=np.float32),
    )
    selected = np.ones(2, dtype=np.bool_)

    retained, _tails, counts, threshold = (
        MODULE.build_global_threshold_matrix(
            documents,
            selected,
            retained_count=1,
            max_fraction=2.0,
        )
    )

    assert retained.nnz == 2
    assert counts.tolist() == [0, 2]
    assert threshold == 1.0


def test_work_ratio_uses_actual_retained_list_lengths() -> None:
    queries = sparse.csr_matrix(
        np.asarray([[1, 1, 1]], dtype=np.float32),
    )
    frequencies = np.asarray([10, 20, 30], dtype=np.int64)
    selected = np.asarray([True, False, True])
    retained = np.asarray([2, 0, 6], dtype=np.int64)

    assert MODULE.work_ratios(
        queries,
        frequencies,
        selected,
        retained,
    ) == [28 / 60]
