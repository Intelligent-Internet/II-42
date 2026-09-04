# M1918 OpenSearch Checkpoint Native Closure

Decision: **reject_native_opensearch_parent_promotion**

| Dataset | Surface | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | baseline | 0.370244 | 0.310860 | 0.656042 | 0.456006 | 0.353827 | 0.849637 |
| fiqa | candidate | 0.366495 | 0.307929 | 0.651428 | 0.449572 | 0.352160 | 0.849303 |
| fiqa | delta | -0.003749 | -0.002932 | -0.004614 | -0.006435 | -0.001667 | -0.000334 |
| arguana | baseline | 0.397538 | 0.277422 | 0.988580 | 0.275082 | 0.527645 | 0.999286 |
| arguana | candidate | 0.394717 | 0.275328 | 0.987152 | 0.273097 | 0.526453 | 0.999286 |
| arguana | delta | -0.002821 | -0.002094 | -0.001428 | -0.001985 | -0.001192 | 0.000000 |
| nfcorpus | baseline | 0.344921 | 0.160261 | 0.285311 | 0.566408 | 0.330929 | 0.573762 |
| nfcorpus | candidate | 0.343910 | 0.160273 | 0.284818 | 0.565582 | 0.329938 | 0.578360 |
| nfcorpus | delta | -0.001011 | 0.000012 | -0.000492 | -0.000826 | -0.000991 | 0.004597 |
| scifact | baseline | 0.708953 | 0.675193 | 0.934333 | 0.685412 | 0.404167 | 0.993333 |
| scifact | candidate | 0.706949 | 0.673192 | 0.931000 | 0.681897 | 0.402600 | 0.993333 |
| scifact | delta | -0.002004 | -0.002001 | -0.003333 | -0.003515 | -0.001567 | 0.000000 |

## Gate

```json
{
  "candidate_upper_pass": true,
  "dense_overlap_pass": true,
  "latency_pass": true,
  "macro_quality_pass": false,
  "row_safety_pass": true,
  "storage_pass": true
}
```
