# M713 Single-Atom Pair-Impact Audit

M713 answers a narrow first-stage question: before launching a deeper selector
or compiler training run, do individual candidate atoms have measurable native
scorer impact on dense-boundary pair recovery?

This audit keeps the current P1.3 / M549U native signed-dot baseline frozen.
It does not use BM25, reranking, qrels-driven optimization, or a learned gate.

## Why This Audit

The concern is valid: fast iteration can abandon a route before training depth
has enough chance to work. The way to avoid that is not to blindly run every
route longer; it is to identify whether the route has a scalable signal.

For the current dense-equivalence bottleneck, the scale-up rule should be:

- If a probe only improves a metric by spending dense overlap or support, do
  not scale it.
- If a probe cannot create boundary movement even under direct native
  intervention, longer training is unlikely to fix it.
- If a probe shows separable useful vs harmful movement while preserving head
  overlap, then it deserves a larger training run.

M713 is exactly that separability check.

## Method

For each query, M713 samples candidate atoms from four groups:

- `missed_target`: dense-boundary target atoms visible but outside b384.
- `captured_target`: target atoms already inside b384.
- `selected_non_target`: non-target atoms currently selected inside b384.
- `head_other`: fallback head atoms not already sampled by the other groups.

Each sampled atom is added or boosted alone in the query atom vector, then the
native semantic posting scorer is rerun. The audit records dense-boundary pair
movement:

- `fixed_pairs`: baseline-failed dense-boundary pairs fixed by the atom.
- `regressed_pairs`: baseline-success pairs broken by the atom.
- `top95 overlap`: preservation of the baseline native head.

## Runs

| Run | Datasets | Queries / dataset | Atoms / query | Scale |
| --- | --- | ---: | ---: | ---: |
| smoke | fiqa, scidocs | 5 | 24 | 0.02 |
| canary | arguana, cqadupstack, fiqa, scidocs | 10 | 32 | 0.05 |
| canary_s01 | arguana, cqadupstack, fiqa, scidocs | 10 | 32 | 0.10 |
| canary_s02 | arguana, cqadupstack, fiqa, scidocs | 10 | 32 | 0.20 |

## Eval Surface Summary

| Scale | Group | Tested | Useful | Harmful | Fixed | Regressed | Pair success | Top95 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.05 | missed_target | 141 | 0 | 0 | 0 | 0 | 0.513538 -> 0.513538 | 0.999776 |
| 0.05 | selected_non_target | 96 | 0 | 4 | 0 | 4 | 0.532609 -> 0.527174 | 0.995504 |
| 0.10 | missed_target | 141 | 4 | 3 | 4 | 3 | 0.513538 -> 0.514440 | 0.998358 |
| 0.10 | selected_non_target | 96 | 1 | 8 | 1 | 8 | 0.532609 -> 0.523098 | 0.987500 |
| 0.20 | missed_target | 141 | 5 | 4 | 5 | 4 | 0.513538 -> 0.514440 | 0.995371 |
| 0.20 | selected_non_target | 96 | 2 | 13 | 2 | 13 | 0.532609 -> 0.517663 | 0.976974 |

## Interpretation

M713 finds a weak but real separability signal:

- At low scale, missed target atoms barely move native ranks.
- At scale 0.10 and 0.20, missed target atoms become more useful than selected
  non-target atoms.
- Non-target atoms become strongly harmful as scale increases.
- The signal is not yet strong enough to justify a broad training run: eval
  missed-target fixed/regressed is only `4/3` at scale 0.10 and `5/4` at scale
  0.20.
- The all-atom surface is unsafe at higher scale because head preservation
  drops, especially for non-target atoms.

The practical conclusion is that training depth alone is not the main blocker.
There is a target-specific movement signal, but the model must learn a safe
selector or impact controller. Simply training longer on the current scalar
features or expanding all atoms is likely to amplify harmful atoms.

## Decision

Keep the route alive, but do not scale it as a generic selector yet.

The next probe should be a pair-impact selector with strict head-overlap gates:

- Train only on atom-level pair-impact labels from direct native interventions.
- Separate target-like useful atoms from selected non-target harmful atoms.
- Use scale or impact as a predicted quantity, not a fixed global constant.
- Require eval improvement with `top95 >= 0.995` for target-selected atoms and
  no aggregate dense-head regression.

Stop the route if a selector cannot beat the M713 direct labels on held-out
queries, or if gains require selecting non-target atoms that reduce top95
overlap.

## Files

- `scripts/audit_m713_single_atom_pair_impact.py`
- `runs/m713_single_atom_pair_impact_smoke_v1/m713_summary.json`
- `runs/m713_single_atom_pair_impact_canary_v1/m713_summary.json`
- `runs/m713_single_atom_pair_impact_canary_s01_v1/m713_summary.json`
- `runs/m713_single_atom_pair_impact_canary_s02_v1/m713_summary.json`
