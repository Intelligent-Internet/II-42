# M1518 DF-FLOPS Paired Smoke Report

Date: 2026-07-10

Decision: primary S1 and the qrels-free budget-matched M1518B correction both
failed the corpus-cost gate. Close the pooled M1510 DF-FLOPS source; do not
authorize S2 or an alpha correction.

## Paired design

Both branches start from the same M1510 SPLARE reproduction adapter, pinned
MS MARCO rows, seed, batch order, optimizer, and inference budgets. The first
100 optimizer steps use ordinary FLOPS in both branches. At step 101 only the
document regularizer changes to DF-FLOPS with alpha 0.10 and beta 10.

Step 0 and step 100 metrics match exactly, establishing the causal control.
No BEIR qrels, BM25 inference score, dataset-specific threshold, or checkpoint
selection enters the run.

## Formal gate

| Final metric | FLOPS | DF-FLOPS | Delta |
| --- | ---: | ---: | ---: |
| KL | 0.612169 | 0.597826 | -0.014343 |
| Pair accuracy | 0.888672 | 0.908203 | +0.019531 |
| Head 1% posting share | 0.162576 | 0.151267 | -0.011309 |
| Mean active document dimensions | 307.752 | 395.674 | +87.922 |
| Score cosine to initialization | 0.985352 | 0.989119 | +0.003767 |
| Top atom DF | 1.000000 | 1.000000 | 0.000000 |
| Mean touched-document ratio | 1.000000 | 1.000000 | 0.000000 |

The ranking guard passes and DF-FLOPS improves teacher fit, pair ordering,
score preservation, and head concentration. The two required corpus-cost
checks both fail, so the generated gate decision is `stop_m1518_s1`.

## Failure mechanics

The checkpointed fixed-sample DF estimator shows redistribution rather than
restored selectivity:

| Step | Active dims | DF=100% | DF>=90% | DF>=50% | DF>=20% | DF>=10% | DF>=2% |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 17,739 | 7 | 11 | 21 | 86 | 314 | 3,339 |
| 200 | 21,307 | 2 | 3 | 17 | 137 | 563 | 5,273 |
| 300 | 21,520 | 2 | 3 | 20 | 138 | 544 | 5,213 |
| 400 | 22,164 | 4 | 8 | 20 | 95 | 476 | 5,329 |

DF-FLOPS initially removes most universal dimensions and lowers head posting
share, but mass spreads into many lower-frequency dimensions. Four universal
dimensions remain at the final checkpoint and the union of the query's 40
dimensions still touches every sampled document.

This is not a ranking-capacity failure. It is a regularization-strength and
query-union mismatch. Ordinary FLOPS and same-weight DF-FLOPS both leave the
posting index at full touch.

## Literature and correction

[Porco et al.](https://arxiv.org/abs/2505.15070) train for 50,000 steps, ramp
regularization for 30,000 steps, and explicitly use higher regularization
factors for DF-FLOPS because its value is always no greater than ordinary
FLOPS. The primary M1518 run intentionally held lambda fixed for a clean
mechanism control, but its first DF-active batch made the resulting mismatch
measurable:

| Quantity at step 101 | Value |
| --- | ---: |
| Ordinary document FLOPS | 681.562744 |
| DF-conditioned document FLOPS | 233.180771 |
| Exact budget-match ratio | 2.922894 |
| KL parity error | 0 |
| Query-FLOPS parity error | 0 |

M1518B applies `df_loss_scale=2.922894291780351` only when DF weights are
active. It preserves exact ordinary-FLOPS behavior through step 100 and then
matches the initial document regularization budget without qrels or a grid
search. Alpha, beta, data, model, and gate remain frozen.

## M1518B result

M1518B preserved exact metric parity at steps 0 and 100. At the first active
batch, raw DF-FLOPS 233.18077 multiplied by 2.922894 exactly reproduced the
ordinary document penalty 681.56274 and the paired total loss. The subsequent
trajectory therefore isolates the stronger DF-conditioned gradients.

| Final metric | FLOPS | Primary DF-FLOPS | Budget-matched DF-FLOPS |
| --- | ---: | ---: | ---: |
| KL | 0.612169 | 0.597826 | 0.626783 |
| Pair accuracy | 0.888672 | 0.908203 | 0.898438 |
| Head 1% posting share | 0.162576 | 0.151267 | 0.133156 |
| Mean active document dimensions | 307.752 | 395.674 | 385.488 |
| Score cosine to initialization | 0.985352 | 0.989119 | 0.988919 |
| Top atom DF | 1.000000 | 1.000000 | 1.000000 |
| Mean touched-document ratio | 1.000000 | 1.000000 | 1.000000 |

The stronger objective further lowers head concentration, but teacher KL is
now slightly worse than FLOPS and neither required cost metric changes.

| M1518B step | Active dims | DF=100% | DF>=90% | DF>=50% | DF>=20% | DF>=10% | DF>=2% |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 17,739 | 7 | 11 | 21 | 86 | 314 | 3,339 |
| 200 | 21,236 | 2 | 3 | 15 | 100 | 549 | 5,352 |
| 300 | 21,335 | 2 | 3 | 8 | 87 | 547 | 5,515 |
| 400 | 21,803 | 1 | 4 | 15 | 79 | 450 | 5,386 |

At the final checkpoint, latent 57373 remains active in every sampled
document, latent 32989 remains active in 99.83%, and latent 23201 in 98.96%.
Budget matching can redistribute mass and nearly remove the universal head,
but it does not cross the selectivity boundary.

## Final route decision

Do not run M1518C, S2, complete-corpus BEIR validation, or native-index work
from this source. Alpha controls which DF ratios receive near-maximal weight;
the final universal atom already has weight 1. Lowering alpha would penalize
more medium-DF atoms but cannot strengthen the saturated universal-atom
gradient that failed here.

The retained scientific result is that retrieval-trained sparse SAE latents
carry real ranking signal and respond to corpus-aware regularization, but the
pooled M1510 representation depends on corpus-universal structural latents.
It is therefore not a viable bounded-cost unified posting source. Future work
must change the representation source or explicitly remove background
components before retrieval training; it must not continue this checkpoint
with longer schedules, threshold grids, or post-hoc selectors.

## Artifacts

- `runs/m1518_df_flops_paired_smoke_v1/m1518_s1_paired_gate.json`
- `runs/m1518_df_flops_paired_smoke_v1/flops.metrics.jsonl`
- `runs/m1518_df_flops_paired_smoke_v1/df-flops.metrics.jsonl`
- `runs/m1518_df_flops_paired_smoke_v1/m1518b_df_budget_scale.json`
- `runs/m1518b_df_flops_budget_match_v1/m1518_s1_paired_gate.json`
- `runs/m1518b_df_flops_budget_match_v1/df-flops.metrics.jsonl`
- `scripts/derive_m1518_df_budget_scale.py`
- `scripts/run_m1518b_df_flops_budget_match_spark.sh`
