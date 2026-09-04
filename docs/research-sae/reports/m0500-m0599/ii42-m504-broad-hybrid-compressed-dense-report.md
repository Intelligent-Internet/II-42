# II-42 M504 Broad Hybrid Compressed Dense Report

## Summary

M504 validates the M500-series compressed dense surface after BM25 z-blending on
the full 10-task materialized retrieval root. This stage is read-only: it does
not train, fine-tune, or modify model weights.

The result confirms the current mainline:

- `row_int8` remains equivalent to exact dense in dense-only retrieval;
- `row_int8_bm25_zblend` remains equivalent to `exact_dense_bm25_zblend`;
- `row_int4` remains close on qrels metrics, but its overlap loss is too large
  to promote as the default dense-faithful surface;
- BM25 improves the hybrid frontier, but the dense component should stay
  row-int8 if compression is required.

## Config

- Task root:
  `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`
- Evaluation machine: `spark-1`
- Run root:
  `/home/huoju/leask/runs/ii42-m504-broad-hybrid-compressed-dense-v1/full10`
- Tasks: 10
- Dense topK: 1000
- BM25 zblend alpha: 0.1
- Candidate surfaces:
  `exact_dense`, `row_int8`, `row_int4`, `bm25`,
  `exact_dense_bm25_zblend`, `row_int8_bm25_zblend`,
  `row_int4_bm25_zblend`

## Artifacts

- Script:
  `scripts/research_sae_m504_broad_hybrid_compressed_dense.py`
- Full JSON:
  `outputs/m504/full10/m504_broad_hybrid_compressed_dense_full10.json`
- Full report:
  `outputs/m504/full10/m504_broad_hybrid_compressed_dense_full10.md`
- Smoke JSON:
  `outputs/m504/smoke_arguana_fiqa/m504_broad_hybrid_compressed_dense_smoke.json`
- Smoke report:
  `outputs/m504/smoke_arguana_fiqa/m504_broad_hybrid_compressed_dense_smoke.md`

## Macro Result

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 | Hybrid O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `row_int4_bm25_zblend` | 0.59073 | 0.44760 | 0.75491 | 0.66433 | 0.86781 | 0.95471 |
| `exact_dense_bm25_zblend` | 0.59061 | 0.44781 | 0.75511 | 0.66395 | 0.87680 | 1.00000 |
| `row_int8_bm25_zblend` | 0.59052 | 0.44793 | 0.75495 | 0.66408 | 0.87682 | 0.99687 |
| `row_int4` | 0.57364 | 0.43548 | 0.74780 | 0.65187 | 0.94834 | 0.00000 |
| `row_int8` | 0.57270 | 0.43543 | 0.74799 | 0.65188 | 0.99664 | 0.00000 |
| `exact_dense` | 0.57268 | 0.43538 | 0.74805 | 0.65183 | 1.00000 | 0.00000 |
| `bm25` | 0.40515 | 0.28117 | 0.58537 | 0.47993 | 0.28113 | 0.00000 |

## Against Exact Dense and Exact Hybrid

Against `exact_dense`:

| Source | Avg NDCG Delta | Min NDCG Delta | Max NDCG Delta | Avg Dense O@100 |
| --- | ---: | ---: | ---: | ---: |
| `row_int8` | 0.000024 | -0.00055 | 0.00082 | 0.996636 |
| `row_int4` | 0.000964 | -0.00130 | 0.00895 | 0.948340 |

Against `exact_dense_bm25_zblend`:

| Source | Avg NDCG Delta | Min NDCG Delta | Max NDCG Delta | Avg Dense O@100 | Avg Hybrid O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `row_int8_bm25_zblend` | -0.000089 | -0.00133 | 0.00055 | 0.876820 | 0.996868 |
| `row_int4_bm25_zblend` | 0.000116 | -0.00232 | 0.00230 | 0.867806 | 0.954708 |

## Interpretation

`row_int8_bm25_zblend` passes the hybrid gate. Its macro NDCG delta against
exact hybrid is effectively zero, and Hybrid O@100 remains 0.99687. This means
row-wise int8 is not only a dense-only storage trick; it is also safe as the
compressed dense component in the current hybrid retrieval pipeline.

`row_int4_bm25_zblend` is not promoted as the default. Its qrels metrics are
close and macro NDCG is slightly above exact hybrid in this run, but Hybrid
O@100 falls to 0.95471. That is an approximate-tier signal, not a strict
dense-faithfulness signal.

BM25 alone remains far below the dense and hybrid surfaces. The BM25 blend is
useful as a retrieval layer, but it should not be used to excuse a compressed
dense surface that fails dense-overlap gates.

## Decision

Promote row-wise int8 as the default compressed dense surface for the current
PPLX-derived route:

1. dense-only retrieval can use `row_int8` as an exact-dense replacement;
2. hybrid retrieval can use `row_int8_bm25_zblend` as an exact-hybrid
   replacement;
3. future model-side compression must reproduce the row-int8 dense surface
   before posting or ranking optimization begins;
4. row-int4 remains a lower-bit or rerank-tier probe, not the default.

## Next Stage

M505 should move from output/index compression to model-side compression:

- bf16 PPLX baseline;
- weight-only int8 PPLX;
- weight-only int4 PPLX as a lower-bound probe;
- activation-aware or QAT repair if plain weight quantization fails;
- row-int8 output equivalence as the promotion gate.

Only after M505 passes should M506 start the PPLX-initialized posting adapter
or text-to-posting transfer stage.
