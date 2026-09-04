# M1258 Movement-Constrained Reserve Replay

## Goal

M1257 found that reserve1 harm is associated with large document movement:

- `top100_new >= 3`
- `top10_new > 0`

M1258 tests exactly these movement constraints as a bounded source-construction
replay.  This is not a broad threshold grid.

## Runs

Smoke:

- `runs/m1258_movement_constrained_reserve_smoke_v1/`
- Datasets:
  `cqadupstack`, `scidocs`, `webis-touche2020`

Full `shared15`:

- JSON:
  `runs/m1258_movement_constrained_reserve_v1/m1258_movement_constrained_reserve.json`
- Markdown:
  `runs/m1258_movement_constrained_reserve_v1/m1258_movement_constrained_reserve.md`
- Query count: `1342`

## Full Shared15 Result

| Variant | Accepted | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1258_no_top10_or_large100_s1` | 0.8770 | 8.835 | 0 | +0.001566 | +0.003491 | +0.003663 | +0.003016 | +0.000155 | +0.031815 |
| `m1258_no_top10_new_s1` | 0.9307 | 8.888 | 0 | +0.001555 | +0.003481 | +0.003671 | +0.003007 | +0.000151 | +0.031725 |
| `m1258_no_large100_s1` | 0.9322 | 8.890 | 0 | +0.001573 | +0.003551 | +0.003308 | +0.003006 | +0.000269 | +0.031413 |
| `m1258_reserve1_s1` | 1.0000 | 8.958 | 0 | +0.001562 | +0.003536 | +0.003268 | +0.002995 | +0.000092 | +0.031034 |
| `m1258_reserve0_s1` | 0.0000 | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |

## Interpretation

This is the first movement-aware source-construction result that improves over
both adjacent baselines:

- beats true `reserve0_s1`
- beats unconstrained `reserve1_s1`
- keeps all five macro metrics positive
- restores most of the NDCG/MRR/CUB loss introduced by unconstrained reserve1
- preserves the reserve1 Recall/MAP gain

The best variant is:

`m1258_no_top10_or_large100_s1`

Policy:

- start from true reserve0
- add one low-tail reserve atom only if reserve1 does not introduce a new top10
  document and does not introduce three or more new top100 documents

This is a qrels-free document movement constraint.  It uses native query-time
behavior, not qrels or dataset-specific tuning.

## Comparison

Against `reserve0_s1`:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.000167 |
| MAP@100 | +0.000155 |
| NDCG@10 | +0.000037 |
| MRR@20 | -0.000110 |
| CUB | +0.000040 |
| Score | +0.001195 |

Against unconstrained `reserve1_s1`:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.000004 |
| MAP@100 | -0.000045 |
| NDCG@10 | +0.000394 |
| MRR@20 | +0.000021 |
| CUB | +0.000063 |
| Score | +0.000781 |

## Decision

Promote `m1258_no_top10_or_large100_s1` to the next candidate for stability
audit.

Do not call it default yet.  It still needs:

1. per-dataset stability
2. query-level negative count comparison against reserve0/reserve1
3. confirmation that gains are not concentrated in one surface

## Next Step

Run M1259 stability audit:

- compare `reserve0_s1`, `reserve1_s1`, and
  `no_top10_or_large100_s1`
- output per-dataset deltas
- output query-level negative counts
- locate worst rows

If M1259 shows row/dataset stability improves, this becomes the first
movement-aware candidate worth broader/native engineering validation.
