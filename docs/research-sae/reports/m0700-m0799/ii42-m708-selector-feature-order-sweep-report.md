# M708 Selector Feature Order Sweep Report

M708 tested whether existing expanded-interface candidate features contain a
simple ordering that can recover the M704 oracle target atoms.

This is a no-training first-stage audit:

- no BM25
- no reranker
- no qrels loss
- no native reranking sweep
- no learned gate

## Context

M704 remains the positive upper bound:

| Version | Budget | Target/oracle result |
| --- | ---: | --- |
| M704 | 384 | pair success `0.541176 -> 0.617647`, fixed/regressed `234/0` |

M706 showed M705 misses too much target coverage:

| Version | Budget | Target recall | Oracle target recall |
| --- | ---: | ---: | ---: |
| M706 | 384 | 0.701639 | 0.982671 |

M707 tried query-conditioned features and a risk model, but got worse:

| Version | Budget | Target recall | Oracle target recall |
| --- | ---: | ---: | ---: |
| M707 | 384 | 0.596205 | 0.982671 |

M708 asks whether a simple feature ordering can do better before training a
new model.

## Run

```bash
python3 scripts/audit_m708_selector_feature_order_sweep.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --budgets 24,48,96,192,384 \
    --output-root runs/m708_selector_feature_order_sweep_v1
```

Output:

- `runs/m708_selector_feature_order_sweep_v1/m708_summary.json`
- `runs/m708_selector_feature_order_sweep_v1/m708_report.md`

## Eval Result

Best eval feature ordering:

| Variant | Target visibility | Target recall | Oracle target recall | Gap | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: |
| `abs_vote_sum_b384` | 0.982671 | 0.537679 | 0.982671 | 0.444992 | 0.048033 |

Many heuristic orderings tie at the same result:

- `abs_vote_sum`
- `candidate_abs_impact`
- `candidate_positive_impact`
- `score_weight_sum`
- `same_sign_vote_sum`
- `risk_penalized_support`
- `weighted_same_support`

The tie is not because features are absent. A spot check confirmed the features
exist and vary. The issue is that these scalar features are highly co-linear in
the expanded candidate path, so their top-budget selections are effectively the
same.

## Interpretation

M708 rules out a simple explanation:

- M704 oracle succeeds because target membership/order is special.
- M705/M707 do not recover that target membership.
- Existing scalar feature orderings also do not recover it.

Therefore the next useful probe is not "train longer" or "sort by another
scalar". The current feature space does not expose enough separability for
oracle target atoms.

## Decision

Do not scale M707 or a simple feature-rank selector.

The next attempt should change the supervision or representation:

1. Pair-impact supervision: train on whether an atom fixes/regresses
   dense-boundary pairs under native scoring.
2. Oracle-order distillation: reproduce the M704 safe-fill target order as a
   listwise target instead of pointwise target membership.
3. Candidate representation upgrade: add features derived from target/source
   doc interactions or atom co-occurrence, not only scalar atom popularity and
   impact.

M708 strengthens the conclusion that the M704 route is real but the current
selector feature space is under-specified.

