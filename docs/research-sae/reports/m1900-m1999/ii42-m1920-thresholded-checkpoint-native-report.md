# M1920 Fixed-Impact Checkpoint Native Closure

Decision: **reject_thresholded_checkpoint_promotion**

Global qrels-free impact floor: `0.05`.

| Dataset | Surface | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | baseline | 0.370244 | 0.310860 | 0.656042 | 0.456006 | 0.353827 | 0.849637 |
| fiqa | raw_checkpoint | 0.366495 | 0.307929 | 0.651428 | 0.449572 | 0.352160 | 0.849303 |
| fiqa | thresholded_checkpoint | 0.367226 | 0.308999 | 0.650105 | 0.451613 | 0.352052 | 0.851154 |
| fiqa | delta | -0.003018 | -0.001862 | -0.005936 | -0.004393 | -0.001775 | 0.001517 |
| arguana | baseline | 0.397538 | 0.277422 | 0.988580 | 0.275082 | 0.527645 | 0.999286 |
| arguana | raw_checkpoint | 0.394717 | 0.275328 | 0.987152 | 0.273097 | 0.526453 | 0.999286 |
| arguana | thresholded_checkpoint | 0.394849 | 0.275742 | 0.987152 | 0.273447 | 0.526253 | 0.999286 |
| arguana | delta | -0.002689 | -0.001680 | -0.001428 | -0.001635 | -0.001392 | 0.000000 |
| nfcorpus | baseline | 0.344921 | 0.160261 | 0.285311 | 0.566408 | 0.330929 | 0.573762 |
| nfcorpus | raw_checkpoint | 0.343910 | 0.160273 | 0.284818 | 0.565582 | 0.329938 | 0.578360 |
| nfcorpus | thresholded_checkpoint | 0.343135 | 0.159758 | 0.285357 | 0.564111 | 0.329659 | 0.575903 |
| nfcorpus | delta | -0.001785 | -0.000503 | 0.000046 | -0.002297 | -0.001269 | 0.002141 |
| scifact | baseline | 0.708953 | 0.675193 | 0.934333 | 0.685412 | 0.404167 | 0.993333 |
| scifact | raw_checkpoint | 0.706949 | 0.673192 | 0.931000 | 0.681897 | 0.402600 | 0.993333 |
| scifact | thresholded_checkpoint | 0.707024 | 0.673302 | 0.931000 | 0.682034 | 0.402367 | 0.993333 |
| scifact | delta | -0.001929 | -0.001891 | -0.003333 | -0.003378 | -0.001800 | 0.000000 |

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
