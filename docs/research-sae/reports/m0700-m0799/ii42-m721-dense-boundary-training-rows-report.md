# M653-D Dense Boundary Training Rows

Status: training rows built; no model promoted.

Rows encode dense-only pairwise boundary constraints.  Qrels, BM25, rerankers, and learned gates are not used.

## Scope

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root: `runs/m721_dense_teacher_top256_shared15_v1`
- Top-k: `100`
- Candidate-k: `256`
- Max pairs/query: `64`

## Macro

| Scope | Pairs | Queries | Dense margin | P1 margin | P1 error | Z error | Pos dense rank | Pos P1 rank | Neg P1 rank |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| All | 44910 | 1335 | 0.008953 | -0.007783 | 0.007783 | 0.153196 | 90.410933 | 112.264796 | 91.526453 |
| Split `dev` | 8073 | 227 | 0.009274 | -0.008007 | 0.008007 | 0.159144 | 89.801065 | 113.259631 | 91.129072 |
| Split `test` | 8774 | 269 | 0.009283 | -0.007563 | 0.007563 | 0.149726 | 90.043424 | 112.446547 | 91.699111 |
| Split `train` | 28063 | 839 | 0.008758 | -0.007787 | 0.007787 | 0.152570 | 90.701279 | 111.921783 | 91.586787 |

## Dataset Matrix

| Dataset | Pairs | Queries | Dense margin | P1 error | Z error |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 2711 | 100 | 0.008212 | 0.007854 | 0.130892 |
| `climate-fever` | 1987 | 100 | 0.008383 | 0.008270 | 0.137621 |
| `cqadupstack` | 2770 | 99 | 0.008448 | 0.007648 | 0.121483 |
| `dbpedia-entity` | 3142 | 98 | 0.008994 | 0.008117 | 0.161396 |
| `fever` | 3173 | 100 | 0.008682 | 0.008326 | 0.146734 |
| `fiqa` | 2937 | 100 | 0.008961 | 0.007559 | 0.141244 |
| `hotpotqa` | 4248 | 100 | 0.009384 | 0.008027 | 0.157661 |
| `msmarco` | 793 | 39 | 0.009613 | 0.007787 | 0.103407 |
| `nfcorpus` | 4589 | 100 | 0.009387 | 0.007610 | 0.193904 |
| `nq` | 4166 | 100 | 0.009617 | 0.008079 | 0.161812 |
| `quora` | 3372 | 100 | 0.008717 | 0.007362 | 0.132538 |
| `scidocs` | 3768 | 100 | 0.008641 | 0.007952 | 0.165520 |
| `scifact` | 4306 | 100 | 0.009353 | 0.007387 | 0.166114 |
| `trec-covid` | 1663 | 50 | 0.007714 | 0.006641 | 0.163424 |
| `webis-touche2020` | 1285 | 49 | 0.009316 | 0.007773 | 0.120130 |

## Interpretation

- The row set is suitable for a first-stage boundary-calibration compiler canary if train/dev/test all contain pairs.
- The target margin is small, so the compiler should use low residual scale and strict overlap/support gates.
- This row set does not justify BM25/reranker work; it only proves that dense teacher boundary constraints can be materialized.

## Artifacts

- JSON: `runs/m721_dense_boundary_training_rows_shared15_v1/m653_dense_boundary_training_rows_summary.json`
- JSONL: `runs/m721_dense_boundary_training_rows_shared15_v1/m653_dense_boundary_training_rows.jsonl`
