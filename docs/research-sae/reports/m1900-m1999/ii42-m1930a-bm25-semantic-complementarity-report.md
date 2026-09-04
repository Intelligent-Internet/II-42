# M1930A BM25/Semantic Complementarity Audit

Decision: **authorize_deterministic_single_index_closure**.

This is an evaluation-only parent-selection audit. The union columns
measure capacity, not a deployable fusion policy.

## Equal-Dataset Macro

| Route | Sem-only @100 | BM25-miss recovery | Union R@100 | Union gain | Union CUB@1000 | CUB gain | Storage MiB | p95 ms | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| p1 | 12.5594% | 54.8173% | 0.770815 | +0.125953 | 0.890982 | +0.128926 | 703.59 | 39.75 | True |
| m1914 | 9.8328% | 50.0623% | 0.743704 | +0.098842 | 0.874436 | +0.112380 | 1026.16 | 31.23 | True |
| opensearch | 9.4199% | 48.5747% | 0.738828 | +0.093966 | 0.869844 | +0.107788 | 1276.27 | 112.23 | True |

Selected trainable parent: `m1914`. Product reference: `p1`.

## Per-Dataset Signal

| Dataset | Route | BM25 R@100 | Semantic R@100 | Union R@100 | Sem-only | BM25-only | Union CUB@1000 | Storage MiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| fiqa | p1 | 0.510764 | 0.770400 | 0.800381 | 31.6530% | 2.6377% | 0.941985 | 538.55 |
| fiqa | m1914 | 0.510764 | 0.665214 | 0.702688 | 20.3400% | 3.6928% | 0.881280 | 797.87 |
| fiqa | opensearch | 0.510764 | 0.656042 | 0.691492 | 18.8746% | 3.9859% | 0.870318 | 957.82 |
| arguana | p1 | 0.952891 | 0.990007 | 0.994290 | 4.1399% | 0.4283% | 0.999286 | 81.67 |
| arguana | m1914 | 0.952891 | 0.988580 | 0.992862 | 3.9971% | 0.4283% | 0.999286 | 121.50 |
| arguana | opensearch | 0.952891 | 0.988580 | 0.993576 | 4.0685% | 0.4996% | 1.000000 | 166.39 |
| nfcorpus | p1 | 0.233236 | 0.266389 | 0.332588 | 7.0699% | 6.2186% | 0.625992 | 34.39 |
| nfcorpus | m1914 | 0.233236 | 0.292702 | 0.316600 | 7.0293% | 2.9188% | 0.623845 | 44.81 |
| nfcorpus | opensearch | 0.233236 | 0.285311 | 0.312576 | 7.3618% | 2.5134% | 0.615725 | 61.67 |
| scifact | p1 | 0.882556 | 0.897667 | 0.956000 | 7.3746% | 5.6047% | 0.996667 | 48.98 |
| scifact | m1914 | 0.882556 | 0.954333 | 0.962667 | 7.9646% | 0.8850% | 0.993333 | 61.98 |
| scifact | opensearch | 0.882556 | 0.934333 | 0.957667 | 7.3746% | 2.0649% | 0.993333 | 90.38 |

## Interpretation

A passing result authorizes only M1930B deterministic one-index closure. It does not authorize residual training or product promotion. Standalone semantic storage is an upper-bound proxy; M1930B must measure the exact combined index frontier.
