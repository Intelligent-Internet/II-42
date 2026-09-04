# ii42 M353 Recall-Constrained Anchor Policy Report

## Goal

M352 showed the first positive learned-policy signal: learned `ndcg` improved
all-query NDCG/MAP/MRR versus fixed `alpha=0.03`, but still reduced
Recall@100. M353 tests whether the same learned gate can be calibrated by
raising the probability threshold, reducing high-alpha usage and Recall loss
while preserving rank-quality gains.

This is still a posthoc diagnostic. It does not change the base scorer training
loss.

## Contract

- Base runner: `scripts/run_m344_explicit_admission_broad8_spark.sh`
- Shared runner: `scripts/run_m334_interaction_feature_scorer_spark.sh`
- M353 runner:
  `scripts/run_m353_recall_constrained_anchor_policy_spark.sh`
- Run dir:
  `/home/huoju/leask/runs/ii42-m353-recall-constrained-anchor-policy-v1`
- Output:
  `/home/huoju/leask/runs/ii42-m353-recall-constrained-anchor-policy-v1/m353_recall_constrained_anchor_policy_seed1050_split1050.json`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m343-explicit-admission-candidate-cache-v1`

Training is kept comparable with M352:

- `LEARNING_RATE=3.0e-4`
- `EVAL_EVERY=5`
- `EPOCHS=60`
- `EARLY_STOP_PATIENCE=4`
- `LISTWISE_WEIGHT=0.80`
- `RANK_DISCOUNT_ADMISSION_WEIGHT=0.30`
- `RANK_DISCOUNT_ADMISSION_CUTOFF=100`
- `RANK_DISCOUNT_ADMISSION_MARGIN=0.03`
- `RANK_DISCOUNT_ADMISSION_NEGATIVES=32`

## Calibration Rows

Fixed-alpha controls:

- `0.03`;
- `0.18`;
- `0.22`.

Learned gate:

- objective: `ndcg`;
- low alpha: `0.03`;
- high alpha: `0.18`;
- label min-delta: `0.0`;
- probability thresholds: `0.50`, `0.60`, `0.70`, `0.80`, `0.90`.

The `0.50` row should reproduce the M352 learned `ndcg` behavior. Higher
thresholds should use high alpha less often, which should reduce Recall loss if
the gate is meaningfully calibrated.

## Stop Rule

Promote this line only if at least one threshold keeps all-query NDCG/MAP/MRR
above fixed `alpha=0.03` while materially reducing the M352 Recall loss.

If every calibrated threshold either loses rank-quality gain or still carries
unacceptable Recall loss, stop posthoc learned-gate work and move the anchor
signal into the broader final admission/ranking objective instead.

## Current Status

- Local implementation added and syntax-checked.
- Runner added and syntax-checked.
- Launched on `spark-1` in tmux session
  `ii42_m353_recall_constrained_anchor`.
- Confirmed command includes fixed alpha controls `0.03 0.18 0.22`.
- Confirmed command includes learned `ndcg` gate with thresholds
  `0.50 0.60 0.70 0.80 0.90`.
- Completed on `spark-1`; final JSON generated and copied locally.
- Runtime note: the scorer stage was CPU-bound before the first `epoch=5`
  log line, but then matched the M352 trace and early-stopped at epoch 30.

## Result

Run completed on `spark-1`.

- Final JSON:
  `/home/huoju/leask/runs/ii42-m353-recall-constrained-anchor-policy-v1/m353_recall_constrained_anchor_policy_seed1050_split1050.json`
- Local copy:
  `/tmp/ii42-m353-recall-constrained-anchor-policy-v1/m353_recall_constrained_anchor_policy_seed1050_split1050.json`
- Training matched the M352 scorer trace and early-stopped at epoch 30.

Macro all-query metrics versus fixed `alpha=0.03`:

| Row | R@100 | dR | MRR@20 | dMRR | NDCG@10 | dNDCG | MAP@100 | dMAP |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| fixed 0.03 | 0.707038 | +0.000000 | 0.378365 | +0.000000 | 0.372586 | +0.000000 | 0.307407 | +0.000000 |
| learned thr0.50 | 0.705069 | -0.001969 | 0.378631 | +0.000266 | 0.372917 | +0.000331 | 0.307620 | +0.000213 |
| learned thr0.60 | 0.706085 | -0.000953 | 0.378634 | +0.000270 | 0.372896 | +0.000311 | 0.307667 | +0.000260 |
| learned thr0.70 | 0.707267 | +0.000229 | 0.378587 | +0.000222 | 0.373075 | +0.000490 | 0.307784 | +0.000376 |
| learned thr0.80 | 0.707096 | +0.000058 | 0.378287 | -0.000078 | 0.372670 | +0.000084 | 0.307433 | +0.000026 |
| learned thr0.90 | 0.707161 | +0.000124 | 0.378323 | -0.000042 | 0.372536 | -0.000049 | 0.307367 | -0.000041 |
| fixed 0.18 | 0.702202 | -0.004835 | 0.377973 | -0.000391 | 0.371813 | -0.000772 | 0.306325 | -0.001082 |
| fixed 0.22 | 0.701033 | -0.006005 | 0.377835 | -0.000530 | 0.371235 | -0.001351 | 0.305988 | -0.001419 |
| scorer | 0.707999 | +0.000962 | 0.378358 | -0.000007 | 0.372187 | -0.000399 | 0.307328 | -0.000079 |
| candidate upper bound | 0.941474 | +0.234436 | 1.000000 | +0.621635 | 0.963847 | +0.591262 | 0.941474 | +0.634066 |

Macro heldout metrics versus fixed `alpha=0.03`:

| Row | R@100 | dR | MRR@20 | dMRR | NDCG@10 | dNDCG | MAP@100 | dMAP |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| fixed 0.03 | 0.684136 | +0.000000 | 0.378437 | +0.000000 | 0.369319 | +0.000000 | 0.300175 | +0.000000 |
| learned thr0.50 | 0.685718 | +0.001583 | 0.378381 | -0.000056 | 0.370214 | +0.000895 | 0.300545 | +0.000370 |
| learned thr0.60 | 0.685394 | +0.001258 | 0.378584 | +0.000148 | 0.370248 | +0.000929 | 0.300682 | +0.000507 |
| learned thr0.70 | 0.685090 | +0.000954 | 0.378289 | -0.000147 | 0.370130 | +0.000811 | 0.300572 | +0.000397 |
| learned thr0.80 | 0.684173 | +0.000037 | 0.378363 | -0.000074 | 0.369656 | +0.000337 | 0.300315 | +0.000140 |
| learned thr0.90 | 0.684177 | +0.000041 | 0.378394 | -0.000043 | 0.369285 | -0.000034 | 0.300132 | -0.000043 |

Weighted policy usage:

| Row | Split | Threshold | High-rate | Mean alpha | Mean p(high) | Queries |
|---|---|---:|---:|---:|---:|---:|
| learned thr0.50 | all | 0.50 | 0.534183 | 0.110127 | 0.495216 | 6904 |
| learned thr0.60 | all | 0.60 | 0.428447 | 0.094267 | 0.495216 | 6904 |
| learned thr0.70 | all | 0.70 | 0.297943 | 0.074691 | 0.495216 | 6904 |
| learned thr0.80 | all | 0.80 | 0.124855 | 0.048728 | 0.495216 | 6904 |
| learned thr0.90 | all | 0.90 | 0.008835 | 0.031325 | 0.495216 | 6904 |
| learned thr0.70 | heldout | 0.70 | 0.400000 | 0.090000 | 0.571503 | 4200 |
| learned thr0.70 | train | 0.70 | 0.139423 | 0.050913 | 0.376723 | 2704 |

Per-dataset all-query `thr0.70` deltas versus fixed `alpha=0.03`:

| Dataset | dR@100 | dNDCG@10 | dMAP@100 |
|---|---:|---:|---:|
| arguana | +0.000000 | +0.000000 | +0.000000 |
| cqadupstack | +0.001934 | +0.001014 | +0.001022 |
| fiqa | -0.004129 | +0.000539 | -0.001608 |
| nfcorpus | -0.000089 | +0.000000 | -0.000015 |
| scidocs | -0.003052 | +0.000260 | +0.000135 |
| scifact | +0.000000 | +0.000000 | +0.000000 |
| trec-covid | -0.005754 | -0.017331 | -0.005418 |
| webis-touche2020 | +0.001534 | +0.000067 | +0.000322 |

## Verdict

M353 passes the macro stop rule with `thr0.70`: it keeps all-query Recall,
MRR, NDCG, and MAP above fixed `alpha=0.03`, while eliminating the M352
learned-gate Recall loss. The useful regime is not "use high alpha often"; it
is a constrained gate with high alpha on about 30% of all queries.

This should not be treated as final promotion yet. The macro gain is real but
small, and per-dataset risk is uneven: `trec-covid` regresses sharply in
NDCG/MAP, while `fiqa` and `scidocs` lose Recall. The next useful step is a
risk-constrained gate, not another unconstrained threshold sweep:

- keep `thr0.70` as the candidate operating point;
- add per-dataset or query-risk guardrails that fall back to `alpha=0.03` for
  fragile regimes;
- optimize the learned gate objective against Recall-constrained NDCG/MAP,
  rather than pure high-alpha win/loss classification.
