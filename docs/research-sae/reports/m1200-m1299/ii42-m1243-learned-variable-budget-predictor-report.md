# M1243 Learned Variable-Budget Predictor Report

M1243 tested whether the M1241 oracle variable-budget signal can be made
deployable with a learned query-level budget predictor.  The predictor only
chooses budget; atom ordering remains `source_abs`.

## Inputs

- Label: safe CUB target atom count per query.
- Features:
  - `curve_only`: top16 source score curve, normalized scores, gaps,
    cumulative mass.
  - `curve_native`: score curve plus native rank/context features.
- Models:
  - presence: balanced logistic regression.
  - count: random forest regressor.
- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.
- Output:
  `runs/m1243_learned_variable_budget_predictor_smoke_v1/m1243_learned_variable_budget_predictor.json`

## Smoke Result

Baseline `fixed_top8`:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |

Best learned variants:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `curve_native_learned_count_plus1` | 0.6175 | 0.2524 | 0.0075 | 0.2448 | 2.133 |
| `curve_only_learned_count_plus1` | 0.5899 | 0.2700 | 0.0084 | 0.2616 | 1.904 |
| `curve_native_learned_count_x2` | 0.5576 | 0.2101 | 0.0052 | 0.2049 | 2.313 |
| `curve_native_learned_presence_top8` | 0.5207 | 0.1906 | 0.0051 | 0.1855 | 2.382 |

Fold diagnostics:

| Dataset | Group | PresenceAUC | CountMAE |
| --- | --- | ---: | ---: |
| `cqadupstack` | `curve_native` | 0.5189 | 1.7051 |
| `cqadupstack` | `curve_only` | 0.5787 | 1.4728 |
| `scidocs` | `curve_native` | 0.7047 | 0.9613 |
| `scidocs` | `curve_only` | 0.6619 | 0.9591 |
| `webis-touche2020` | `curve_native` | 0.5386 | 1.8168 |
| `webis-touche2020` | `curve_only` | 0.4719 | 1.4286 |

## Interpretation

The learned budget predictor does not recover the M1241 oracle behavior.  It
can improve precision by choosing fewer atoms, but target recall collapses from
`0.9770` to at most `0.6175` in the tested learned policies.

The fold diagnostics explain why: safe-target presence and count are not
reliably observable from source score curve plus native rank summaries.  The
signal is somewhat learnable on `scidocs`, but unstable on `cqadupstack` and
`webis-touche2020`.

## Decision

Stop M1243 at smoke.

- Do not run full shared15.
- Do not deepen this budget-predictor training shape.
- Keep M1241 as an oracle upper bound, but treat deployable budget prediction
  from current features as failed.

The next useful route should change the proposal/teacher source so that
safe-target presence is observable without qrels, or introduce a stronger
query-conditioned signal.  More score-curve/native-feature budget models are
likely to repeat M1239/M1243 failure.
