# II-42 M503 Broad Compressed Dense Report

## Summary

M503 validates the M501/M502 row-wise quantization route on the full visible
materialized retrieval root. It intentionally excludes BM25 and evaluates only
dense-preservation quality:

- `exact_dense`;
- `dense_fp16_output`;
- `row_int8`;
- `row_int4`.

This stage uses GPU-batched retrieval on `spark-1`. It does not train or modify
model weights.

Data:

- root:
  `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`;
- tasks: 10;
- source machine for data sync: `spark-2`;
- evaluation machine: `spark-1`;
- run root:
  `/home/huoju/leask/runs/ii42-m503-broad-compressed-dense-v1/full10`.

## Artifacts

- Script:
  `scripts/research_sae_m503_broad_compressed_dense.py`
- JSON:
  `outputs/m503/full10/m503_broad_compressed_dense_full10.json`
- Report:
  `outputs/m503/full10/m503_broad_compressed_dense_full10.md`

## Macro Result

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `row_int4` | 0.57364 | 0.43548 | 0.74780 | 0.65187 | 0.94834 |
| `row_int8` | 0.57270 | 0.43543 | 0.74799 | 0.65188 | 0.99664 |
| `exact_dense` | 0.57268 | 0.43538 | 0.74805 | 0.65183 | 1.00000 |
| `dense_fp16_output` | 0.57268 | 0.43538 | 0.74805 | 0.65183 | 1.00000 |

Against `exact_dense`:

| Source | Avg NDCG Delta | Worst NDCG Delta | Avg Dense O@100 | Worst Dense O@100 |
| --- | ---: | ---: | ---: | ---: |
| `dense_fp16_output` | 0.000000 | 0.000000 | 1.000000 | 1.000000 |
| `row_int8` | 0.000024 | -0.000550 | 0.996636 | 0.995700 |
| `row_int4` | 0.000964 | -0.001300 | 0.948340 | 0.935050 |

## Interpretation

`row_int8` is the clean promotion candidate:

- qrels metrics are indistinguishable from exact dense across the 10 tasks;
- Dense O@100 stays above 0.995 on every task;
- it gives roughly 4x output/index storage compression versus fp32;
- it is a faithful dense-preservation route, not a BM25 rescue route.

`row_int4` is interesting but not strict enough:

- its qrels metrics are surprisingly close and macro NDCG is slightly higher,
  but this is not evidence of better semantic preservation;
- Dense O@100 falls to roughly 0.948 macro and 0.935 worst-task;
- it may be useful as an approximate/rerank storage tier, not as the default
  dense-faithful surface.

`dense_fp16_output` is exactly stable but only 2x compressed. It is a storage
baseline, not the strongest index compression candidate.

## Decision

Promote `row_int8` as the M500-series mainline:

1. use row-wise int8 as the compressed dense/index surface;
2. use exact dense and row-int8 equivalence as the gate for future model-side
   compression;
3. do not spend more effort on raw dimension pruning or layer trimming;
4. keep `row_int4` as a secondary approximate tier only if a two-stage rerank
   design is needed.

## Next Stage

M504 should test the hybrid surface:

- exact dense + BM25;
- row-int8 + BM25;
- row-int4 + BM25;
- existing M396 structural route if directly comparable.

The goal is to verify that row-int8 remains dense-equivalent after BM25 blend
and becomes the default compressed dense component for retrieval pipelines.
