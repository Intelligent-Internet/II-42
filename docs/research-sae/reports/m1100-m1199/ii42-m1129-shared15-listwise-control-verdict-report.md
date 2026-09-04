# ii42 M1129 Shared15 Listwise-Only Gate Verdict

M1129 expands the M1128 listwise-only control to the full shared15 surface.

The goal is to test whether the retained M1128 signal survives a broad dataset
mix without semantic expansion, per-dataset tuning, or top-rank preservation.

## Run Status

Remote run:
`/home/huoju/leask/runs/ii42-m1129-shared15-listwise050-control-export-v1`

Local synced run:
`/Volumes/Betty/Tmp/ii42-m1000/m1129-shared15-listwise050-control-export-v1`

The run completed normally and wrote final JSON, checkpoint, dataset partials,
and ranking exports for all 15 datasets.

Datasets:
`nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, `trec-covid`,
`cqadupstack`, `webis-touche2020`, `climate-fever`, `dbpedia-entity`, `fever`,
`hotpotqa`, `msmarco`, `nq`, `quora`.

## Training Shape

| Epoch | Loss | Rank | Listwise | Top-Rank | Fanout | Background |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 2.170040 | 0.626284 | 0.922446 | 0.000000 | 22.625166 | 0.351283 |
| 2 | 1.177798 | 0.482485 | 0.141404 | 0.000000 | 12.987189 | 0.127453 |
| 3 | 0.494281 | 0.287365 | 0.058587 | 0.000000 | 3.287627 | 0.041933 |
| 4 | 0.275434 | 0.177380 | 0.041723 | 0.000000 | 1.312398 | 0.026067 |

The objective remains stable at shared15.  The listwise term decays cleanly and
does not require the M1126/M1127 top-rank preservation term.

## Heldout Macro

Fine-grid verdict remains `do_not_promote_fixed_alpha`, because train-selected
alpha is `0.75` while heldout-best alpha is `0.60`.

| Alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.00 | 0.746410 | 0.742393 | 0.669888 | 0.563245 |
| 0.25 | 0.766004 | 0.764444 | 0.696505 | 0.590268 |
| 0.50 | 0.770982 | 0.777186 | 0.699337 | 0.604800 |
| 0.60 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| 0.75 | 0.771468 | 0.763056 | 0.689540 | 0.600224 |

Best alpha=0.60 versus lexical:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.025988 | +0.037379 | +0.030078 | +0.042939 |

Selected alpha=0.75 versus lexical:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.025058 | +0.020664 | +0.019652 | +0.036979 |

## Row-Level Result

At best alpha=0.60 versus lexical:

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | -0.017113 | -0.057816 | -0.016161 |
| `climate-fever` | +0.033333 | +0.082265 | +0.067646 | +0.067208 |
| `cqadupstack` | -0.039227 | -0.007229 | +0.002241 | -0.010519 |
| `dbpedia-entity` | -0.008187 | -0.016667 | -0.038794 | -0.044922 |
| `fever` | +0.000000 | +0.051667 | +0.039416 | +0.051667 |
| `fiqa` | +0.065079 | +0.189706 | +0.145550 | +0.163015 |
| `hotpotqa` | +0.016667 | +0.000000 | +0.026912 | +0.036946 |
| `msmarco` | +0.094338 | +0.000000 | +0.045201 | +0.133443 |
| `nfcorpus` | -0.002429 | +0.101389 | +0.035999 | +0.018507 |
| `nq` | +0.033333 | +0.033333 | +0.045957 | +0.043578 |
| `quora` | +0.000000 | -0.035238 | -0.020468 | -0.027788 |
| `scidocs` | +0.035000 | +0.014317 | -0.013439 | -0.007142 |
| `scifact` | +0.056667 | +0.010926 | +0.008459 | +0.006519 |
| `trec-covid` | +0.015442 | +0.033333 | +0.061045 | +0.080071 |
| `webis-touche2020` | +0.089809 | +0.120000 | +0.103255 | +0.149663 |

The route is broad macro-positive and strongly positive on several hard rows,
but it is not row-clean.  The main remaining regression rows are `arguana`,
`dbpedia-entity`, `quora`, and parts of `cqadupstack`/`scidocs`.

## Query Tradeoff

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| 0.00 -> 0.25 | +0.016744 | +0.021398 | +0.024125 | +0.022624 |
| 0.25 -> 0.30 | +0.000557 | +0.001035 | -0.000262 | +0.002692 |
| 0.30 -> 0.50 | +0.002834 | +0.011952 | +0.002221 | +0.008437 |
| 0.50 -> 0.75 | -0.000835 | -0.015570 | -0.011706 | -0.007987 |

The useful pressure region is bounded.  Alpha `0.25` to `0.60` is useful;
alpha `0.75` starts to spend top-rank quality.

## Verdict

M1129 validates the core M1128 conclusion on shared15:

- listwise-only atom pressure is the current best broader route;
- top-rank preservation should not be default;
- fixed high alpha should not be promoted;
- the next bottleneck is row-level risk, not lack of training depth.

This is a real route-level signal, not a shared5/shared8 artifact.

## Next Step

Stop broad loss swapping for now.  The next useful work is a row-risk audit and
bounded-alpha policy over the M1129 exports:

- identify rows where alpha pressure is consistently safe;
- identify rows where lexical should remain closer to alpha `0.0`;
- keep the policy global and auditable;
- do not tune per dataset as a hidden optimization;
- validate on leave-dataset-out or heldout row groups before native promotion.

If a global row-risk policy cannot keep the M1129 macro gains while protecting
the known regression rows, this route should move from training changes to
native engineering-index evaluation as the next proof surface.
