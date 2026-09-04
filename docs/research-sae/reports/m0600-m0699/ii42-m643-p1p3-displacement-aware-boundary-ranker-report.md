# M643 / P1.3 Displacement-Aware Boundary Ranker Report

Status: `not_promoted_weighting_limit_reached`.

## Goal

M643 follows M642's conclusion: M641 is a real nonlinear scorer breakthrough,
but it is not promotion-ready because gains are concentrated and top100
positive displacement remains high.  M643 keeps the M641 architecture and native
replay surface, but changes the training distribution:

- dataset-balanced sampling
- dataset-balanced sample weights
- stronger top100-positive protection
- explicit dominance/promotion gate

This is still a global scorer.  Dataset ids are used only to balance the
training surface and reporting.  They are not inference features.

## Artifacts

| Artifact | Path |
| --- | --- |
| Script | `scripts/train_m643_displacement_aware_boundary_ranker.py` |
| Tests | `tests/test_train_m643_displacement_aware_boundary_ranker.py` |
| Conservative run | `runs/m643_p1p3_displacement_aware_boundary_ranker_validation_v1/` |
| Soft run | `runs/m643_p1p3_displacement_aware_boundary_ranker_soft_v1/` |

## Runs

| Run | Sampling / weights | alpha | preserve | window | Status |
| --- | --- | ---: | ---: | ---: | --- |
| Conservative | dataset balance exponent 0.5, top100-positive weight 8.0, under-ranked positive 5.0 | 0.10 | 95 | 300 | `capacity_breakthrough_dominance_limited` |
| Soft | dataset balance exponent 0.25, top100-positive weight 5.0, under-ranked positive 6.0 | 0.15 | 95 | 300 | `capacity_breakthrough_dominance_limited` |

## Results

Reference rows:

| Run | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Promoted | Displaced | Net top100 | Max positive share | Major regression |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M641 validation | +0.000000 | +0.000131 | +0.004468 | +0.000000 | 57 | 49 | +8 | 0.784 | msmarco |
| M641 full-grid | +0.000000 | +0.000089 | +0.004193 | +0.000000 | 63 | 58 | +5 | 0.790 | cqadupstack, msmarco |
| M643 conservative | +0.000000 | +0.000088 | +0.003487 | +0.000000 | 20 | 13 | +7 | 0.884 | cqadupstack |
| M643 soft | +0.000000 | +0.000123 | +0.003586 | +0.000000 | 38 | 30 | +8 | 0.856 | cqadupstack |

## Diagnosis

M643 successfully changes the displacement behavior, but not in the way needed
for promotion.

The conservative run reduces displacement from M641 validation's `49` to `13`.
However, under-ranked positive promotions also collapse from `57` to `20`, so
Recall@100 drops from `+0.004468` to `+0.003487`.

The soft run partially recovers promotion (`38`) but still remains below M641
validation and introduces a larger cqadupstack regression.  It also does not fix
dominance: FiQA still accounts for `85.6%` of positive dataset recall delta.

This shows the key blocker more precisely:

- row weighting can trade promotions for fewer displacements
- row weighting does not identify which positive is safe to replace
- dataset-balanced sampling does not solve FiQA dominance
- simply protecting top100 positives makes the scorer too conservative

Therefore the current row-classification objective is the wrong abstraction for
the next step.  It treats candidate rows independently, but the failure is a
query-local replacement decision: a boundary positive should only enter top100
when it can beat a specific safe-to-displace item.

## Decision

Do not promote M643.

Do not continue row-weight-only M641/M643 variants.  They are now a mapped
tradeoff:

- M641: more promotions, too many displacements, near gate
- M643: fewer displacements, fewer promotions, farther from gate

The useful retained signal is that nonlinear scorer capacity is real, but the
objective must be query-local and displacement-aware, not just row-weighted.

## Next Step

The next stage should not be another HGB row classifier sweep.  The next useful
probe is M644:

1. Build query-local replacement pairs from M629 rows.
2. For each query, pair under-ranked positives with actual top100 candidates.
3. Label protected top100 positives as non-displaceable.
4. Train a scorer or margin model on replacement decisions:
   boundary positive > safe negative, but not > protected positive.
5. Evaluate by net top100 positives and per-dataset dominance, not row AUC.

Acceptance should remain unchanged:

- dRecall@100 >= `+0.005`
- dMAP@100 >= `0`
- no NDCG/MRR regression
- no major dataset Recall@100 regression below `-0.001`
- max positive dataset share <= `0.75`

If M644 also cannot pass, the second-stage scorer line should pause and M641
recovered/displaced pairs should become first-stage generated-posting training
data.

## Verification

Completed:

- `python3 -m py_compile scripts/train_m643_displacement_aware_boundary_ranker.py scripts/train_m641_nonlinear_native_boundary_ranker.py scripts/audit_m642_m641_dominance.py`
- `pytest -q tests/test_train_m643_displacement_aware_boundary_ranker.py`
