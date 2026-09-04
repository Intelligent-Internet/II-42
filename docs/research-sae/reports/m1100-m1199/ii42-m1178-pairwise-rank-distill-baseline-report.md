# M1178 Pairwise Rank-Distillation Baseline

M1178 trains a tiny leave-one-dataset-out pairwise scorer from M1177 pairs,
using only base/direct features.  It evaluates reranking over the M1177 union
candidate set.  This is a signal test, not a full native index result.

Artifacts:

- Script: `scripts/audit_m1178_pairwise_rank_distill_baseline.py`
- JSON: `runs/m1178_pairwise_rank_distill_baseline_v1/pairwise_rank_distill_baseline.json`
- Summary: `runs/m1178_pairwise_rank_distill_baseline_v1/summary.md`

## Result

| View | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| direct_union | 0.663369 | 0.663236 | 0.563780 | 0.680472 | 0.928382 |
| pairwise_rerank | 0.663369 | 0.663201 | 0.562627 | 0.673206 | 0.939139 |
| rerank_minus_direct | +0.000000 | -0.000036 | -0.001153 | -0.007266 | +0.010757 |

The result is not row-floor clean.

## Interpretation

The M1177 pair supervision contains signal, but a shallow pairwise scorer over
base/direct features mostly learns a high-rank MRR tradeoff and hurts MAP/NDCG.
This should not be scaled as a final reranker.

The useful conclusion is negative: if the route continues, it needs a better
model/objective shape than linear pairwise reranking over existing features.
