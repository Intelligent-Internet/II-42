# M1209 Row-Local Veto On Top3 Scale 0.075

M1209 reruns the M1207 row-local observability veto with a safer aggressive
action: `top3_s0.075` instead of `top3_s0.10`.

This tests whether the best M1208 fixed-scale operating point becomes usable
after learned qrels-free damage veto.

## Setup

- Surface: native shared15 cache
- Query count: 1342
- Aggressive action: `top3_s0.075`
- Conservative action: `top1_s0.10`
- Model: leave-one-dataset-out logistic veto
- Features: qrels-free row-local movement observability

## Macro Result

| Variant | Take | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_pair` | 0.855 | 0 | +0.002327 | +0.004891 | +0.005091 | +0.004254 | +0.000443 |
| `aggressive` | 1.000 | 1 | +0.001610 | +0.003509 | +0.003358 | +0.002682 | -0.000033 |
| `veto_utility_or_cub_logistic` | 0.770 | 0 | +0.001732 | +0.003119 | +0.002319 | +0.002661 | +0.000056 |
| `veto_any_metric_logistic` | 0.779 | 0 | +0.001716 | +0.003082 | +0.002252 | +0.002428 | +0.000032 |
| `veto_rank_metric_logistic` | 0.735 | 0 | +0.001406 | +0.003186 | +0.002543 | +0.002689 | +0.000061 |
| `conservative` | 1.000 | 0 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## Interpretation

This is safety-positive but not a new best route.

The learned veto turns `top3_s0.075` into a macro-safe policy and makes CUB
positive.  However, it gives back too much NDCG/MAP versus fixed
`top3_s0.075`, and it still does not beat the existing M1191 damage-veto route
overall.

The result is still useful: it shows that the 0.075 action is a good
intermediate safety action.  The problem is not the action itself; the problem
is choosing between only two actions.  A binary veto has to collapse too much
risk into either full `top3` or full `top1`.

## Decision

- Do not promote M1209 over M1191.
- Keep M1209 as evidence for a three-level ordered-risk policy.
- Stop binary veto tuning on the same action pair.

## Next Direction

Move to M1210:

- action 0: `top3_s0.10` for low-risk/high-upside queries
- action 1: `top3_s0.075` for medium-risk queries
- action 2: `top1_s0.10` for high-risk queries

Acceptance for M1210 should be strict:

- macro Recall/MAP/NDCG/MRR all positive
- macro CUB non-negative
- better than M1191 on at least two of Recall, MAP, NDCG, MRR
- no increase in dataset-level negative metric cells versus M1191

If M1210 fails, the current local policy route should stop and the next stage
should move to a different proposal generator or broader native replay.

## Artifacts

- JSON: `runs/m1209_row_local_veto_top3_s0075_v1/m1207_row_local_observability_veto.json`
- Markdown: `runs/m1209_row_local_veto_top3_s0075_v1/m1207_row_local_observability_veto.md`
- Script: `scripts/audit_m1207_row_local_observability_veto.py`
