from __future__ import annotations

import importlib.util
from pathlib import Path

import numpy as np
from scipy import sparse


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'benchmark_centered_exception_oracle',
    ROOT / 'scripts' / 'benchmark_centered_exception_oracle.py',
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_best_baseline_prefers_dense_nonzero_cluster() -> None:
    baseline, coverage = MODULE.best_baseline(
        np.asarray([0.48, 0.5, 0.51, 0.52], dtype=np.float32),
        5,
        0.025,
    )

    assert 0.49 <= baseline <= 0.51
    assert coverage == 4


def test_centered_exceptions_include_missing_correction() -> None:
    documents = sparse.csr_matrix(
        np.asarray([[0.5], [0.52], [0.0]], dtype=np.float32)
    )

    matrix, baselines, tolerances, counts = (
        MODULE.build_centered_exceptions(
            documents,
            np.asarray([True]),
            0.05,
        )
    )

    assert baselines[0] > tolerances[0]
    assert counts[0] == 1
    assert matrix.toarray()[2, 0] == -baselines[0]


def test_query_work_uses_exception_count_for_selected_terms() -> None:
    queries = sparse.csr_matrix(
        np.asarray([[1.0, 1.0]], dtype=np.float32)
    )
    ratios = MODULE.query_work_ratios(
        queries,
        np.asarray([100, 50]),
        np.asarray([True, False]),
        np.asarray([10, 0]),
    )

    assert ratios == [0.4]
