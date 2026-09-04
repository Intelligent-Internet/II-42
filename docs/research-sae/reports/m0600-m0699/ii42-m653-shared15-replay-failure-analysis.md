# M653 Shared15 Replay Failure Analysis

This report analyzes the full shared15 replay of the M653-E id432
query-side compiler.  It uses only replay query rows and compares
`m653_dense_boundary` against the frozen `p1_native` baseline.

## Macro Signal

| Queries | dO@100 | dO@256 | dR@100 | dMAP@100 | dNDCG@10 | dMRR@20 | O256- queries | O256+ queries |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1342 | +0.00005961 | -0.00001455 | +0.00009041 | -0.00008828 | +0.00006790 | +0.00037339 | 216 | 211 |

## Per Dataset Delta

| Dataset | Q | dO@100 | dO@256 | dR@100 | dMAP@100 | dNDCG@10 | O256- | O256+ | O100+ and O256- | R100+ and O256- |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 100 | +0.00050000 | +0.00011719 | +0.00000000 | +0.00000000 | +0.00000000 | 23 | 24 | 6 | 0 |
| `climate-fever` | 100 | +0.00100000 | -0.00007813 | +0.00000000 | -0.00036836 | +0.00055765 | 16 | 14 | 4 | 0 |
| `cqadupstack` | 100 | -0.00030000 | -0.00015625 | -0.00047619 | +0.00004440 | +0.00022002 | 14 | 14 | 3 | 0 |
| `dbpedia-entity` | 100 | +0.00010000 | +0.00003906 | +0.00005750 | +0.00045316 | +0.00051671 | 13 | 12 | 1 | 0 |
| `fever` | 100 | -0.00020000 | +0.00003906 | +0.00000000 | +0.00000000 | +0.00000000 | 13 | 12 | 1 | 0 |
| `fiqa` | 100 | +0.00070000 | -0.00027344 | +0.00000000 | +0.00016803 | +0.00007758 | 23 | 17 | 5 | 0 |
| `hotpotqa` | 100 | -0.00020000 | +0.00011719 | +0.00000000 | +0.00002367 | +0.00000000 | 15 | 18 | 1 | 0 |
| `msmarco` | 43 | -0.00046512 | -0.00109012 | +0.00024740 | +0.00012008 | -0.00008691 | 7 | 1 | 0 | 0 |
| `nfcorpus` | 100 | -0.00010000 | -0.00031250 | -0.00077880 | +0.00014853 | +0.00223653 | 18 | 10 | 0 | 0 |
| `nq` | 100 | +0.00010000 | -0.00039063 | +0.00000000 | +0.00000000 | +0.00000000 | 19 | 13 | 3 | 0 |
| `quora` | 100 | -0.00150000 | +0.00007813 | +0.00000000 | -0.00166228 | -0.00173197 | 15 | 15 | 0 | 0 |
| `scidocs` | 100 | +0.00100000 | +0.00023437 | +0.00200000 | -0.00059492 | -0.00142816 | 19 | 21 | 7 | 0 |
| `scifact` | 100 | -0.00000000 | +0.00035156 | +0.00000000 | +0.00039241 | +0.00010394 | 9 | 15 | 3 | 0 |
| `trec-covid` | 50 | -0.00060000 | +0.00070312 | -0.00010542 | -0.00006383 | +0.00103712 | 4 | 12 | 0 | 0 |
| `webis-touche2020` | 49 | +0.00040816 | +0.00031888 | +0.00072886 | +0.00038973 | -0.00024960 | 8 | 13 | 0 | 0 |

## Worst O@256 Queries

| Dataset | Query | dO@100 | dO@256 | dR@100 | dMAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `msmarco` | `207786` | +0.000000 | -0.015625 | +0.000000 | +0.000000 |
| `nfcorpus` | `PLAIN-924` | +0.000000 | -0.011719 | +0.000000 | -0.000012 |
| `nq` | `test38` | +0.010000 | -0.011719 | +0.000000 | +0.000000 |
| `arguana` | `test-environment-aeghhgwpe-con03a` | +0.010000 | -0.007812 | +0.000000 | +0.000000 |
| `arguana` | `test-health-ahiahbgbsp-pro02a` | +0.000000 | -0.007812 | +0.000000 | +0.000000 |
| `arguana` | `test-health-dhgsshbesbc-con02a` | +0.000000 | -0.007812 | +0.000000 | +0.000000 |
| `arguana` | `test-health-hpehwadvoee-pro04a` | +0.000000 | -0.007812 | +0.000000 | +0.000000 |
| `climate-fever` | `141` | +0.000000 | -0.007812 | +0.000000 | +0.000000 |
| `climate-fever` | `189` | +0.010000 | -0.007812 | +0.000000 | +0.000000 |
| `climate-fever` | `86` | +0.000000 | -0.007812 | +0.000000 | +0.000000 |

## Conclusion

The failure is a tail-preservation failure, not a support failure.  Across 1342 replayed queries, O@100 moved +0.00005961 and Recall@100 moved +0.00009041, but O@256 moved -0.00001455.  The next experiment should keep the same frozen-doc/query-side setup but add an explicit top256/listwise tail-preservation selection guard before increasing capacity.  If that cannot preserve O@256 while keeping the small Recall/O@100 gains, the current boundary-only objective is too underconstrained for first-stage promotion.
