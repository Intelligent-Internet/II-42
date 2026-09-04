# M1239 Query Gate for Safe Added Atoms Report

M1239 tested the structural decomposition implied by M1238: first predict
whether a query needs safe added atoms, then apply the existing `source_abs`
top8 atom selector only for gated queries.

## Inputs

- Label: query has at least one safe CUB target atom.
- Features:
  - `native_only`: native rank/context features.
  - `native_source`: native features plus source candidate aggregate features.
- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.
- Output:
  `runs/m1239_query_gate_for_safe_added_atoms_smoke_v1/m1239_query_gate_for_safe_added_atoms.json`

## Smoke Result

Ungated source baseline:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |

Query-gated variants:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `native_only_double_train_rate` | 0.8341 | 0.1797 | 0.0070 | 0.1728 | 4.044 |
| `native_source_double_train_rate` | 0.8203 | 0.1702 | 0.0067 | 0.1635 | 4.201 |
| `native_source_train_rate` | 0.4931 | 0.1798 | 0.0050 | 0.1748 | 2.390 |
| `native_only_train_rate` | 0.4009 | 0.1611 | 0.0019 | 0.1593 | 2.169 |

Fold AUC:

| Dataset | NativeOnly | NativeSource |
| --- | ---: | ---: |
| `cqadupstack` | 0.3403 | 0.4352 |
| `scidocs` | 0.5369 | 0.6294 |
| `webis-touche2020` | 0.5982 | 0.6316 |

## Interpretation

The query gate does not work with the current feature set.  It reduces proposal
count and sometimes harm, but mostly by dropping target recall.  The best
high-coverage gate still lowers recall from `0.9770` to `0.8341` and has a
worse separation gap than the ungated source baseline.

The fold AUC confirms this is not a threshold-only issue.  Query-level need is
not reliably observable from native rank summary plus source aggregate features,
especially on `cqadupstack`.

## Decision

Stop M1239 at smoke.

- Do not run full shared15.
- Do not run native replay.
- Do not continue with query-level gate variants over the same feature set.

M1238 still stands: target atoms are mostly safe.  M1239 says the current
query-level features cannot decide when to add them.  The next change must
modify the proposal source or use a stronger query-conditioned teacher, not
another gate threshold.
