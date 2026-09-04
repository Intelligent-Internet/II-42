# M1240 Added-Atom Budget Curve Report

M1240 tested whether the CUB-specific added-atom selector is mainly
over-budgeted inside each query.  It evaluates source/prior atom rules at
several per-query budgets before launching any replay.

## Inputs

- Label surface: M1224 CUB-specific target/harm atoms.
- Rules: `source_abs`, `prior_x_source`, `agreement_abs`, `prior_contrast`.
- Budgets: `top1`, `top2`, `top4`, `top6`, `top8`, `top12`, `top16`.
- Full output:
  `runs/m1240_added_atom_budget_curve_v1/m1240_added_atom_budget_curve.json`

## Full Shared15 Result

Key `source_abs` curve:

| Budget | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| top1 | 0.1178 | 0.2578 | 0.0373 | 0.2206 | 1.000 |
| top2 | 0.2245 | 0.2455 | 0.0339 | 0.2116 | 2.000 |
| top4 | 0.4302 | 0.2354 | 0.0304 | 0.2050 | 3.999 |
| top6 | 0.6192 | 0.2261 | 0.0296 | 0.1965 | 5.991 |
| top8 | 0.7793 | 0.2138 | 0.0298 | 0.1840 | 7.975 |
| top12 | 0.8781 | 0.1627 | 0.0298 | 0.1328 | 11.808 |
| top16 | 0.9608 | 0.1393 | 0.0289 | 0.1105 | 15.085 |

## Interpretation

The smoke surface made `top4` look promising, but the full shared15 curve does
not support promoting a fixed lower budget.  Lower budgets improve precision
and separation, but recall collapses too far:

- `top4` gap improves from `0.1840` to `0.2050`, but target recall drops from
  `0.7793` to `0.4302`.
- `top6` is a softer tradeoff, but still drops recall to `0.6192`.
- `top12/top16` recover recall, but precision and gap collapse.

So fixed budget is not a breakthrough.  It confirms the true shape of the
problem: each query needs a variable atom budget and better within-query atom
ordering.  A global budget can only trade recall for precision.

## Decision

Stop M1240 as a direct policy.

- Do not run native replay with fixed top4/top6.
- Keep the curve as evidence for a future variable-budget selector.
- Do not continue fixed-budget grid search.

The next viable route must predict query-local atom budget and atom ordering
together, or change proposal source so that high-recall budgets do not flood
non-target atoms.
