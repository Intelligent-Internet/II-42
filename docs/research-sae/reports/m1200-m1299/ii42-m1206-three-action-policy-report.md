# M1206 Three-Action Policy

M1206 tests whether the M1190 `oracle_best3` ceiling can be approached by a
deployable qrels-free policy that chooses one of three native unified-posting
actions:

- baseline P1 query
- conservative `top1` posting delta
- aggressive `top3` posting delta

The run uses the M1190 cache only.  No native DB query is rerun.

## Macro Result

| Variant | Choice shape | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `oracle_best3` | baseline 0.694 / top1 0.112 / top3 0.194 | +0.003013 | +0.006581 | +0.006700 | +0.005578 | +0.000708 |
| `aggressive` | top3 1.000 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `cv_logistic_balanced` | baseline 0.508 / top1 0.227 / top3 0.265 | +0.001288 | +0.002273 | +0.001804 | +0.001583 | -0.000107 |
| `conservative` | top1 1.000 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |
| `cv_logistic` | baseline 0.820 / top1 0.041 / top3 0.139 | +0.000312 | +0.001425 | +0.001295 | +0.001348 | -0.000032 |

## Interpretation

This is a negative result for naive three-action policy learning.

The oracle ceiling is large and still important: a correct three-action policy
would substantially beat fixed top3/top1 and the M1191 damage-veto.  But the
current feature-only multiclass formulation does not learn that policy.  Both
logistic variants keep CUB negative and underperform the existing M1191
`veto_utility_or_cub_logistic` frontier.

This says the issue is not the action space.  The action space is useful.  The
issue is observability/supervision: current query-time features do not expose
enough information to identify when baseline, top1, or top3 is the correct
choice under leave-one-dataset-out validation.

## Decision

- Reject naive multiclass policy as a near-term route.
- Keep M1191 damage-veto as the best deployable control route for now.
- Keep M1206 as evidence that the oracle gap is real but not reachable with
  the current feature set and plain multiclass labels.

## Next Direction

The next useful step must add new observability or a better supervision target,
not another classifier swap.  Candidate directions:

1. Add row-local evidence features from actual entrant/exit score margins and
   positive-displacement mechanics.
2. Train a pairwise harm model specifically for `top3` vs `top1`, then use it
   as a veto, not as multiclass argmax.
3. Revisit proposal construction only if it gives a new observable risk
   feature, not just another tail source.

## Artifacts

- JSON: `runs/m1206_three_action_policy_v1/m1206_three_action_policy.json`
- Markdown: `runs/m1206_three_action_policy_v1/m1206_three_action_policy.md`
- Script: `scripts/audit_m1206_three_action_policy.py`
