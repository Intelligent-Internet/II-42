# M1195 Veto Failure Mechanics

## Purpose

M1195 analyzes why the M1191 damage-veto still misses harmful queries on hard
rows.  It reuses the M1190 cached examples and retrains the same
leave-one-dataset-out `utility_or_cub` logistic veto.

No native DB query is rerun.

## Artifacts

- Script: `scripts/audit_m1195_veto_failure_mechanics.py`
- JSON:
  `runs/m1195_veto_failure_mechanics_v1/m1195_veto_failure_mechanics.json`
- Markdown:
  `runs/m1195_veto_failure_mechanics_v1/m1195_veto_failure_mechanics.md`

## Confusion On Hard Rows

| Dataset | Veto rate | Precision | Harm recall | TP | FP | FN | TN |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 0.190 | 0.316 | 0.353 | 6 | 13 | 11 | 70 |
| `scidocs` | 0.120 | 0.333 | 0.129 | 4 | 8 | 27 | 61 |
| `webis-touche2020` | 0.082 | 0.250 | 0.083 | 1 | 3 | 11 | 34 |
| `trec-covid` | 0.420 | 0.571 | 0.400 | 12 | 9 | 18 | 11 |
| `nfcorpus` | 0.390 | 0.436 | 0.515 | 17 | 22 | 16 | 45 |
| `dbpedia-entity` | 0.360 | 0.417 | 0.469 | 15 | 21 | 17 | 47 |

## Key Observation

The model is not completely blind.

Mean harm probabilities:

- `cqadupstack`: FN 0.463 vs TP 0.787
- `scidocs`: FN 0.565 vs TP 0.804
- `webis-touche2020`: FN 0.515 vs TP 0.863
- `trec-covid`: FN 0.415 vs TP 0.853

The problem is not that all harmful queries are indistinguishable.  The problem
is that the selected threshold is high enough that many moderately risky
queries are allowed through.

## Feature Signals

The top missed-harm gaps are consistent across hard rows:

- missed harmful queries often have worse aggressive top100 BM25 rank mean
- missed harmful queries often have worse aggressive entrant BM25 rank mean
- missed harmful queries often have lower proposal/query atom sharing
- for `scidocs`, missed harmful queries have higher proposal P1 rank mean

This suggests the current features contain risk information, but the policy
objective is too focused on macro utility and not enough on row-harm recall.

## Decision

Do not switch to another heavy classifier yet.

Do not add dual-source/tail proposals yet.

The next best move is to change the gate objective:

- keep the same qrels-free features
- keep logistic for speed and auditability
- select threshold with explicit harm-recall / row-harm constraints
- compare against M1191

This is not blind threshold tuning; it follows from M1195 showing that missed
harm probabilities are systematically below the current selected threshold but
not random.

## Next Step

M1196 should evaluate constrained-threshold veto policies from the existing
M1190 cache:

- same `utility_or_cub` logistic probabilities
- thresholds selected by train fold with constraints such as:
  - harm recall >= 0.50
  - macro CUB >= 0
  - retain at least 70% of aggressive MAP/NDCG gain
- no dataset-specific threshold
- leave-one-dataset-out evaluation

Acceptance:

- CUB non-negative
- better row-harm recall than M1191
- MAP/NDCG/MRR retain enough of aggressive gain

If this fails, then the blocker is feature/proposal observability rather than
gate objective.
