# M704 Budgeted Set-Selection Feasibility

## Purpose

M703 showed that raw expanded atom append can move native dense-boundary ranks,
but the effect is too weak and unsafe to justify long compiler training.

M704 tests a stronger first-stage upper bound: if we use dense-boundary labels
to select atoms from the expanded interface, can the native semantic scorer move
the intended dense-positive documents above the false-P1 negatives while
preserving the baseline head?

This is still first-stage only:

- no qrels objective
- no BM25 objective
- no learned gate
- no reranker
- frozen doc posting/index geometry

## Inputs

- Dense-boundary rows:
  `runs/m653_dense_boundary_training_rows_canary_v1/m653_dense_boundary_training_rows.jsonl`
- Expanded interface:
  `source384 / doc_atom_head48 / cap1536`
- Baseline:
  `P1.3 / M549U native signed-dot`

Outputs:

- JSON: `runs/m704_budgeted_set_selection_feasibility_v1/m704_summary.json`
- Markdown: `runs/m704_budgeted_set_selection_feasibility_v1/m704_report.md`

## Strict Gate Result

The strict gate requires:

- pair-success lift at least `0.01`
- fixed pairs at least twice regressed pairs
- macro top95 overlap at least `0.95`
- every dataset top95 overlap at least `0.95`

Best strict-gate variant:

| Variant | Pair success | Baseline success | Fixed | Regressed | Top95 | Min dataset Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `safe_fill_delta_target_b384_s0.02` | 0.617647 | 0.541176 | 234 | 0 | 0.967023 | 0.958000 |

Per-dataset:

| Dataset | Pair success | Top95 | Fixed | Regressed |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0.604381 | 0.970842 | 66 | 0 |
| cqadupstack | 0.668937 | 0.969910 | 48 | 0 |
| fiqa | 0.641732 | 0.969368 | 52 | 0 |
| scidocs | 0.559645 | 0.958000 | 68 | 0 |

This is the first strong first-stage signal in this sub-route: it is not a
single-dataset artifact, and it does not buy movement by spending regressions.

## Candidate-Impact Control

The target-impact variant is an oracle upper bound because it uses the
dense-positive/negative boundary docs to infer atom direction. A more
inference-compatible control uses the candidate-row impact direction:

| Variant | Pair success | Baseline success | Fixed | Regressed | Top95 | Min dataset Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `target_delta_candidate_b192_s0.05` | 0.594118 | 0.541176 | 162 | 0 | 0.975148 | 0.973263 |

This is weaker than the target-impact oracle, but still a real gate-passing
signal. It shows that the expanded interface contains usable direction
information, not only unattainable oracle information.

## Interpretation

M704 changes the status of the route.

Earlier:

1. M701 proved target atoms are visible under expanded interface.
2. M702 showed independent linear atom admission cannot compress them.
3. M703 showed raw append is too weak.

M704 now shows the missing ingredient is budgeted set selection plus impact
direction/scale. When target atoms are selected and assigned useful directions,
native semantic ranking moves substantially while preserving head overlap.

This answers the training-depth concern more precisely:

- Training longer on M700/M703 would have been wrong.
- Abandoning the route after M703 would also have been premature.
- The right next step is to train a model that approximates the M704 selector
  and impact direction under dense-equivalence guards.

## Decision

Proceed to M705.

M705 should train or audit a learnable approximation of M704:

1. Predict target-like atom selection from expanded-interface features.
2. Predict impact direction/scale, not just binary atom inclusion.
3. Keep per-query and per-dataset top95 overlap guards.
4. Evaluate through the same native semantic path on M653 canary before any
   broader shared15 matrix.

Do not promote M704 itself as a deployable model. It is an oracle/upper-bound
feasibility proof that identifies the next trainable objective.
