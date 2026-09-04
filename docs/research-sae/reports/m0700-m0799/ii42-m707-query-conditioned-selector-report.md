# M707 Query-Conditioned Selector Report

M707 tested whether the M705/M706 failure can be fixed by adding per-query
normalization and a separate negative-only risk model.

This remains a first-stage dense-equivalence probe:

- no BM25
- no reranker
- no qrels loss
- no learned gate
- no doc posting geometry change

M707 only audits target coverage before native reranking. It intentionally does
not run native evaluation unless target recovery improves enough.

## Background

M704 proved the expanded interface has a strong oracle:

| Route | Pair success | Fixed/Regr. | Top95 |
| --- | ---: | ---: | ---: |
| `safe_fill_delta_target_b384_s0.02` | `0.541176 -> 0.617647` | `234/0` | `0.967023` |

M706 showed why M705 did not recover that oracle:

| Budget | Learned target recall | Oracle target recall | Gap |
| ---: | ---: | ---: | ---: |
| 384 | 0.701639 | 0.982671 | 0.281032 |

M707 asks whether query-conditioned features and a risk model close this gap.

## Method

M707 uses the same expanded candidate interface:

- `source384`
- `doc_atom_head=48`
- `max_atom_candidates_per_query=1536`

Changes relative to M705:

- raw atom features are augmented with per-query z-scores;
- raw atom features are augmented with per-query descending percentiles;
- a target classifier predicts target atom probability;
- a separate risk classifier predicts negative-only atom probability;
- variants rank by `target_probability - risk_weight * risk_probability`.

Canary run:

```bash
python3 scripts/train_m707_query_conditioned_selector.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --max-atom-train-rows 0 \
    --budgets 24,48,96,192,384 \
    --risk-weights 0,0.25,0.5,1 \
    --output-root runs/m707_query_conditioned_selector_v1
```

Output:

- `runs/m707_query_conditioned_selector_v1/m707_summary.json`
- `runs/m707_query_conditioned_selector_v1/m707_report.md`

## Model Stats

| Rows | Positives | Positive share | Target AUC | Risk positives | Risk share | Risk AUC |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 388608 | 9553 | 0.024583 | 0.849174 | 8571 | 0.022056 | 0.833207 |

## Eval Result

Best eval variant:

| Variant | Learned target recall | Oracle target recall | Gap | Negative-only |
| --- | ---: | ---: | ---: | ---: |
| `qcond_b384_rw0` | 0.596205 | 0.982671 | 0.386466 | 0.052659 |

The result is worse than M706's simple pointwise selector at budget 384:

| Version | Budget | Learned target recall |
| --- | ---: | ---: |
| M706 | 384 | 0.701639 |
| M707 | 384 | 0.596205 |

Risk weights did not change the selected rows in the top variants. The target
model dominates the ranking, and the risk model does not solve the target
coverage problem.

## Decision

M707 does not pass the attribution gate and should not be sent to native
reranking.

This is not evidence that the M704 route is dead. It narrows the failure:

- target atoms are visible;
- M704 oracle can safely use them;
- M705 pointwise selector misses too many;
- M707 query-conditioned/risk features miss even more.

The next attempt should not add more generic features to the same classifier.
The next attempt should train directly on the missing decision:

1. Pair-impact supervision: label atoms by whether they fix or regress
   dense-boundary pairs under native scoring.
2. Oracle-order distillation: train to reproduce the M704 safe-fill order, not
   just target membership.
3. Setwise coverage objective: reward recovering diverse target atoms within a
   fixed budget, because independent atom scoring is not enough.

M707 is a useful stop signal for the query-conditioned HGB variant. Continue
from M704/M706, but do not scale M707.

