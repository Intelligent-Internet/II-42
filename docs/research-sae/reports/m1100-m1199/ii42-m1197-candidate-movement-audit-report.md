# M1197 Candidate Movement Audit

## Purpose

M1196 showed that changing only the M1191 veto threshold did not recover the
macro contract. M1197 reruns a targeted native-path audit on hard rows to find
what actually moves in and out of top100 under the current BM25-top posting
delta.

This is an explanatory audit only. It does not train a new model.

## Artifacts

- Script: `scripts/audit_m1197_candidate_movement_audit.py`
- JSON:
  `runs/m1197_candidate_movement_audit_v1/m1197_candidate_movement_audit.json`
- Markdown:
  `runs/m1197_candidate_movement_audit_v1/m1197_candidate_movement_audit.md`

## Surface

- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: 249
- Baseline: current P1-a0125 native query
- Aggressive: BM25 top3 context posting delta
- Conservative: BM25 top1 context posting delta
- Veto: M1191 logistic damage-veto selection

## Focus Macro

| Source | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `aggressive` | +0.003630 | -0.000027 | -0.000027 | -0.004234 | -0.000522 |
| `conservative` | -0.000472 | +0.000414 | +0.000244 | -0.000262 | -0.000773 |
| `veto` | +0.003939 | -0.000162 | -0.000259 | -0.004056 | -0.000522 |

## Main Finding

Hard-row harm is not primarily a candidate-miss problem. It is mostly a
rank/order problem inside the top100 boundary.

Aggressive mechanism counts across the 249 focus queries:

| Mechanism | Count |
| --- | ---: |
| `positive_rank_demoted` | 46 |
| `rank_order_or_score_shape_harm` | 7 |
| `positive_displacement` | 6 |
| `cub_harm_without_top100_positive_loss` | 2 |
| `useful_movement` | 90 |
| `neutral_or_mixed` | 98 |

This means the current proposal often moves useful candidates, but in the hard
rows it also perturbs score geometry enough to demote existing positives.

## Dataset Notes

- `cqadupstack`: clearly bad frontier. Aggressive and veto both lose Recall,
  MAP, NDCG, and MRR. The dominant harm is positive rank demotion, not CUB loss.
- `scidocs`: aggressive gains Recall/MAP/NDCG but loses CUB by -0.002000. This
  is a candidate-boundary/upper-bound issue mixed with useful movement.
- `webis-touche2020`: aggressive gains Recall/MAP/CUB but loses NDCG. The issue
  is rank-order damage rather than top100 candidate loss.

## Decision

Do not continue threshold-only tuning.

Do not add another generic classifier on the same features unless it directly
models score-shape/rank-order risk. The next step should test whether qrels-free
movement/score-shape features can separate harmful rank perturbations from
useful movement.
