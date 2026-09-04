# M670 BM25 Rescue Slot Admission

- Recommendation: `continue_bm25_rescue_slot_admission`
- Input JSONL files: `4`
- Query count: `452`

## Method

M670 is deterministic and does not train a model.  It preserves the native fused head, reorders only tail candidates after `preserve_top_k`, and uses qrels only for evaluation.  This makes the probe a second-stage scorer test, not a first-stage encoder update.

## Baseline

| Source | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `native_fixed_order` | 0.741534 | 0.413351 | 0.231289 | 0.364802 | 0.543658 |

## Selected Variant

- preserve_top_k: `98`
- admission_feature: `bm25_score`
- status: `accepted`
- failed checks: `[]`
- harmed datasets: `[]`

| Metric | Delta |
| --- | ---: |
| `candidate_upper_bound` | +0.000000 |
| `map_at_100` | +0.000315 |
| `mrr_at_20` | +0.000000 |
| `ndcg_at_10` | +0.000000 |
| `recall_at_100` | +0.004049 |

| Source | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m670_selected` | 0.741534 | 0.417400 | 0.231604 | 0.364802 | 0.543658 |

Per-dataset deltas:

| Dataset | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `nfcorpus` | +0.003545 | +0.000272 | +0.000000 | +0.000000 |
| `quora` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `webis-touche2020` | +0.013982 | +0.001114 | +0.000000 | +0.000000 |

## Sweep Summary

| preserve_top_k | feature | status | dRecall | dMAP | dNDCG | dMRR | harms |
| ---: | --- | --- | ---: | ---: | ---: | ---: | --- |
| 95 | `bm25_rr` | `rejected` | +0.007106 | +0.000655 | +0.000000 | +0.000000 | `['cqadupstack']` |
| 95 | `bm25_score` | `rejected` | +0.007513 | +0.000674 | +0.000000 | +0.000000 | `['cqadupstack']` |
| 95 | `bm25_zscore` | `rejected` | +0.007513 | +0.000674 | +0.000000 | +0.000000 | `['cqadupstack']` |
| 95 | `source_count` | `accepted` | +0.001134 | +0.000162 | +0.000000 | +0.000000 | `[]` |
| 98 | `bm25_rr` | `accepted` | +0.003848 | +0.000309 | +0.000000 | +0.000000 | `[]` |
| 98 | `bm25_score` | `accepted` | +0.004049 | +0.000315 | +0.000000 | +0.000000 | `[]` |
| 98 | `bm25_zscore` | `accepted` | +0.004049 | +0.000315 | +0.000000 | +0.000000 | `[]` |
| 98 | `source_count` | `accepted` | +0.000577 | +0.000101 | +0.000000 | +0.000000 | `[]` |
| 99 | `bm25_rr` | `accepted` | +0.002937 | +0.000207 | +0.000000 | +0.000000 | `[]` |
| 99 | `bm25_score` | `accepted` | +0.002937 | +0.000207 | +0.000000 | +0.000000 | `[]` |
| 99 | `bm25_zscore` | `accepted` | +0.002937 | +0.000207 | +0.000000 | +0.000000 | `[]` |
| 99 | `source_count` | `accepted` | +0.000291 | +0.000050 | +0.000000 | +0.000000 | `[]` |

## Conclusion

The strict BM25 rescue slot-admission probe passed.  This is a valid second-stage signal.  The important boundary is that top95 is too aggressive and harms cqadupstack, while top98 keeps all dataset guards clean.  The next step should scale the selected top98/BM25-score admission rule to a broader native matrix before any richer learned scorer is attempted.
