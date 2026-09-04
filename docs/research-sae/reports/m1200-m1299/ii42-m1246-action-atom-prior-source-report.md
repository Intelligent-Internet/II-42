# M1246 Action-Atom Prior Source

## Goal

M1244 showed a strong action-source oracle upper bound.  M1245 showed that
the winning action source is not safely predictable from current query-time
features.

M1246 tests a different idea: do not predict a winner action.  Instead, move
harm separation into the proposal source by learning action-specific atom
target/harm priors on training folds and applying them to held-out action rows.

This is a source separability audit before native replay.

## Inputs

- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Validation: leave-one-dataset-out
- Teacher surface: M1224 CUB-specific target/harm atoms
- Output:
  - `runs/m1246_action_atom_prior_source_smoke_v2/m1246_action_atom_prior_source.json`
  - `runs/m1246_action_atom_prior_source_smoke_v2/m1246_action_atom_prior_source.md`

## Result

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `all_actions_top8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |
| `all_actions_top4` | 0.8940 | 0.2277 | 0.0082 | 0.2195 | 3.422 |
| `consensus_delta_top4` | 0.6912 | 0.3425 | 0.0091 | 0.3333 | 1.759 |
| `consensus_delta_top8` | 0.7005 | 0.3297 | 0.0087 | 0.3210 | 1.851 |
| `consensus_clean_prior_top4` | 0.4424 | 0.3254 | 0.0102 | 0.3153 | 1.185 |
| `action_clean_delta_top4` | 0.4931 | 0.2373 | 0.0067 | 0.2306 | 1.811 |
| `action_prior_top8` | 0.9816 | 0.1859 | 0.0061 | 0.1798 | 4.602 |

The clean consensus source is real: it roughly doubles precision versus
`all_actions_top8` and keeps harm low.  But it loses too much target recall.

The refill variants are also diagnostic:

| Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| `consensus_delta_prefix1_fill8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 |
| `consensus_delta_prefix2_fill8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 |
| `consensus_delta_prefix4_fill8` | 0.9770 | 0.1850 | 0.0061 | 0.1789 |

Clean-prefix plus all-actions refill collapses exactly to the baseline.  The
clean atoms are already inside the `all_actions_top8` set on this smoke
surface, so the source does not create a better candidate set.

## Interpretation

M1246 is not a deployable candidate-source breakthrough.

It does produce one useful observation: consensus/action-prior atoms are clean
evidence, but they are evidence for weighting or ordering rather than new atom
coverage.  As a set-selection source, they trade away too much recall.  As a
prefix/refill source, they do not change the selected set.

This means the remaining problem is no longer just "find cleaner atoms."  On
this surface, the clean atoms are already present.  The next productive line
must decide whether their cleanliness can be converted into score/impact
calibration, or change the teacher/candidate source to expose target atoms not
already dominated by `all_actions_top8`.

## Decision

Stop M1246 at smoke.  Do not run full shared15 and do not launch native replay
from this source as-is.

Next stage should not be another classifier or threshold.  It should test a
different mechanism:

1. Use consensus/action-prior evidence as a scoring or impact-calibration
   signal, not as a replacement source; or
2. Change the teacher/candidate construction so the clean source contributes
   new target atoms outside the current all-actions top8 set.
