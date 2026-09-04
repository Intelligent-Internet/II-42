# M1155 Marginal Atom Admission

## Objective

Test the hypothesis from M1154: query-level batch admission is too coarse, and
the remaining regressions are caused by a small number of harmful atoms. M1155
replays each top M1151 predicted atom independently through the native path.

## Inputs

- M1151 proxy predictions:
  `runs/m1151_admission_proxy_recovery_v1/admission_proxy.json`
- M1152 base metrics:
  `runs/m1152_admission_proxy_native_replay_v1/native_replay.json`
- M1155 native marginal replay:
  `runs/m1155_marginal_atom_admission_v1/marginal_atom_replay.json`

## Result

| Item | Value |
| --- | ---: |
| query_count | 149 |
| atom_count | 2370 |
| failures | 0 |
| scale 0.01 safe rate | 0.844726 |
| scale 0.01 safe-gain rate | 0.103797 |
| scale 0.05 safe rate | 0.716456 |
| scale 0.05 safe-gain rate | 0.189030 |

Oracle single-atom action:

| dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| +0.001118 | +0.019178 | +0.005896 | +0.008171 | +0.005705 |

The oracle is row-floor clean: all dataset-level deltas are non-negative.

## Interpretation

This is a strong positive structural signal. It validates the diagnosis that
batch topK admission is too coarse. A single good atom is often enough to
recover useful retrieval gains, while avoiding the harmful atoms that caused
M1152-M1154 row regressions.

This also reframes the next bottleneck: the route is not blocked by native
posting geometry. It is blocked by deployable atom-level risk prediction.

## Decision

Keep the per-atom marginal route alive. The next required step is not another
query-level gate. It is a stronger atom-level policy with features that can
distinguish useful atoms from top-rank destructive atoms.
