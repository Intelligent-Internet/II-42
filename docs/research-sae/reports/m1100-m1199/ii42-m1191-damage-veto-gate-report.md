# M1191 Damage Veto Gate

## Purpose

M1190 showed that accept classifiers over-admit aggressive deltas because most
queries prefer aggressive over conservative.  M1191 flips the control problem:

- default to aggressive
- train a high-precision damage veto
- fall back to conservative only when predicted harmful

It reuses the M1190 per-query cache and does not rerun native DB queries.

## Artifacts

- Script: `scripts/audit_m1191_damage_veto_gate.py`
- Input cache:
  `runs/m1190_qrels_free_risk_feature_gate_v1/m1190_qrels_free_risk_feature_gate.json`
- Full JSON:
  `runs/m1191_damage_veto_gate_v1/m1191_damage_veto_gate.json`
- Full Markdown:
  `runs/m1191_damage_veto_gate_v1/m1191_damage_veto_gate.md`

## Full Shared15 Macro

| Variant | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_pair` | 0.852 | +0.002599 | +0.005643 | +0.005712 | +0.004721 | +0.000419 |
| `aggressive` | 1.000 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `veto_utility_or_cub_logistic` | 0.849 | +0.002065 | +0.003412 | +0.002557 | +0.002157 | +0.000002 |
| `veto_any_metric_logistic` | 0.849 | +0.001931 | +0.003460 | +0.002509 | +0.002266 | -0.000030 |
| `veto_rank_metric_logistic` | 0.836 | +0.001944 | +0.003360 | +0.002308 | +0.002293 | -0.000019 |
| `veto_utility_logistic` | 0.857 | +0.002078 | +0.003114 | +0.002229 | +0.001554 | -0.000015 |
| `conservative` | 1.000 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## Interpretation

This is a real positive update.

`veto_utility_or_cub_logistic` is the first learned qrels-free gate in this
sequence that:

- makes macro CUB non-negative
- keeps Recall gain slightly above fixed aggressive
- preserves most of aggressive MAP/NDCG/MRR gain
- avoids collapsing to the conservative fallback

Retention versus fixed aggressive:

- Recall: 106%
- MAP: 92%
- NDCG: 78%
- MRR: 100%
- CUB: fixed from -0.000082 to +0.000002

This validates the control framing from M1190: damage-veto is more appropriate
than accept-classification for this route.

## Remaining Gap

The learned veto still does not reach the oracle-pair ceiling:

- oracle pair MAP gain: +0.005643
- veto MAP gain: +0.003412
- oracle pair NDCG gain: +0.005712
- veto NDCG gain: +0.002557

The missing piece is either:

1. insufficient qrels-free observability in current features, or
2. insufficient proposal diversity from the current BM25-top docs3/docs1 pair.

It is not currently evidence for deeper model training; this was learned from
query-time features and already exposes a useful gate.

## Decision

Keep M1191 as the current best learned control shape.

Do not go back to generic accept gates.  Do not scale HGB-style heavy models
for this phase; the interrupted HGB run was too slow relative to signal.

## Next Step

M1192 should do row-level harm analysis and proposal-family expansion under
the same veto contract:

- compare fixed aggressive, conservative, and `veto_utility_or_cub_logistic`
  per dataset
- identify rows where veto improves or fails
- add one new proposal family at a time, not a broad grid
- candidate next proposal: BM25-top plus BM25-tail mixed proposal, with the
  same damage-veto gate

Acceptance for M1192:

- keep CUB non-negative
- beat M1191 on MAP/NDCG or reduce row harm without losing ranking gains
- if expanded proposals only help with dataset-specific behavior, reject them
