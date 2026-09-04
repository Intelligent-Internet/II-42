# M1200 Candidate-Boundary Guard

## Purpose

M1199 left a CUB regression on the hard-row surface. M1200 tests a qrels-free
candidate-boundary guard: apply the consensus atom proposal only when generated
query top1000 overlap with baseline is high enough; otherwise keep the baseline
P1 query.

## Artifacts

- Script: `scripts/audit_m1200_candidate_boundary_guard.py`
- JSON:
  `runs/m1200_candidate_boundary_guard_v1/m1200_candidate_boundary_guard.json`
- Markdown:
  `runs/m1200_candidate_boundary_guard_v1/m1200_candidate_boundary_guard.md`

## Hard-Row Macro

| Source | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `consensus2_mean_s0.10` | 1.000 | +0.001135 | +0.000756 | +0.000611 | +0.000914 | -0.000522 |
| `guard_t0.98` | 0.980 | +0.001135 | +0.000769 | +0.000609 | +0.001047 | -0.000522 |
| `guard_t0.99` | 0.839 | +0.000689 | +0.000048 | -0.000505 | +0.000045 | +0.000282 |
| `guard_t0.995` | 0.679 | -0.000130 | -0.000063 | -0.000154 | +0.000000 | +0.000251 |
| `guard_t0.999` | 0.550 | +0.000000 | +0.000010 | +0.000000 | +0.000000 | +0.000000 |

## Interpretation

Candidate-k overlap can repair CUB, but on the hard-row-only surface it gives
back too much NDCG/Recall. This is not enough to promote by itself.

However, the result supports a broader test because the hard-row surface is
biased toward failure rows. The guard should be evaluated on full shared15
before the branch is stopped.
