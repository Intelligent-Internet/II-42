# ii42 M1140 Tail-Only Expansion Replay

## Objective

Test a structural rank-composition policy instead of another gamma, alpha, or selector loop. The policy keeps the base top ranks fixed and only uses the tail run as an expansion source.

## Macro Results

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| `tail` | 0.782350 | 0.762162 | 0.698423 | 0.598732 |
| `protect5_then_tail` | 0.782436 | 0.781183 | 0.706386 | 0.608956 |
| `protect10_then_tail` | 0.783111 | 0.780256 | 0.700210 | 0.608367 |
| `protect15_then_tail` | 0.782991 | 0.780126 | 0.700210 | 0.608376 |
| `protect20_then_tail` | 0.783112 | 0.779772 | 0.700210 | 0.607856 |
| `protect25_then_tail` | 0.782881 | 0.779772 | 0.700210 | 0.607327 |
| `protect30_then_tail` | 0.782377 | 0.779772 | 0.700210 | 0.607405 |
| `protect40_then_tail` | 0.782478 | 0.779772 | 0.700210 | 0.607233 |

## Delta Versus Base

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tail` | +0.009952 | -0.017610 | -0.001542 | -0.007453 |
| `protect5_then_tail` | +0.010038 | +0.001412 | +0.006421 | +0.002771 |
| `protect10_then_tail` | +0.010713 | +0.000484 | +0.000245 | +0.002182 |
| `protect15_then_tail` | +0.010593 | +0.000355 | +0.000245 | +0.002191 |
| `protect20_then_tail` | +0.010714 | +0.000000 | +0.000245 | +0.001672 |
| `protect25_then_tail` | +0.010483 | +0.000000 | +0.000245 | +0.001142 |
| `protect30_then_tail` | +0.009979 | +0.000000 | +0.000245 | +0.001221 |
| `protect40_then_tail` | +0.010080 | +0.000000 | +0.000245 | +0.001048 |

## `protect5_then_tail` Per-Dataset Delta

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.005197 | +0.039654 | +0.004245 |
| `climate-fever` | -0.008333 | +0.001850 | +0.003329 | -0.004932 |
| `cqadupstack` | +0.048937 | +0.001296 | -0.005422 | +0.002424 |
| `dbpedia-entity` | +0.014774 | +0.000000 | +0.017115 | +0.032088 |
| `fever` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `fiqa` | +0.058333 | +0.004945 | -0.008545 | -0.002713 |
| `hotpotqa` | +0.000000 | +0.000000 | -0.006153 | -0.003104 |
| `msmarco` | -0.055517 | +0.000000 | -0.004894 | -0.046478 |
| `nfcorpus` | +0.066618 | +0.000000 | +0.029556 | +0.018267 |
| `nq` | +0.000000 | +0.000000 | +0.000000 | +0.000674 |
| `quora` | +0.000000 | +0.000794 | +0.000762 | +0.000348 |
| `scidocs` | -0.001667 | +0.000477 | -0.006740 | -0.008424 |
| `scifact` | -0.006667 | +0.006614 | +0.006503 | +0.000675 |
| `trec-covid` | +0.012161 | +0.000000 | +0.027573 | +0.043173 |
| `webis-touche2020` | +0.021935 | +0.000000 | +0.003576 | +0.005329 |

## Query-Level Classes

- `clean_gain`: 85
- `damage`: 69
- `mixed`: 25
- `unchanged`: 224

## Protect Underfill

For the selected policy, `2` queries had fewer base candidates than the protected rank count. In those cases the tail run fills the missing ranks.

## Verdict

If the selected protected-tail policy improves all macro metrics, promote it to the next validation route. It should still be replayed on another split/seed or a native path before becoming a training target.
