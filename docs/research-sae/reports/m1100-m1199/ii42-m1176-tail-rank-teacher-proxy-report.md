# M1176 Tail Rank-Teacher Proxy

M1176 tests whether existing deployable M1144/M1145 query/native features can
approximate the M1175 oracle `tail_rank_teacher` selector under
leave-one-dataset-out validation.

Artifacts:

- Script: `scripts/audit_m1176_tail_rank_teacher_proxy.py`
- JSON: `runs/m1176_tail_rank_teacher_proxy_v1/tail_rank_teacher_proxy.json`
- Run summary: `runs/m1176_tail_rank_teacher_proxy_v1/summary.md`

## Result

Labels:

- `rank_teacher`: 51/138.
- `no_harm`: 71/138.

LODO AUC:

- `rank_teacher`: 0.577868.
- `no_harm`: 0.544671.

No non-empty row-floor-clean deployable policy was found.

Best-any policy:

| Policy | Accepted | Rank precision | Rank recall | Harm accepts | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| rank0.20_noharm0.10 | 84 | 0.429 | 0.706 | 38 | +0.000000 | +0.000000 | +0.004841 | +0.009239 | +0.018478 |
| rank0.10_noharm0.10 | 111 | 0.387 | 0.843 | 55 | +0.000000 | -0.000133 | +0.000124 | +0.004812 | +0.020721 |
| rank0.30_noharm0.10 | 64 | 0.422 | 0.529 | 28 | +0.000000 | +0.000000 | +0.003341 | +0.008944 | +0.011232 |

## Interpretation

M1175 proves that a clean rank-teacher ceiling exists, but M1176 shows that the
current query/native gate features cannot safely recover it.  Best-any policies
only improve by accepting many `tail_harm` rows, and no threshold combination is
row-floor clean.

This is a stop signal for more selector-gate loops on this feature set.

## Decision

Do not continue with another threshold/proxy selector for `tail_rank_teacher`.
The next branch should train a direct rank-distillation objective:

1. Use `tail_rank_teacher` rows as positive pair/listwise rank movement.
2. Use `tail_harm` rows as explicit negative constraints.
3. Preserve protected direct action and P1 dense-equivalence floors.
4. Evaluate through native replay, not offline score-only approximations.
