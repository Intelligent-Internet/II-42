# ii42 M1121 Low-Pressure Shared5 Verdict

## Scope

M1121 reruns the M1118 shared5 export with lower posting-rank pressure:

- `RANK_EPOCHS=4`
- `LEARNING_RATE=0.0007`
- semantic hard negatives disabled
- datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`

The goal was to test whether the M1118/M1120 failure mode was caused by
over-pressured atom training. The specific failure mode was that train split
kept selecting high atom alpha (`0.75`), while heldout top-rank metrics peaked
near low alpha (`0.25`).

## Result

M1121 completed successfully on spark-1 and was replayed locally through the
same M1118/M1119/M1120 analysis path.

| Surface | Verdict | Train-selected alpha | Heldout-best alpha |
| --- | --- | ---: | ---: |
| M1118 shared5 | `do_not_promote_fixed_alpha` | 0.75 | 0.25 |
| M1121 low-pressure shared5 | `do_not_promote_fixed_alpha` | 0.75 | 0.30 |

The train/heldout alpha mismatch remains. Lower pressure did not fix alpha
selection reliability.

## Heldout Macro Comparison

M1121 minus M1118:

| Alpha | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.00 | -0.009089 | +0.000022 | +0.000018 | -0.000054 |
| 0.10 | +0.003811 | -0.011706 | -0.007570 | -0.004043 |
| 0.20 | +0.004225 | -0.016054 | -0.009164 | -0.003881 |
| 0.25 | +0.009974 | -0.014839 | -0.006684 | -0.004885 |
| 0.30 | +0.015308 | -0.015769 | -0.000728 | -0.002699 |
| 0.40 | +0.007655 | -0.013340 | +0.000538 | +0.000757 |
| 0.50 | +0.015863 | -0.003892 | +0.003317 | +0.004094 |
| 0.75 | +0.010974 | -0.000192 | +0.005142 | +0.002082 |

M1121 improves Recall across atom-weighted alphas and improves MAP/NDCG at
moderate/high alpha, but it loses the top-rank advantage that made M1118
`alpha=0.25` attractive.

## Best Heldout Profiles

| Run | Best profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| M1118 | additive atom 0.25 | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| M1121 | additive atom 0.30 | 0.703565 | 0.548662 | 0.494046 | 0.391612 |

M1121 is better for Recall, but M1118 remains better for MRR, NDCG, and MAP.

## Interpretation

Lower training pressure changes the scoring tradeoff, but it does not solve
the underlying selection problem.

Kept evidence:

1. Atom signal is real: M1121 still beats lexical heldout at low/mid additive
   alpha.
2. Pressure controls the Recall/top-rank tradeoff: lower pressure raises
   Recall at several alphas, but spends MRR.
3. Train split remains unreliable for alpha selection: it still chooses
   `alpha=0.75`, which is not the heldout-optimal operating point.

Rejected promotion:

1. Do not promote M1121 over M1118 as the default training recipe.
2. Do not continue low-pressure swap experiments unless the selection surface
   is redesigned.
3. Do not promote a fixed alpha based on train split.

## Next Direction

The next useful work is not another epoch or alpha swap. It should target the
selection objective:

1. Build an alpha/pressure selection gate using heldout-like constraints:
   preserve MRR/NDCG/MAP while allowing Recall gains.
2. Add query-level diagnostics for where atom pressure helps Recall but hurts
   top-rank evidence.
3. If expanding, run one structured candidate that explicitly optimizes
   low-alpha heldout top-rank quality rather than train rank loss.

Stop condition for this sub-route: if a selection gate cannot predict safe
low/mid atom pressure on shared5 without per-dataset tuning, stop fixed-alpha
scoring geometry and move to a different objective.
