# M1269 Doc Movement Observability

## Question

M1268 failed because source-geometry features over-selected nonraw mixes on hard
rows.  M1269 checks whether the missing signal is visible after looking at the
native document/rank movement caused by a candidate nonraw mix.

This is still an audit.  It does not promote a policy.

## Run

- Script: `scripts/audit_m1269_doc_movement_observability.py`
- Output root: `runs/m1269_doc_movement_observability_v1/`
- JSON:
  `runs/m1269_doc_movement_observability_v1/m1269_doc_movement_observability.json`
- Markdown:
  `runs/m1269_doc_movement_observability_v1/m1269_doc_movement_observability.md`
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`

## Group Summary

Groups are derived from the M1268 LODO model and M1267 safe-oracle labels.

| Group | Count | MeanScoreVsRaw | RelevantEntrants | RelevantExits | NetRelevant | Entrants | Exits | Top20Changed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `false_positive_nonraw` | 95 | -0.001156 | 0.000 | 0.021 | -0.021 | 0.747 | 0.747 | 0.105 |
| `missed_nonraw` | 18 | +0.018643 | 0.056 | 0.000 | +0.056 | 0.444 | 0.444 | 0.333 |
| `true_nonraw` | 4 | +0.000919 | 0.000 | 0.000 | 0.000 | 2.250 | 2.250 | 0.500 |
| `wrong_nonraw` | 20 | +0.008535 | 0.000 | 0.000 | 0.000 | 0.700 | 0.700 | 0.500 |
| `true_raw` | 112 | +0.000000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |

Qrels-only columns are diagnostic only.  They confirm the failure mode:
false-positive nonraw choices can eject relevant top100 docs, while missed
nonraw choices contain real upside.

## True Nonraw vs False-Positive Nonraw

The movement features are qrels-free.  They compare the native candidate list
before/after applying the proposed nonraw mix.

| Feature | OrientedAUC | AUC | TrueNonrawMean | FalsePositiveMean |
| --- | ---: | ---: | ---: | ---: |
| `rank_down_count` | 0.9842 | 0.9842 | 38.500000 | 18.494737 |
| `shared_abs_rank_delta_mean` | 0.9658 | 0.9658 | 2.513192 | 0.699564 |
| `rank_up_count` | 0.9539 | 0.9539 | 36.000000 | 19.031579 |
| `entrant_fused_rank_mean` | 0.7763 | 0.2237 | 322.895833 | 535.696842 |
| `entrant_count` | 0.7724 | 0.7724 | 2.250000 | 0.747368 |
| `exit_count` | 0.7724 | 0.7724 | 2.250000 | 0.747368 |
| `shared_count` | 0.7724 | 0.2276 | 97.750000 | 99.252632 |

## Interpretation

This is a positive observability pivot.

M1268 did not fail because the safe/non-safe split is completely invisible.
It failed because selected-atom geometry is the wrong observation surface.
When the candidate mix is actually replayed and native doc/rank movement is
observed, useful nonraw choices and false-positive nonraw choices separate
strongly.

Key signal:

- false-positive nonraw changes are small, often noise-like rank perturbations;
- true nonraw changes create larger coherent rank movement;
- missed nonraw choices have the highest mean score upside, so a better action
  proposal/detector could recover additional value.

## Decision

Stop source-geometry-only calibrators.

Promote the next branch to a candidate-level movement safety detector:

1. Treat every nonraw mix as an action candidate.
2. Compute qrels-free doc/rank movement features for that action.
3. Label an action safe if it improves over raw without metric regressions.
4. Train an abstain-first detector under LODO hard-row validation.
5. Replay only accepted actions; otherwise fall back to raw.

This is not a return to generic gate tweaking.  It changes the observation
surface from atom geometry to native candidate-list movement, which M1269 shows
has materially stronger separability.
