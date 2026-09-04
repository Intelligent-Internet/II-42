from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path
from types import SimpleNamespace

import numpy as np
from scipy import sparse


SCRIPT = (
    Path(__file__).resolve().parents[1]
    / 'scripts'
    / 'benchmark_seismic_npz_oracle.py'
)
SPEC = importlib.util.spec_from_file_location(
    'benchmark_seismic_npz_oracle',
    SCRIPT,
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def save_csr(path: Path, matrix: sparse.csr_matrix) -> None:
    np.savez_compressed(
        path,
        data=matrix.data,
        indices=matrix.indices,
        indptr=matrix.indptr,
        shape=np.asarray(matrix.shape, dtype=np.int64),
    )


def test_posting_cap_uses_global_retained_mass() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [1, 1, 0],
                [1, 0, 1],
                [1, 0, 0],
                [1, 0, 0],
            ],
            dtype=np.float32,
        )
    )

    assert MODULE.posting_cap_for_mass(documents, 0.5) == (1, 0.5)
    assert MODULE.posting_cap_for_mass(documents, 2 / 3) == (2, 2 / 3)
    assert MODULE.posting_cap_for_mass(documents, 1.0) == (4, 1.0)
    assert MODULE.posting_cap_for_mass(documents, 0.5, 3) == (3, 5 / 6)
    assert MODULE.retained_posting_mass(documents, 2) == 2 / 3


def test_empty_posting_surface_has_stable_policy() -> None:
    documents = sparse.csr_matrix((3, 5), dtype=np.float32)

    assert MODULE.posting_cap_for_mass(documents, 0.45) == (1, 1.0)
    assert MODULE.posting_cap_for_mass(documents, 0.45, 1000) == (
        1000,
        1.0,
    )
    assert MODULE.retained_posting_mass(documents, 100) == 1.0


def test_accelerated_terms_cover_smallest_high_df_prefix() -> None:
    documents = sparse.csr_matrix(
        np.asarray(
            [
                [1, 1, 1, 0, 0],
                [1, 0, 1, 0, 1],
                [1, 0, 1, 0, 0],
                [1, 0, 0, 0, 1],
            ],
            dtype=np.float32,
        )
    )

    selected, frequencies, actual_mass = MODULE.select_accelerated_terms(
        documents,
        0.5,
    )

    assert frequencies.tolist() == [4, 1, 3, 0, 2]
    assert np.flatnonzero(selected).tolist() == [0, 2]
    assert actual_mass == 0.7


def test_residual_offsets_union_exact_posting_lists() -> None:
    documents = sparse.csc_matrix(
        np.asarray(
            [
                [1, 1, 1, 0, 0],
                [1, 0, 1, 0, 1],
                [1, 0, 1, 0, 0],
                [1, 0, 0, 0, 1],
            ],
            dtype=np.float32,
        )
    )

    assert MODULE.residual_document_offsets(
        documents,
        np.asarray([1, 4]),
    ).tolist() == [0, 1, 3]
    assert MODULE.residual_document_offsets(
        documents,
        np.asarray([], dtype=np.int64),
    ).tolist() == []


def test_dataset_root_loads_shards_and_applies_limits(
    tmp_path: Path,
) -> None:
    documents = tmp_path / 'documents'
    documents.mkdir()
    first = sparse.csr_matrix(
        np.asarray([[1, 0, 2], [0, 3, 0]], dtype=np.float32)
    )
    second = sparse.csr_matrix(
        np.asarray([[4, 0, 0], [0, 0, 5]], dtype=np.float32)
    )
    queries = sparse.csr_matrix(
        np.asarray([[1, 0, 1], [0, 1, 0]], dtype=np.float32)
    )
    save_csr(documents / 'first.npz', first)
    save_csr(documents / 'second.npz', second)
    save_csr(tmp_path / 'queries.npz', queries)
    (documents / 'first.ids').write_text('d0\nd1\n')
    (documents / 'second.ids').write_text('d2\nd3\n')
    (tmp_path / 'query.ids').write_text('q0\nq1\n')
    (tmp_path / 'qrels.json').write_text(
        json.dumps({'q0': {'d0': 1}, 'q1': {'d1': 1}})
    )
    (tmp_path / 'manifest.json').write_text(
        json.dumps(
            {
                'documents': {
                    'shards': [
                        {
                            'matrix': 'documents/first.npz',
                            'ids': 'documents/first.ids',
                        },
                        {
                            'matrix': 'documents/second.npz',
                            'ids': 'documents/second.ids',
                        },
                    ]
                },
                'queries': {
                    'matrix': 'queries.npz',
                    'ids': 'query.ids',
                    'qrels': 'qrels.json',
                },
            }
        )
    )

    loaded = MODULE.load_dataset_root(tmp_path, 3, 1)

    assert loaded[0].toarray().tolist() == [
        [1.0, 0.0, 2.0],
        [0.0, 3.0, 0.0],
        [4.0, 0.0, 0.0],
    ]
    assert loaded[1].toarray().tolist() == [[1.0, 0.0, 1.0]]
    assert loaded[2] == ['d0', 'd1', 'd2']
    assert loaded[3] == ['q0']
    assert loaded[4] == {'q0': {'d0': 1.0}, 'q1': {'d1': 1.0}}
    assert len(loaded[5]) == 64
    assert len(loaded[6]) == 64


def test_build_index_reuses_existing_artifact(
    tmp_path: Path,
    monkeypatch,
) -> None:
    artifact = tmp_path / 'index.index.seismic'
    artifact.write_bytes(b'reusable')
    loaded = object()

    class FakeIndex:
        @staticmethod
        def load(path: str):
            assert path == str(artifact)
            return loaded

        @staticmethod
        def build(*_args, **_kwargs):
            raise AssertionError('reuse must not rebuild the index')

    monkeypatch.setitem(
        sys.modules,
        'seismic',
        SimpleNamespace(
            SeismicIndex=FakeIndex,
            SeismicIndexDotVByte=FakeIndex,
        ),
    )
    args = SimpleNamespace(
        index_variant='standard',
        run_dir=tmp_path,
        reuse_index=True,
    )

    index, elapsed, size = MODULE.build_index(args, tmp_path / 'unused')

    assert index is loaded
    assert elapsed >= 0.0
    assert size == len(b'reusable')
