# M736 Positive-Enriched Bundle Teacher

## Purpose

M735 showed that atom-level causal labels are separable, but deployable utility
selection did not pick any holdout productive atom.  M736 tests the next
minimal hypothesis: the teacher unit may need to be a tiny atom bundle, and the
training surface may need more positive-enriched rows before any deep compiler
training.

This stage keeps P1.3/M549U document posting geometry frozen and reuses the
native unified-posting replay path.  Oracle policies are teacher-only and may
use M735 labels; model policies use only M735B model scores.

## Runs

| Run | Scope | Output |
| --- | --- | --- |
| M736A | M735 20-query rows, bundle replay | `docs/research-sae/reports/m0700-m0799/ii42-m736a-positive-enriched-bundle-replay-report.md` |
| M736B | Expanded atom replay, 50 queries per canary dataset | `docs/research-sae/reports/m0700-m0799/ii42-m736b-expanded-causal-atom-replay-report.md` |
| M736B selector | Expanded causal separability model | `docs/research-sae/reports/m0700-m0799/ii42-m736b-causal-signal-separability-report.md` |
| M736C | Expanded bundle replay using expanded rows/selector | `docs/research-sae/reports/m0700-m0799/ii42-m736c-expanded-bundle-replay-report.md` |

## Key Results

### M736A small smoke

| Surface | Split | Rows | Productive | dRecall | dMAP | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| oracle_productive_bundle | holdout | 1 | 1 | +0.000000 | +0.000391 | +0.000000 | +0.000000 | +0.000000 | 1 |
| model_productive_bundle | holdout | 10 | 0 | +0.000000 | +0.000000 | +0.000000 | -0.001000 | +0.000000 | 0 |

Small-sample conclusion: oracle bundle signal exists, but the M735B model
surface does not recover it.

### M736C expanded smoke

| Surface | Split | Rows | Productive | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| oracle_productive_bundle | holdout | 5 | 5 | +0.040000 | +0.007242 | +0.002607 | +0.000000 | +0.000000 | +0.002000 | +0.002344 | 1 |
| oracle_productive_plus_safe_bundle | holdout | 5 | 5 | +0.040000 | +0.007166 | +0.002607 | +0.000000 | +0.000000 | +0.002000 | +0.002344 | 1 |
| model_productive_bundle | holdout | 29 | 1 | +0.000000 | +0.000003 | +0.000000 | +0.000000 | +0.000000 | -0.000345 | +0.000135 | 0 |

Expanded-sample conclusion: increasing teacher density helps.  The model
surface now finds a productive holdout bundle, but it still admits dense
overlap damage.  A post-hoc threshold audit over safe/danger/productive scores
found no threshold that both keeps a productive holdout row and clears the
strict O@100 gate.  Therefore this is not a simple threshold-calibration
problem.

## Interpretation

M736 preserves an important positive result: support-safe, productive
query-local posting bundles exist in native replay.  The oracle expanded
surface is strong enough to improve Recall@100, MAP@100, NDCG@10, and dense
overlap simultaneously on the holdout slice.

The deployable selector remains the bottleneck.  M736C shows that M735B model
scores can find some productive rows, but the same score surface cannot
separate dense-overlap damage.  The damaging bundle still passes all simple
safe/danger/productive thresholds, so more training depth on the same feature
surface is unlikely to be the right next move.

## Decision

Do not expand the current model_productive_bundle selector to shared15.

Do not start deep compiler training from this selector yet.

Proceed to a new selector-interface step that adds boundary damage witnesses:

1. Treat oracle productive bundles as positive teachers.
2. Treat model-selected O@100/O@256 loss bundles as explicit hard negatives.
3. Add features derived from pulled-in / pushed-out dense-overlap docs,
   boundary displaced docs, and per-atom interaction inside the bundle.
4. Train a conservative selector against both productive and damage labels.
5. Replay through native path before any broader expansion.

This should be M737, not another threshold sweep.
