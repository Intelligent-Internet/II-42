# M1196 Constrained Veto Threshold

## Purpose

M1195 showed that missed harmful queries often had moderate harm probability.
M1196 tests whether changing only threshold selection can improve hard-row harm
recall while retaining the M1191 macro contract.

It uses the same cached M1190 examples and the same logistic
`utility_or_cub` harm model.  No native DB query is rerun.

## Artifacts

- Script: `scripts/audit_m1196_constrained_veto_threshold.py`
- JSON:
  `runs/m1196_constrained_veto_threshold_v1/m1196_constrained_veto_threshold.json`
- Markdown:
  `runs/m1196_constrained_veto_threshold_v1/m1196_constrained_veto_threshold.md`

## Macro

| Variant | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_pair` | 0.852 | +0.002599 | +0.005643 | +0.005712 | +0.004721 | +0.000419 |
| `aggressive` | 1.000 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `constrained_veto` | 0.805 | +0.002284 | +0.003391 | +0.002215 | +0.002211 | -0.000062 |
| `conservative` | 1.000 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## Interpretation

Constrained threshold selection does not pass.

It improves some hard-row harm recall:

- `scidocs`: 0.129 -> 0.290
- `webis-touche2020`: 0.083 -> 0.250
- `trec-covid`: 0.400 -> 0.533

But the macro contract fails:

- CUB remains negative (-0.000062)
- MAP/NDCG are below M1191
- it does not beat the M1191 `veto_utility_or_cub_logistic` frontier

## Decision

Stop threshold-objective tuning for now.

M1195/M1196 together show:

1. There is risk signal in existing qrels-free features.
2. Moving the threshold can catch more harm.
3. Existing features/proposals cannot catch enough harm without giving back
   too much macro quality.

Therefore, the bottleneck is not simply threshold selection.  It is missing
observability or insufficient proposal structure.

## Next Direction

Do not test more thresholds.

Do not add naive tail/dual-tail proposal families.

Next valuable work should be a targeted mechanics audit on hard rows:

- inspect candidate movement for cqadupstack/scidocs/webis-touche2020
- capture which documents enter/leave top100 under top3 and veto
- identify whether qrels positives are displaced by lexical-only entrants,
  semantic-low-rank entrants, or score-normalization shifts

If this exposes a qrels-free feature, add it to the veto.  If not, the current
proposal family should be treated as locally capped.
