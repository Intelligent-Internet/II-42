# M1198 Score-Shape Veto Audit

## Purpose

M1197 found that hard-row damage is dominated by rank/order perturbation rather
than pure top100 candidate loss. M1198 tests whether qrels-free movement and
score-shape features from M1197 can learn a better hard-row veto.

This audit does not rerun native DB queries. It uses the M1197 movement cache
and leave-one-dataset-out validation on the same hard-row surface.

## Artifacts

- Script: `scripts/audit_m1198_score_shape_veto_audit.py`
- JSON:
  `runs/m1198_score_shape_veto_audit_v1/m1198_score_shape_veto_audit.json`
- Markdown:
  `runs/m1198_score_shape_veto_audit_v1/m1198_score_shape_veto_audit.md`

## Focus Macro

| Source | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `aggressive` | 1.000 | +0.003630 | -0.000027 | -0.000027 | -0.004234 | -0.000522 |
| `conservative` | 0.000 | -0.000472 | +0.000414 | +0.000244 | -0.000262 | -0.000773 |
| `m1191_veto` | 0.859 | +0.003939 | -0.000162 | -0.000259 | -0.004056 | -0.000522 |
| `score_shape_veto` | 0.478 | +0.000369 | -0.000597 | -0.001347 | -0.002156 | +0.000031 |
| `union_veto` | 0.438 | +0.000369 | -0.001028 | -0.001860 | -0.002156 | +0.000031 |

## Interpretation

The score-shape veto can make CUB slightly positive, but it pays for that by
giving back too much Recall and worsening MAP/NDCG. The union veto is worse.

Per-dataset behavior confirms this is not a stable frontier:

- `scidocs`: CUB is fixed, but Recall gain collapses from +0.010000 to +0.000000.
- `cqadupstack`: score-shape veto is worse than both aggressive and M1191 veto.
- `webis-touche2020`: CUB is reduced to zero and NDCG remains negative.

## Decision

Stop this BM25-top proposal plus veto family as the main route.

M1197/M1198 together show:

1. The route has real useful movement.
2. The hard-row damage is mostly rank/order score-shape damage.
3. Existing qrels-free movement features can see some risk, but cannot convert
   it into a better deployable policy.

The next useful work is proposal redesign, not another classifier or threshold
pass. A better proposal should reduce score-shape perturbation at the source,
for example by producing smaller/localized posting deltas, protecting existing
high-confidence positive-like head evidence, or changing the proposal from
BM25-top document atoms to a rank-stable atom subset.
