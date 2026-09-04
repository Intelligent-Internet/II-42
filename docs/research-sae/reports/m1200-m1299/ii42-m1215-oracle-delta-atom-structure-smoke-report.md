# M1215 Oracle-Delta Atom Structure Smoke

M1215 checks whether M1213 oracle-winning actions expose a structured
generated-posting target before launching larger training.

This is a hard-row smoke only.  The full shared15 run was not launched because
the smoke does not show a clean positive target structure.

Datasets:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Labels:

- `positive_winner`: non-baseline action selected by oracle_best4
- `negative_harm`: non-baseline action with negative protected score delta
- `other_nonwinner`: non-baseline action that is not oracle-best and not
  protected-score harmful

Atoms are query-atom deltas relative to the baseline query.

## Smoke Summary

| Label | Instances | Mean ScoreDelta | Mean AtomCount | Mean Top3Share | AtomCount | Top50Share | RepeatShare |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `negative_harm` | 205 | -0.700550 | 13.873 | 0.270 | 939 | 0.273 | 0.878 |
| `other_nonwinner` | 427 | +0.041876 | 12.527 | 0.318 | 1223 | 0.230 | 0.940 |
| `positive_winner` | 115 | +0.201620 | 12.357 | 0.311 | 708 | 0.281 | 0.721 |

Positive/negative atom Jaccard:

- `0.443471`

## Interpretation

The positive target is not clean enough.

There is a weak positive sign:

- `positive_winner` Top50Share is slightly higher than `negative_harm`
  (`0.281` vs `0.273`).
- positive instances have positive protected score delta by construction.

But the stronger evidence is negative:

- `positive_winner` RepeatShare is much lower than `negative_harm`
  (`0.721` vs `0.878`).
- positive and negative atom sets overlap heavily (`Jaccard 0.443`).
- top atoms are similar across labels: atom ids such as `10`, `65`, `154`,
  `37`, and `608` appear in both positive and negative groups.
- positive deltas are not more compact per instance than other nonwinners.

This means the M1213 oracle is mostly selecting among context-dependent
movements, not exposing a simple reusable atom dictionary target.

## Decision

- Do not launch a larger generated-posting model from this naive oracle-delta
  target.
- Do not continue local selector or raw atom-prior variants.
- Treat current evidence as pointing to a missing teacher definition, not a
  missing model capacity.

## What This Rules Out

This rules out the immediate plan:

> Use M1213 oracle-winning action atoms directly as positive generated-posting
> targets.

That target is too entangled with harm atoms and would likely train another
model that looks good in-fold but fails held-out rows.

## Next Direction

The next route needs a stronger teacher before training:

1. Separate atom direction from atom identity.
2. Condition target atoms on boundary document movement, not only oracle action.
3. Penalize atoms that also appear in `negative_harm` contexts.
4. Build a contrastive teacher over positive-vs-harm atom contributions before
   any larger model.

The smallest next experiment should be a contrastive atom teacher audit:

- keep only atoms whose positive expected contribution exceeds negative harm
  contribution by a margin
- replay a synthetic delta from those filtered atoms
- check whether the filtered target preserves CUB and improves hard-row Recall

If contrastive filtering cannot produce a safe replay target, generated-posting
training should pause and the project should return to the current M1191/M1210
frontier plus engineering/native-index validation.

## Artifacts

- Script: `scripts/audit_m1215_oracle_delta_atom_structure.py`
- Smoke JSON: `runs/m1215_oracle_delta_atom_structure_smoke_v1/m1215_oracle_delta_atom_structure.json`
- Smoke Markdown: `runs/m1215_oracle_delta_atom_structure_smoke_v1/m1215_oracle_delta_atom_structure.md`
