# M657 / Support-Rank Barrier Report

Status: `support_rank_barrier_failed`

M657 tests the next hypothesis after M656.  M656 proved that a
query-conditioned masked head can produce boundary Recall movement, but it
damaged dense/P1 support geometry.  M657 adds an explicit protected-top100
versus outside-risk rank barrier to determine whether the same model can cross
boundary positives without letting intruders replace protected top100 rows.

## Runs

| Run | Key config | Status | all dO@100 | all dNDCG@10 | all dMAP@100 | all dR@100 | boundary dO@100 | boundary dR@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M656 aggressive | `MAX_DELTA=0.03`, no barrier | failed | -0.033867 | +0.001538 | +0.002484 | -0.003683 | -0.043000 | +0.001905 |
| M656 safe | `MAX_DELTA=0.005`, strong scalar preservation | failed | -0.004258 | +0.000539 | +0.000597 | +0.000000 | -0.006000 | +0.000000 |
| M657 barrier | support-rank barrier | failed | -0.030977 | +0.001907 | +0.002288 | -0.000074 | -0.034333 | -0.000635 |
| M657 safe barrier | strong barrier + small delta | failed | -0.003164 | +0.000161 | +0.000238 | -0.000558 | -0.002000 | -0.004762 |

## What Happened

The support-rank barrier did not solve the M656 bottleneck.

The regular M657 barrier run still lost too much support overlap and no longer
improved boundary Recall.  The safe barrier run reduced overlap damage, but it
also pushed boundary Recall negative.  In other words, the barrier converts the
model back into rank polishing rather than support-safe crossing.

The loss traces confirm the barrier was active but ineffective:

| Run | first support-rank loss | last support-rank loss |
| --- | ---: | ---: |
| M657 barrier | 0.667291 | 0.666402 |
| M657 safe barrier | 0.672160 | 0.671285 |

The barrier term barely decreases.  Increasing its weight raises total loss but
does not create a clean protected-vs-intruder separation.

## Interpretation

This is a stop signal for the current M654-M657 coordinate-mask branch.

The branch produced useful evidence:

1. M654 showed coordinate-level safe movement exists under oracle selection.
2. M655 showed fixed masks can polish ranking but cannot create Recall
   crossing.
3. M656 showed a query-conditioned masked head can cross, but only by damaging
   dense support.
4. M657 showed support-rank barrier does not repair that damage.

The current representation/mask does not expose a clean local degree of freedom
that simultaneously improves boundary Recall and preserves dense/P1 support.
Further tuning of the same mask, delta scale, or support-rank weight is likely
to loop.

This also matches the older M536 lesson: broad support-rank/BCE-like objectives
were already treated as a negative control because they damaged row geometry.
M657 is narrower and query-conditioned, but it fails for the same practical
reason: rank barriers are not producing a stable, useful output-head geometry.

## Current Block

The block is not lack of training depth.  The block is objective geometry.

The model has enough capacity to move rankings.  It can improve MAP/MRR or
force boundary crossing.  What it cannot do in this coordinate-mask family is
keep the dense-equivalent top100 support fixed while crossing new positives.
That means the mask is not the right abstraction for the next core breakthrough.

## Stop Condition

Stop M654-M657 as a main branch.

Do not continue with:

- larger masked heads on the same coordinates;
- more support-rank weight sweeps;
- more global/fixed-mask scale tuning;
- reviving old support-rank/BCE stage2 objectives.

## Next Direction

Return to the first-stage output-head/posting-compiler route, but with the
M654-M657 lessons:

1. The desired output cannot be a local query-side patch over M549 masks.
2. The first-stage compiler must directly generate a dense-faithful posting
   surface with support geometry built in.
3. The training target should be teacher-derived posting distribution and
   support membership, with retrieval constraints used as diagnostics or late
   fine-tuning, not as the primary geometry-creating force.
4. The next fast probe should test a non-local output-head parameterization
   that can alter support geometry coherently, rather than sparse coordinate
   deltas.

Suggested next experiment: M658 `support-geometry compiler reset`.

M658 should train a compact output head to predict the dense-derived posting
surface directly under dense-faithfulness gates, then evaluate whether any
retrieval-constrained fine-tuning is still needed.  This returns to the user's
core hypothesis: first preserve dense capability as a standalone encoder, then
solve BM25/search fusion separately.

## Artifacts

- M657 barrier JSON:
  `runs/ii42-m657-support-rank-barrier-v1/m657_support_rank_barrier_seed6571/m657_support_rank_barrier_seed6571.json`
- M657 safe barrier JSON:
  `runs/ii42-m657-support-rank-barrier-v1/m657_support_rank_safe_seed6571/m657_support_rank_safe_seed6571.json`
- M657 checkpoints:
  `runs/ii42-m657-support-rank-barrier-v1/m657_support_rank_barrier_seed6571/m657_support_rank_barrier_seed6571.masked_head.pt`
  `runs/ii42-m657-support-rank-barrier-v1/m657_support_rank_safe_seed6571/m657_support_rank_safe_seed6571.masked_head.pt`
