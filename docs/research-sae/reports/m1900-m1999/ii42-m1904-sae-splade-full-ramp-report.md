# M1904 SAE-SPLADE Full-Ramp Report

Date: 2026-07-12

Decision: **stop official-data scale and native FiQA for this bounded
SAE-SPLADE branch. Complete the paired M1905 standard-SPLADE control before
attributing the failure to the SAE representation.**

## Question Answered

M1903 showed that deeper SAE reconstruction and 2,000 retrieval steps produced
real disjoint ranking gains, but its document activations remained nearly
universal. M1904 continued the locked SAE checkpoint for 10,000 retrieval
steps and completed the full 6,000-step FLOPS ramp. It asked whether the high
document-frequency shape was merely an unfinished regularization schedule.

The answer on this bounded surface is no. The schedule can strongly suppress
the sampled activation/FLOPS proxies, but the same transition removes ranking
quality. No checkpoint at or after the completed ramp passes the frozen M1903
ranking floor.

## Locked Selection Trajectory

| Step | Eligible | Pairwise | Positive top1 | KL | Doc nnz | Doc FLOPS | Doc maxDF |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | no | 0.721680 | 0.375000 | 1.443044 | 117.83 | 93.867103 | 1.000000 |
| 500 | no | 0.788086 | 0.484375 | 1.082575 | 65.49 | 68.987032 | 1.000000 |
| 1,000 | no | 0.788086 | 0.445312 | 1.060095 | 71.54 | 58.570003 | 1.000000 |
| 2,000 | **yes** | 0.830078 | 0.578125 | 0.949281 | 100.20 | 10.095506 | 0.986979 |
| 4,000 | no | 0.849609 | 0.531250 | 0.931307 | 189.47 | 3.815748 | 0.943576 |
| 6,000 | no | 0.802734 | 0.414062 | 1.112573 | 149.08 | 1.352761 | 0.357639 |
| 8,000 | no | 0.788086 | 0.398438 | 1.133933 | 111.89 | 0.855994 | 0.288194 |
| 10,000 | no | 0.796875 | 0.429688 | 1.152607 | 106.69 | 0.809938 | 0.229167 |

Step 4,000 is informative but not selectable. Its pairwise agreement and KL
improve while sampled maxDF falls below `0.95`, but positive top1 is
`0.531250`, missing the predeclared floor `0.534688` by `0.003438`. After the
FLOPS ramp reaches full strength at step 6,000, positive top1 falls by another
0.117 absolute and does not recover by step 10,000.

The only eligible checkpoint is step 2,000, before the full ramp. The
cost-first frontier therefore restores that state for the one disjoint
confirmation evaluation.

## Disjoint Confirmation

| Surface | Pairwise | Positive top1 | KL | Doc nnz | Doc FLOPS | Doc maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1903 selected | 0.809814 | 0.484375 | 0.994030 | 122.65 | 30.548737 | 1.000000 |
| M1904 selected step 2,000 | 0.798584 | 0.460938 | 1.017299 | 101.36 | 10.072014 | 0.987630 |
| OpenSearch sparse-v2 control | 0.924072 | 0.675781 | 1.250686 | 177.41 | 0.617832 | 0.063802 |

Relative to M1903, the selected M1904 checkpoint changes:

- pairwise agreement: `-0.011230`;
- positive top1: `-0.023438`;
- KL: `+0.023269`, which is worse;
- document nnz: `0.8264x`;
- document FLOPS: `0.3297x`;
- sampled document maxDF: `0.9876x`.

The branch passes its internal confirmation gate relative to its much weaker
pre-training baseline, but fails the actual M1903 retention gate and remains
far behind the mature OpenSearch control. The final result correctly records
`authorize_official_data_scale=false` and `authorize_native_fiqa=false`.

## Interpretation

M1904 resolves the immediate schedule ambiguity. The M1903 high-frequency
shape was not simply waiting for the FLOPS ramp to finish: full regularization
changes that shape, but on this teacher/data scale it does so by trading away
positive ordering and teacher fit.

This is not a global falsification of SAE-SPLADE. The experiment remains much
smaller than the paper route: 10,000 local teacher rows and 10,000 retrieval
steps rather than the official 27 GB ColBERTv2 64-way surface and 240,000
steps. The measured maxDF is also computed over disjoint candidate documents,
not the complete MS MARCO corpus. It is a schedule/representation diagnostic,
not a native latency measurement.

Recent learned-sparse evidence further requires that distinction. Li-LSR and
M1910 show that modern sparse engines can make less-regularized expansion
practical, so a low sampled FLOPS value is not itself a product objective.
M1904 is stopped because ranking does not survive, not because it failed to
reach an arbitrarily small activation proxy.

## Next Gate

M1905 is running the standard vocabulary SPLADE head with the identical rows,
teacher, seed, batch schedule, optimizer, ranking loss, FLOPS weights, and
6,000-step ramp. Its purpose is causal localization:

- if standard SPLADE alone has a post-ramp survivor, the bounded result favors
  the mature vocabulary projection;
- if neither branch survives, close this small local teacher surface rather
  than tuning another SAE loss;
- if SAE alone survives, require official-data reproduction before any route
  promotion.

M1905 ClearML task: `2048e41d64fb405db86058eda9d83c0b`.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1904-sae-splade-full-ramp-contract.md`
- Runner: `scripts/run_m1904_sae_splade_full_ramp_spark.sh`
- Training script: `scripts/train_m1904_sae_splade_full_ramp.py`
- ClearML task: `60ffc91de0ef463b9e1ac91d0b99b18d`
- Remote summary: `m1904-sae-splade-full-ramp-v1/summary.json`
