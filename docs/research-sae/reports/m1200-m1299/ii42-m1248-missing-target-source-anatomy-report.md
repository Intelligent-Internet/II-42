# M1248 Missing Target Source Anatomy

## Goal

M1245-M1247 showed that action routing and clean action-prior evidence do not
produce a deployable policy.  M1248 asks a more structural question:

Where are the CUB-specific target atoms that all-actions source_abs top8
misses?

This audit decides whether the next source should change coverage, action
routing, or rank-boundary budget.

## Runs

- Buggy first full run: `runs/m1248_missing_target_source_anatomy_v1/`
- Corrected full run: `runs/m1248_missing_target_source_anatomy_v2/`

The v1 run ranked by per-action max.  That does not match the source_abs
baseline used by M1241/M1244.  The v2 run ranks by summed absolute action
delta, and its coverage@8 matches the earlier source_abs baseline.

## Result

| Metric | Value |
| --- | ---: |
| target atoms | 2936 |
| coverage@8 | 0.7793 |
| missing target atoms | 648 |
| missing target share | 0.2207 |
| missing-query share | 0.0999 |

Rank bands:

| Band | Targets | HarmOverlap |
| --- | ---: | ---: |
| top4 | 1263 | 33 |
| top8 | 1025 | 9 |
| top12 | 290 | 1 |
| top16 | 243 | 0 |
| top32 | 115 | 0 |
| absent | 0 | 0 |

Missing target actions:

| Action | MissingTargets |
| --- | ---: |
| high | 26 |
| mid | 4 |
| low | 618 |

Missing target presence:

| Presence | MissingTargets |
| --- | ---: |
| 1 | 618 |
| 2 | 30 |

## Interpretation

The target atoms are not absent from action rows.  The missing mass is near the
rank boundary: ranks 9-16 recover 533 of 648 missing targets, and ranks 17-32
recover the rest.

The more important structure is action-specific: almost all missing targets are
`low` action and single-action.  This means the source_abs top8 score is
systematically under-allocating slots to low-action tail atoms.

## Decision

Do not build another action router.  The next useful source test is a
low-action reserve/quota over the all-actions top8 baseline.
