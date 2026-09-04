# M1961 M190 Query-DF Pruning Observability Audit

## Decision Boundary

This is a qrels-free, train-free observability audit. It does not
claim retrieval improvement. A threshold may enter native replay only
when it leaves every query non-empty, retains at least 80% mean L2
impact mass, and predicts at least 5% lower mean exact posting work.

The fixed `0.12` threshold comes from the historical M150 C4
experiment. It is not selected on M1960 evaluation labels.
DeepImpact/uniCOIL motivate preserving learned impacts; DF-FLOPS
motivates controlling corpus document frequency; Two-Step SPLADE
motivates validating retrieval quality and traversal separately.

## Macro Observability

| maxDF ratio | Eligible rows | Mean touch ratio | Mean L1 mass | Mean L2 mass | Zero queries |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.20 | 7/9 | 0.8466 | 0.9813 | 0.9891 | 0 |
| 0.12 | 8/9 | 0.7169 | 0.9572 | 0.9753 | 0 |
| 0.10 | 8/9 | 0.6606 | 0.9421 | 0.9670 | 0 |
| 0.05 | 9/9 | 0.4608 | 0.8655 | 0.9236 | 0 |

## Fixed 0.12 Per-Dataset Audit

| Dataset | maxDF | Baseline touches | Touch ratio | L1 mass | L2 mass | Mean query nnz | Zero queries | Replay? |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| nfcorpus | 435 | 5402.2 | 0.5720 | 0.9633 | 0.9801 | 77.21 | 0 | yes |
| scifact | 621 | 12503.4 | 0.6381 | 0.9472 | 0.9707 | 76.06 | 0 | yes |
| arguana | 1040 | 18356.5 | 0.8036 | 0.9691 | 0.9833 | 77.67 | 0 | yes |
| fiqa | 6916 | 113429.3 | 0.7618 | 0.9647 | 0.9793 | 77.50 | 0 | yes |
| scidocs | 3078 | 46165.0 | 0.7658 | 0.9679 | 0.9818 | 77.67 | 0 | yes |
| trec-covid | 20559 | 713731.6 | 0.4670 | 0.8797 | 0.9281 | 71.56 | 0 | yes |
| webis-touche2020 | 45905 | 807285.1 | 0.5821 | 0.9377 | 0.9630 | 75.65 | 0 | yes |
| quora | 62751 | 600622.2 | 0.8844 | 0.9879 | 0.9933 | 79.11 | 0 | yes |
| cqadupstack | 54863 | 480594.3 | 0.9773 | 0.9972 | 0.9983 | 79.81 | 0 | no |

## Stop Rule

Do not train or tune a DF objective from this audit. If fixed 0.12
fails observability, retain the raw checkpoint and stop this branch.
If it passes, run one native replay after the frozen broad9 baseline
completes. Reject it if row-safe Recall, MAP, NDCG, or MRR is lost.
