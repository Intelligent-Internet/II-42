# SAE M50 Hard-Dataset Semantic Retention Results Report

Status: completed first diagnostic sweep. Pure export expansion is helpful but
not sufficient.

## Summary

M50 tested whether the local regressions on `trec-covid` and `msmarco` are
caused by insufficient semantic retention in the exported query atoms.

The result is mixed but useful:

- larger semantic-retention profiles can repair the current M46/M49 regression
  on both datasets;
- no tested profile beats BM25 on both datasets for both NDCG@10 and MAP@100;
- the best target profile is much more expensive, so it is not directly
  promotable as the global default.

This means the current encoder has useful semantic signal that is being clipped
or under-used, but export expansion alone does not solve the ranking problem.
The next step should combine semantic-retention routing with BM25-preserving
score calibration.

## Baseline

Current M46/M49 deltas versus BM25:

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `trec-covid` | +0.002659 | -0.000357 | -0.023116 | -0.006017 |
| `msmarco` | +0.023504 | -0.042636 | -0.044267 | -0.018161 |

This is the important failure shape: recall improves, while top-rank quality
regresses.

## Sweep

The sweep reused the current M40 encoder checkpoint and did not retrain. It
only varied query payload retention:

```text
datasets           = trec-covid, msmarco
pool_active_dims   = 96,128,160,192
export_active_dims = 48,64,80,96
fanout_power       = 0.00,0.05,0.10,0.15,0.20,0.25
df_threshold       = 0.08,0.10,0.12,0.15
low_sae_weight     = 0.30,0.35,0.40,0.45,0.55
high_sae_weight    = 0.75,1.00,1.25
```

Output:

```text
results/sae/m50/semantic-retention-target-sweep/
results/sae/m50/semantic-retention-target-report/
```

## Outcome

| Check | Count |
| --- | ---: |
| Total rows evaluated | 5,760 |
| Improve both datasets versus current on NDCG/MAP | 1,269 |
| Beat BM25 on both datasets for NDCG/MAP | 0 |

Best target row:

```text
pool_active_dims   = 96
export_active_dims = 96
fanout_power       = 0.00
low_sae_weight     = 0.30
high_sae_weight    = 0.75
```

Best target-row deltas versus current M46/M49:

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `trec-covid` | +0.004705 | +0.030000 | +0.017992 | +0.022274 |
| `msmarco` | +0.032130 | +0.042636 | +0.081163 | +0.102927 |

Best target-row deltas versus BM25:

| Dataset | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `trec-covid` | +0.007364 | +0.029643 | -0.005124 | +0.016257 |
| `msmarco` | +0.055634 | +0.000000 | +0.036896 | +0.084766 |

Physical cost for the best target row:

| Candidate docs | BM25 postings | SAE postings |
| ---: | ---: | ---: |
| 10,388.372 | 36,372.319 | 13,332.302 |

The best row almost repairs `trec-covid`, but its NDCG@10 is still below BM25
by `0.005124`. It strongly repairs `msmarco`. The cost is roughly an order of
magnitude higher than the M46/M49 canonical selector, so this is diagnostic
evidence rather than a production profile.

## Decision

M50 confirms that semantic retention matters. The current encoder is not
missing all useful signal; keeping more query atoms can recover the hard
datasets.

M50 also shows that pure retention is not enough:

```text
larger semantic payload
-> better recall and better ranking than current
-> still not enough to beat BM25 everywhere
-> too expensive as a default
```

The next step should be M51:

```text
runtime-safe hard-query router
+ larger semantic-retention profile for selected queries
+ BM25-preserving score calibration / lexical floor
```

M51 should test whether the high-retention profile can be used only for queries
that need it, while preserving BM25's top-rank ordering on `trec-covid` and
`msmarco`.
