# M669 Native Reranker Breakthrough Audit

- Recommendation: `try_strict_preserve_topk_native_reranker`
- M604 inputs: `4`
- Positive qrels rows: `13398`

## Gap Shape

| Category | Count | Rate |
| --- | ---: | ---: |
| top100 hit | 2603 | 0.194283 |
| under-ranked | 5840 | 0.435886 |
| candidate miss | 4955 | 0.369831 |

Per dataset:

| Dataset | Positives | Top100 hit | Under-ranked | Candidate miss |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | 80 | 0.587500 | 0.262500 | 0.150000 |
| `nfcorpus` | 12334 | 0.169775 | 0.436598 | 0.393627 |
| `quora` | 52 | 1.000000 | 0.000000 | 0.000000 |
| `webis-touche2020` | 932 | 0.439914 | 0.465665 | 0.094421 |

## Feature Separability

- Selected feature: `bm25_rr`
- Selected query-pair AUC: `0.569258`

| Feature | Query-pair AUC | Pairs |
| --- | ---: | ---: |
| `bm25_rr` | 0.569258 | 110976 |
| `bm25_score` | 0.474186 | 29209 |
| `bm25_zscore` | 0.474186 | 29209 |
| `source_count` | 0.422906 | 67217 |
| `p1_score` | 0.005716 | 79164 |
| `p1_zscore` | 0.005716 | 79164 |
| `p1_rr` | 0.005316 | 110976 |
| `fused_score` | 0.000007 | 67217 |
| `fused_zscore` | 0.000007 | 67217 |

## M605 Sweep Summary

- Tried rows: `28`
- Accepted rows: `0`
- Best Recall@100 delta: `0.013004`
- Best MAP@100 delta: `0.035887`
- Harmed guard datasets: `{"cqadupstack": 27}`

## M611 Reference

- Recommendation: `return_to_score_calibration_low_separability`
- Selected feature: `bm25_rr`
- Selected query-pair AUC: `0.516989`

## Bottom-Slot Oracle

| Preserve top-k | Top100 recall | Candidate recall | Oracle dRecall | Extra hits |
| ---: | ---: | ---: | ---: | ---: |
| 95 | 0.194283 | 0.630169 | 0.094790 | 1270 |
| 98 | 0.194283 | 0.630169 | 0.044260 | 593 |
| 99 | 0.194283 | 0.630169 | 0.024183 | 324 |

## Conclusion

There is enough query-local separability to justify one guarded native reranker, but the signal is narrow.  The selected feature is BM25 reciprocal rank, while fused/P1 scores are effectively anti-separable in the bottom-slot comparison because the target positives are already below the current fused top100.  The next experiment should therefore be a top95-preserving BM25-rescue slot-admission test, not a free global top1000 reranker.  It must pass per-dataset regression gates, especially on cqadupstack.
