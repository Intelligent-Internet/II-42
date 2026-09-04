# II-42 M502 Compressed Surface Retrieval Report

## Summary

M502 turns the M501 output-surface compression signal into a real qrels
retrieval check. It evaluates exact PPLX dense, compressed dense surfaces, BM25,
and dense+BM25 blend on the visible materialized task root.

This is still read-only. It does not train or modify model weights.

Machine and data:

- machine: `spark-1`;
- task: `FiQA2018`;
- docs: 57638;
- queries: 648;
- qrel queries: 648;
- run root:
  `/home/huoju/leask/runs/ii42-m502-compressed-surface-retrieval-v1/fiqa_full`.

## Artifacts

- Script:
  `scripts/research_sae_m502_compressed_surface_retrieval.py`
- JSON:
  `outputs/m502/fiqa_full/m502_compressed_surface_retrieval_fiqa.json`
- Report:
  `outputs/m502/fiqa_full/m502_compressed_surface_retrieval_fiqa.md`

## Result Matrix

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `row_int8_bm25_zblend` | 0.52941 | 0.46841 | 0.82986 | 0.61342 | 0.86873 |
| `exact_dense_bm25_zblend` | 0.52936 | 0.46852 | 0.83244 | 0.61352 | 0.86853 |
| `row_int4_bm25_zblend` | 0.52888 | 0.46808 | 0.83330 | 0.61074 | 0.86131 |
| `exact_dense` | 0.51808 | 0.45889 | 0.83347 | 0.60380 | 1.00000 |
| `row_int4` | 0.51803 | 0.45843 | 0.83806 | 0.60080 | 0.95085 |
| `dense_fp16_output` | 0.51782 | 0.45889 | 0.83347 | 0.60381 | 0.99986 |
| `row_int8` | 0.51753 | 0.45881 | 0.83347 | 0.60370 | 0.99673 |
| `bm25` | 0.23308 | 0.18641 | 0.49255 | 0.29260 | 0.18886 |

## Interpretation

The FiQA qrels result confirms the M501 preservation metrics:

- `row_int8` is effectively dense-equivalent for this task:
  Recall@100 is identical to exact dense, and NDCG@10 differs by only 0.00055.
- `row_int8_bm25_zblend` is also dense+BM25-equivalent:
  it is within noise of exact dense+BM25 across all metrics.
- `row_int4` is more lossy in dense overlap, but qrels metrics remain close on
  FiQA. It should be treated as a second-tier approximate-storage candidate,
  not as the strict default.
- BM25 alone remains far below dense, so the central value is preserving the
  dense surface efficiently, not replacing it with lexical rescue.

## Decision

Promote `row_int8` to the next route:

1. use row-wise int8 as the compressed dense/index surface;
2. verify on full BEIR/MTEB materialized roots once restored;
3. then use this row-int8 preservation gate for model-side compression:
   QAT, weight-only int8/int4, or final-block pruning.

Do not promote raw dimension pruning or raw topK sparsification from M501.
Those lose too much geometry before qrels evaluation.

## Current Limitation

This is only `FiQA2018`, because the visible `spark-1` materialized root
currently contains only that task. The result is strong enough to choose the
next compression family, but not broad enough to claim BEIR/MTEB generality.

Update: M503 synced the full 10-task materialized root from `spark-2` to
`spark-1` and confirmed that `row_int8` remains dense-equivalent across all
visible tasks. The M502 limitation is resolved for dense-only broad validation;
BM25-hybrid broad validation remains a separate M504 step.
