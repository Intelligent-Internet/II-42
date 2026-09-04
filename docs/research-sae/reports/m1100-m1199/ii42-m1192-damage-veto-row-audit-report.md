# M1192 Damage-Veto Row Audit

## Purpose

M1191 found a useful damage-veto gate:

- default to aggressive BM25-top docs3 posting delta
- veto to conservative BM25-top docs1 when predicted harmful
- use qrels only for training labels, not runtime features

M1192 audits row-level behavior to decide whether the next step should improve
the gate or change proposal generation.

## Artifacts

- Script: `scripts/audit_m1192_damage_veto_row_audit.py`
- Input cache:
  `runs/m1190_qrels_free_risk_feature_gate_v1/m1190_qrels_free_risk_feature_gate.json`
- JSON:
  `runs/m1192_damage_veto_row_audit_v1/m1192_damage_veto_row_audit.json`
- Markdown:
  `runs/m1192_damage_veto_row_audit_v1/m1192_damage_veto_row_audit.md`

## Macro

| Source | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_pair` | 0.852 | +0.002599 | +0.005643 | +0.005712 | +0.004721 | +0.000419 |
| `aggressive` | 1.000 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `veto` | 0.849 | +0.002065 | +0.003412 | +0.002557 | +0.002157 | +0.000002 |
| `conservative` | 1.000 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## What Improved

The damage-veto gate is not just macro noise.

- `dbpedia-entity`: CUB changes from -0.000262 under aggressive to +0.000100
  under veto.
- `nfcorpus`: CUB changes from -0.000614 under aggressive to +0.000600 under
  veto, while recall remains positive.
- `trec-covid`: NDCG changes from -0.003314 under aggressive to +0.001100
  under veto.
- `fiqa`: MRR changes from -0.000414 under aggressive to +0.001100 under veto.

These are real safety improvements and explain why macro CUB becomes
non-negative.

## What Still Fails

The gate is still not enough.

- `cqadupstack`: aggressive hurts all ranking metrics; veto improves recall
  slightly but MAP/NDCG/MRR remain worse than baseline and worse than
  conservative.
- `scidocs`: veto keeps the aggressive recall gain but does not fix CUB
  (-0.002000 remains).
- `webis-touche2020`: NDCG remains slightly negative.

This means current qrels-free features can identify some risk, but not all
row-specific harm.

## Diagnosis

The blocker has shifted.

Earlier failures were about whether retrieval-conditioned posting deltas have
any real signal.  M1188-M1191 answered yes.

M1192 shows the remaining bottleneck is proposal/observability:

- The current aggressive proposal is too narrow: BM25 top3.
- The safe fallback is also too narrow: BM25 top1.
- A gate can only choose between those two shapes; it cannot fix rows where
  both shapes are structurally wrong.

Therefore, the next experiment should not be another gate threshold or heavier
classifier.  It should add one new proposal family at a time while keeping the
M1191 damage-veto contract.

## Decision

Keep M1191/M1192 as the current best control route.

Stop:

- plain overlap gates
- generic accept classifiers
- heavier HGB gate training as default

Continue:

- damage-veto framing
- qrels-free runtime features
- native shared15 validation
- one-proposal-family-at-a-time expansion

## Next Step

M1193 should test a mixed proposal:

- aggressive candidate: BM25 top2 + BM25 tail1
- conservative fallback: BM25 top1
- same append8/scale0.10 delta
- same damage-veto audit contract

Rationale:

- top docs carry high precision and ranking gains
- tail doc may recover CUB/under-ranked positives
- mixing avoids the earlier pure tail instability

Acceptance:

- macro CUB must stay non-negative
- MAP/NDCG/MRR should beat M1191 veto or reduce row harm
- if mixed proposal worsens cqadupstack/scidocs, reject and try a different
  proposal source rather than retuning the gate
