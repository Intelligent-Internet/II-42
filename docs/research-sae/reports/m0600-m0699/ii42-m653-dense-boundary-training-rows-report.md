# M653-D Dense Boundary Training Rows

Status: training rows built; no model promoted.

Rows encode dense-only pairwise boundary constraints.  Qrels, BM25, rerankers, and learned gates are not used.

## Scope

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root: `runs/m653_dense_teacher_top256_canary_v1`
- Top-k: `100`
- Candidate-k: `256`
- Max pairs/query: `64`

## Macro

| Scope | Pairs | Queries | Dense margin | P1 margin | P1 error | Z error | Pos dense rank | Pos P1 rank | Neg P1 rank |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| All | 12186 | 399 | 0.008579 | -0.007766 | 0.007766 | 0.141955 | 91.490891 | 111.454702 | 92.129493 |
| Split `dev` | 1842 | 60 | 0.008271 | -0.007780 | 0.007780 | 0.144328 | 91.194897 | 111.602606 | 91.693268 |
| Split `test` | 2491 | 86 | 0.008967 | -0.007864 | 0.007864 | 0.144486 | 90.912485 | 111.630670 | 91.962666 |
| Split `train` | 7853 | 253 | 0.008528 | -0.007732 | 0.007732 | 0.140596 | 91.743792 | 111.364192 | 92.284732 |

## Dataset Matrix

| Dataset | Pairs | Queries | Dense margin | P1 error | Z error |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 2711 | 100 | 0.008212 | 0.007854 | 0.130892 |
| `cqadupstack` | 2770 | 99 | 0.008448 | 0.007648 | 0.121483 |
| `fiqa` | 2937 | 100 | 0.008961 | 0.007559 | 0.141244 |
| `scidocs` | 3768 | 100 | 0.008641 | 0.007952 | 0.165520 |

## Interpretation

- The row set is suitable for a first-stage boundary-calibration compiler canary if train/dev/test all contain pairs.
- The target margin is small, so the compiler should use low residual scale and strict overlap/support gates.
- This row set does not justify BM25/reranker work; it only proves that dense teacher boundary constraints can be materialized.

## Artifacts

- JSON: `runs/m653_dense_boundary_training_rows_canary_v1/m653_dense_boundary_training_rows_summary.json`
- JSONL: `runs/m653_dense_boundary_training_rows_canary_v1/m653_dense_boundary_training_rows.jsonl`

