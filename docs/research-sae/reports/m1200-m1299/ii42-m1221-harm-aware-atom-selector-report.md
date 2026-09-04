# M1221 Harm-Aware Atom Selector

## Question

M1220 made teacher targets observable but could not separate them from harmful
atoms.  M1221 trains separate held-out target and harm models on the same
retrieval-conditioned atom features, then tests fixed selectors of the form:

`target_score - lambda * harm_score`

This remains an observability audit.  It does not replay generated postings.

## Results

Hard-row smoke did not pass.  The best target precision stayed below harm
precision on `cqadupstack`, `scidocs`, and `webis-touche2020`.

Full shared15 produced a real separation signal:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `target_minus_harm0.5_pos_top1` | 0.3652 | 0.2142 | 0.1515 | +0.0628 | 0.1908 | 0.890 |
| `target_minus_harm0.25_pos_top1` | 0.3866 | 0.2101 | 0.1527 | +0.0574 | 0.2019 | 0.961 |
| `target_minus_harm1_pos_top8` | 0.7518 | 0.2134 | 0.1607 | +0.0526 | 0.2064 | 1.841 |
| `target_only_top3` | 0.8146 | 0.1742 | 0.1752 | -0.0009 | 0.2422 | 2.442 |
| `target_only_top8` | 1.0000 | 0.1529 | 0.1869 | -0.0340 | 0.2548 | 3.417 |

## Interpretation

M1221 is the first positive selector signal after M1219/M1220:

- Full shared15 can separate target atoms from harm atoms.
- The useful region is not target-only top8.  It is harm-aware and often
  positive-only.
- `target_minus_harm1_pos_top8` keeps high target recall while creating a
  visible precision-vs-harm gap, so it is the only policy worth replaying.

The hard-row smoke is still negative.  That means the result is not safe enough
to promote, but it is strong enough to justify one bounded native replay.

## Decision

Replay only `target_minus_harm1_pos_top8`.

Do not run another lambda or threshold sweep unless native replay shows that
the signal transfers to metrics.

## Artifacts

- Script: `scripts/audit_m1221_harm_aware_atom_selector.py`
- Smoke JSON: `runs/m1221_harm_aware_atom_selector_smoke_v1/m1221_harm_aware_atom_selector.json`
- Full JSON: `runs/m1221_harm_aware_atom_selector_v1/m1221_harm_aware_atom_selector.json`
