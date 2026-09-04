# M653-C Dense Boundary Oracle

Status: diagnostic completed; no model promoted.

This oracle checks whether the P1.3 top100 dense-equivalence gap is recoverable inside the existing P1 top256 candidate surface when raw dense scores are available.  It is a teacher-headroom measurement, not a train/eval result.

## Scope

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root: `runs/m653_dense_teacher_top256_canary_v1`
- Top-k: `100`
- Candidate-k: `256`

## Dataset Matrix

| Dataset | Q | Baseline O@100 | Oracle O@100 | Gain | Dense in cand | Missing dense | Raw margin | P1 wrong margin |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 100 | 0.949800 | 1.000000 | 0.050200 | 1.000000 | 5.020000 | 0.008058 | 0.007669 |
| `cqadupstack` | 100 | 0.950200 | 1.000000 | 0.049800 | 1.000000 | 4.980000 | 0.007831 | 0.007606 |
| `fiqa` | 100 | 0.947100 | 1.000000 | 0.052900 | 1.000000 | 5.290000 | 0.008188 | 0.007418 |
| `scidocs` | 100 | 0.939000 | 1.000000 | 0.061000 | 1.000000 | 6.100000 | 0.007944 | 0.008052 |

## Macro

| Macro | Q | Baseline O@100 | Oracle O@100 | Gain | Dense in cand | Missing dense | Raw margin | P1 wrong margin |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Query-weighted | 400 | 0.946525 | 1.000000 | 0.053475 | 1.000000 | 5.347500 | 0.008006 | 0.007686 |
| Dataset-weighted | 400 | 0.946525 | 1.000000 | 0.053475 | 1.000000 | 5.347500 | 0.008005 | 0.007686 |

## Interpretation

- The dense-score oracle has enough headroom to justify a boundary-calibration compiler objective.
- Raw dense scores separate missing dense top100 docs from false P1 top100 docs, so the teacher contains a usable ordering signal.
- P1 scores rank false top100 documents above missing dense top100 documents on average.  This is the direct error the next first-stage loss must correct.

## Next Step

- Build M653-D boundary-calibration training rows from this oracle.
- Train only query-side/output compiler changes.
- Gate on O@100/O@256, support cosine, active support, and CUB before considering retrieval metrics.

## Artifacts

- JSON: `runs/m653_dense_boundary_oracle_canary_v1/m653_dense_boundary_oracle.json`
- Query JSONL: `runs/m653_dense_boundary_oracle_canary_v1/m653_dense_boundary_oracle_query_rows.jsonl`

