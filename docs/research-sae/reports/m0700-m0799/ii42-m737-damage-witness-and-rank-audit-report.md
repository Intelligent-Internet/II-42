# M737 Damage-Witness And Rank Audit

## Purpose

M736 proved that oracle positive-enriched bundles can improve native retrieval
while preserving dense/CUB gates, but deployable model-selected bundles still
admit dense-overlap damage.  M737 tests whether a damage-witness selector can
filter the bad rows and whether the remaining failure is a ranking problem or a
bundle interaction problem.

## Artifacts

| Artifact | Path |
| --- | --- |
| Damage-witness selector JSON | `ii42-m737-damage-witness-selector.json` |
| Damage-witness selector report | `docs/research-sae/reports/m0700-m0799/ii42-m737-damage-witness-selector-report.md` |
| Selected holdout rows | `ii42-m737-selected-bundle-rows.jsonl` |
| Single-atom replay JSON | `ii42-m737b-single-atom-model-replay.json` |
| Single-atom replay report | `docs/research-sae/reports/m0700-m0799/ii42-m737b-single-atom-model-replay-report.md` |

## M737A Damage-Witness Selector

M737A trains two global logistic models from M736C native replay labels:

- productive bundle classifier
- dense/CUB damage classifier

Features are restricted to pre-replay bundle features and M735B model score
aggregates.  Replay metrics are only used as labels and acceptance criteria.

| Split | Rows | Productive + | Damage + | Productive AUC | Damage AUC |
| --- | ---: | ---: | ---: | ---: | ---: |
| train | 170 | 3 | 9 | 0.946108 | 0.798482 |
| holdout | 29 | 1 | 2 | 0.785714 | 0.722222 |

The selected threshold passed train gate:

| Split | Rows | Productive | Damage | dMAP | dO@100 | dO@256 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| train | 16 | 3 | 0 | +0.000044 | +0.000000 | +0.000488 | 1 |
| holdout | 5 | 0 | 1 | +0.000000 | +0.000000 | +0.000000 | 1 |

Interpretation: damage filtering became safer, but the selector no longer kept
productive holdout bundles.  This is a useful negative: damage witness helps
avoid harm, but current productive features are too weak.

## M737B Single-Atom Ablation

M736C selected two atoms per query.  M737B reran the same expanded replay with
`max_bundle_atoms=1`.

| Surface | Split | Rows | Productive | dMAP | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| model_productive_bundle | holdout | 29 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| oracle_productive_bundle | holdout | 5 | 5 | +0.007166 | +0.000000 | +0.002000 | +0.002344 | 1 |

Interpretation: reducing to one atom removes aggregate damage, but it also
removes all model-selected productive holdout rows.  The failure is not only
bundle interaction; the atom scorer itself is selecting the wrong atom.

## Productive Atom Rank Audit

Using the expanded M736B atom models, holdout productive atoms were not ranked
near the top:

| Query | Productive atom | Rank / candidates | Note |
| --- | ---: | ---: | --- |
| scidocs `2bd338...` | 14 | 5 / 10 | oracle productive, model bundle chose 65/608 |
| scidocs `3a636...` | 125 | 4 / 8 | strongest recall-positive atom |
| scidocs `44a56...` | 33 | 6 / 10 | oracle productive, model bundle chose 125/207 |
| scidocs `5bb6...` | 125 | 8 / 8 | oracle productive, model bundle chose 10/65 |
| cqadupstack `android_29769` | 818 | 9 / 10 | oracle productive, model bundle chose 178/7 |

The current scorer often assigns high danger probability to the actual
productive atom.  This explains why threshold tuning and damage filtering cannot
recover oracle behavior.

## Decision

M737 should not be expanded to shared15.  It did not produce a deployable
selector.

The route is still alive because M736 oracle bundles are strong, but the next
work must change the productive atom teacher/interface.  The current feature
surface does not rank productive atoms high enough.

## Next Step

M738 should focus on source-conditioned productive atom ranking:

1. Build positive/negative pairs inside each query from M736B atom replay rows.
2. Train a pairwise or listwise atom ranker, not independent atom logistic.
3. Add features that compare atoms within the same query: relative fanout,
   relative boundary impact, source pool membership, and score margin to the
   current top atom.
4. Replay top-1 and top-2 bundles through native path.
5. Stop unless holdout keeps productive rows and clears O@100/O@256/CUB gates.

This is the next meaningful iteration.  More depth on the existing independent
atom classifier is unlikely to fix the observed rank inversion.
