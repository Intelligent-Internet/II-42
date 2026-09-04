# M630 / P1.4 Dense-Calibrated Score Report

Status: smoke completed; no M630-B model promoted.

## Objective

M630-B tests whether the current P1 first-stage native posting score can be
globally recalibrated toward dense teacher behavior without BM25, fixed alpha
search, qrels training, dataset-specific tuning, or a second-stage reranker.

Inference features are limited to P1/posting/lexical/atom geometry.  Dense rank
and dense target are labels only.  BM25 and fused scorer fields are excluded.

## Smoke Results

Baseline is native P1 rank on the P1 top1000 candidate pool.  Evaluation uses
full qrels denominators from M604 gap rows.

Artifact inventory:

| Artifact | Path |
| --- | --- |
| `M630-B listwise replay` | `runs/m630_p1p4_dense_calibrated_scorer_smoke_v1/m630_p1p4_dense_calibrated_replay.json` |
| `M630-B anchor010 model` | `runs/m630_p1p4_dense_calibrated_scorer_smoke_anchor010_v1/m630_p1p4_dense_calibrated_model.json` |
| `M630-B anchor010 replay` | `runs/m630_p1p4_dense_calibrated_scorer_smoke_anchor010_v1/m630_p1p4_dense_calibrated_replay.json` |
| `M630-B pairwise replay` | `runs/m630_p1p4_dense_calibrated_scorer_smoke_pairwise_v1/m630_p1p4_dense_calibrated_replay.json` |
| `M603-style eval rows` | `runs/m630_p1p4_eval_style_smoke_v1/m630_p1p4_smoke/` |
| `Gap delta JSON` | `runs/m630_p1p4_gap_delta_smoke_v1/m630_p1p4_gap_delta_smoke.json` |
| `Gap delta MD` | `runs/m630_p1p4_gap_delta_smoke_v1/m630_p1p4_gap_delta_smoke.md` |

| Variant | Gate | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap@100 | dDense KL |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `listwise` | `guarded_not_promoted` | -0.003183 | +0.002209 | +0.000000 | -0.000708 | +0.000000 | -1.072361 |
| `listwise_anchor_0.10` | `guarded_not_promoted` | -0.000922 | +0.001235 | +0.000000 | +0.004724 | +0.000000 | -1.049708 |
| `listwise_anchor_0.25` | `guarded_not_promoted` | -0.008043 | -0.001611 | +0.000000 | -0.003898 | +0.000000 | -0.989348 |
| `listwise_anchor_0.50` | `guarded_not_promoted` | -0.003491 | -0.000438 | +0.000000 | -0.001961 | +0.000000 | -0.856346 |
| `listwise_anchor_1.00` | `guarded_not_promoted` | -0.008151 | -0.001508 | +0.000000 | -0.010183 | -0.000196 | -0.697039 |
| `dense_top100_pairwise` | `guarded_not_promoted` | -0.053731 | -0.068330 | -0.139778 | -0.005648 | -0.850588 | +346.609050 |

The best bounded variant is `listwise_anchor_0.10`:

- It preserves NDCG@10 within the 0.001 guard.
- It improves MAP@100 and MRR@20 on smoke.
- It reduces dense KL strongly.
- It does not improve Recall@100 or dense top100 overlap.

## Interpretation

The positive signal is narrow but real: P1/posting-only features can fit dense
teacher score distribution, and a small baseline anchor can prevent most top10
damage.  This confirms that score geometry is learnable.

The missing signal is promotion: none of the listwise variants changed
Recall@100 on smoke.  The dense top100 support pairwise variant attempted to
force membership recovery, but it failed catastrophically by learning an
unstable global tail-promotion shape.  That mode should not be expanded.

The most likely reason is that dense-top100 membership is already largely
encoded by P1 rank on this smoke surface, while the residual dense-support
differences are not predictable enough from the current P1/posting-only
features.  Distribution fitting improves within-top100 ordering but does not
recover new relevant documents into top100.

## Decision

Do not promote M630-B and do not expand it to shared15 yet.

The gap delta artifact confirms the stop condition:

- Best bounded M630 variant: `listwise_anchor_0.10`
- M630 smoke dRecall@100: `+0.000000`
- M629-B context dRecall@100: `+0.001923` on shared15 eval split
- M630 does not exceed M629-B by a meaningful margin and does not reach the
  required `+0.005` learned-only Recall@100 acceptance threshold.

Retain:

- `scripts/build_m630_score_geometry_audit.py`
- `scripts/train_m630_dense_calibrated_scorer.py`
- `listwise_anchor_0.10` as a diagnostic reference, not a model baseline

Reject:

- `dense_top100_pairwise` in its current form
- broad expansion of M630-B without a Recall-bearing smoke signal

## Next Practical Direction

The next effective stage should not be another fixed-alpha search or a broad
reranker loop.  The evidence suggests a missing first-stage feature channel:
the current P1/posting features can calibrate dense score shape but cannot
predict which P1-top1000 tail documents should enter top100.

The next probe should therefore be a constrained first-stage compiler update:

1. Add one new auditable feature family that can explain dense-support tail
   membership without BM25, such as query-local atom residuals, dense-tail
   bucket prototypes, or signed-coordinate margin features.
2. Re-run M630-A to verify that the new feature family changes only the
   score-geometry explanation surface.
3. Re-run `listwise_anchor_0.10` and require non-zero Recall@100 gain on smoke
   before any shared15 expansion.

Stop if the new feature family still only reduces KL/MAP but does not improve
Recall@100.
