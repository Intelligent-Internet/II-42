# M1202 Combined Guard Shared15

M1202 tests whether qrels-free movement features can turn the M1201
consensus atom proposal into a safer native unified-posting action on
full shared15.  The action is still query-local posting delta or baseline
fallback; this is not a reranker.

## Setup

- Surface: shared15 native replay
- Query count: 1342
- Feature count: 38 qrels-free movement features
- Label mode: `contract_harm`
- Baseline: frozen P1 native query
- Proposal: M1199 `consensus2_mean_s0.10`
- Fixed guard anchor: M1201 `rule_o1000_099`

## Macro Result

| Source | Take | Macro negative metrics | Dataset negative cells | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `proposal` | 1.000 | 1 | 7 | +0.000379 | +0.000833 | +0.000347 | +0.000938 | -0.000035 |
| `rule_o1000_099` | 0.882 | 0 | 9 | +0.000144 | +0.000560 | +0.000201 | +0.000306 | +0.000059 |
| `model_veto` | 0.756 | 0 | 6 | +0.000061 | +0.000336 | +0.000223 | +0.000709 | +0.000113 |

## Interpretation

This is a real but bounded positive signal.  The raw proposal gives the
largest Recall/MAP/NDCG/MRR lift but still regresses CUB, so it is not safe
enough as a default.  The fixed M1201 guard remains the best simple baseline:
it keeps all macro deltas positive and preserves more Recall/MAP than the
learned veto.

`model_veto` should still be retained.  It improves CUB, MRR, and NDCG
relative to the fixed guard, and it reduces dataset-level negative metric
cells from 9 to 6.  The cost is lower Recall/MAP and a lower take rate.  This
makes it a Pareto-adjacent safety candidate, not the default.

The label is useful enough to keep, but it is not sufficient to solve the
remaining row-level harms.  The remaining negative cells are concentrated in
`cqadupstack`, `dbpedia-entity`, `fiqa`, and `trec-covid`.  The next useful
step is not another threshold-only guard pass; it is proposal source redesign
or a guard trained directly against row-local harm modes.

## Decision

- Keep M1201 `rule_o1000_099` as the fixed shared15 baseline.
- Carry M1202 `model_veto` forward as a learned safety candidate.
- Do not promote `proposal` despite its higher score because it still has a
  negative CUB delta.
- Do not continue label swapping unless it changes the frontier on shared15.

## Artifacts

- JSON: `runs/m1202_combined_guard_shared15_v1/m1202_combined_guard_shared15.json`
- Markdown: `runs/m1202_combined_guard_shared15_v1/m1202_combined_guard_shared15.md`
- Script: `scripts/audit_m1202_combined_guard_shared15.py`
