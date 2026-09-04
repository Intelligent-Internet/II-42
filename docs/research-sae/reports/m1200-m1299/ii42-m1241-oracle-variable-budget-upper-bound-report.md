# M1241 Oracle Variable-Budget Upper Bound Report

M1241 tested whether oracle per-query atom budgets can make the existing
source ordering usable.  This follows M1240: fixed low budgets improve
precision but lose too much recall, so the next question is whether variable
budget is the missing structure.

## Inputs

- Label surface: M1224 CUB-specific target/harm atoms.
- Ranking rules: `source_abs`, `agreement_abs`, `prior_x_source`.
- Policies:
  - fixed budgets: top4/top6/top8/top12.
  - oracle presence: only add atoms for queries with safe targets.
  - oracle safe/target budgets: exact, plus1, plus2, x2.
- Full output:
  `runs/m1241_oracle_variable_budget_upper_bound_v1/m1241_oracle_variable_budget_upper_bound.json`

## Baseline

`source_abs_fixed_top8`:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.7793 | 0.2138 | 0.0298 | 0.1840 | 7.975 |

## Best Oracle Variable-Budget Policies

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `source_abs_oracle_safe_plus1` | 0.7899 | 0.7008 | 0.0175 | 0.6833 | 2.466 |
| `source_abs_oracle_target_plus1` | 0.7980 | 0.6984 | 0.0292 | 0.6692 | 2.500 |
| `source_abs_oracle_safe_plus2` | 0.8130 | 0.6408 | 0.0196 | 0.6212 | 2.776 |
| `source_abs_oracle_safe_x2` | 0.9435 | 0.4878 | 0.0245 | 0.4634 | 4.231 |
| `source_abs_oracle_presence_top8` | 0.7725 | 0.6823 | 0.0274 | 0.6549 | 2.477 |

## Rank Diagnostics

For `source_abs`, all safe target atoms are present in the candidate rows, but
the target ranks are spread:

| Present | Missing | RankP50 | RankP90 | Cov@4 | Cov@8 | Cov@16 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1.0000 | 0.0000 | 5.00 | 13.80 | 0.4252 | 0.7764 | 0.9602 |

## Interpretation

This is a real upper-bound signal.  The failure is not that source ordering
lacks target atoms: safe target atoms are always present.  The failure is that
fixed budget adds too many non-target atoms on some queries and too few on
others.

The strongest point is `source_abs_oracle_safe_plus1`: it slightly improves
target recall over fixed top8 (`0.7899` vs `0.7793`) while increasing separation
gap from `0.1840` to `0.6833`.  `source_abs_oracle_safe_x2` recovers much more
recall (`0.9435`) while still keeping a much better gap than fixed top8.

This means variable-budget/presence is a valid structure.  The remaining
problem is deployability: M1239 showed current query-level features cannot
predict safe-target presence well.  The next step should not be another atom
classifier; it should test qrels-free budget proxies from the source score
curve, then only train a budget predictor if a deployable proxy has signal.

## Decision

Promote variable-budget policy as the next main line.

- Do not replay oracle policies directly; they use labels.
- Build M1242 qrels-free score-curve/elbow budget proxy.
- If M1242 can recover a meaningful fraction of the oracle gap without qrels,
  then train a learned variable-budget predictor.
- If M1242 fails, the bottleneck is deployable budget observability, not atom
  ordering.
