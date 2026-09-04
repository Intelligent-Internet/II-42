# ii42 M1140 Tail-Only Expansion Replay

## Objective

Test a structural rank-composition policy instead of another gamma, alpha, or selector loop. The policy keeps the base top ranks fixed and only uses the tail run as an expansion source.

## Macro Results

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.864721 | 0.889357 | 0.830948 | 0.750745 |
| `tail` | 0.867144 | 0.885913 | 0.825487 | 0.743246 |
| `protect5_then_tail` | 0.867130 | 0.889221 | 0.829581 | 0.750108 |
| `protect10_then_tail` | 0.867323 | 0.889248 | 0.830948 | 0.751074 |
| `protect15_then_tail` | 0.867330 | 0.889332 | 0.830948 | 0.751344 |
| `protect20_then_tail` | 0.867554 | 0.889357 | 0.830948 | 0.751708 |
| `protect25_then_tail` | 0.868289 | 0.889357 | 0.830948 | 0.752097 |
| `protect30_then_tail` | 0.868118 | 0.889357 | 0.830948 | 0.752108 |
| `protect40_then_tail` | 0.868190 | 0.889357 | 0.830948 | 0.752223 |

## Delta Versus Base

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tail` | +0.002423 | -0.003444 | -0.005461 | -0.007500 |
| `protect5_then_tail` | +0.002409 | -0.000136 | -0.001368 | -0.000637 |
| `protect10_then_tail` | +0.002602 | -0.000108 | +0.000000 | +0.000329 |
| `protect15_then_tail` | +0.002610 | -0.000025 | +0.000000 | +0.000599 |
| `protect20_then_tail` | +0.002833 | +0.000000 | +0.000000 | +0.000963 |
| `protect25_then_tail` | +0.003568 | +0.000000 | +0.000000 | +0.001351 |
| `protect30_then_tail` | +0.003397 | +0.000000 | +0.000000 | +0.001363 |
| `protect40_then_tail` | +0.003469 | +0.000000 | +0.000000 | +0.001478 |

## `protect5_then_tail` Per-Dataset Delta

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.001389 | +0.001370 | +0.001389 |
| `climate-fever` | +0.007143 | +0.001160 | -0.002941 | -0.001431 |
| `cqadupstack` | -0.002821 | -0.003176 | -0.008493 | -0.005596 |
| `dbpedia-entity` | +0.012188 | +0.000000 | -0.001798 | +0.004514 |
| `fever` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `fiqa` | -0.003333 | -0.004112 | -0.006486 | -0.005730 |
| `hotpotqa` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `msmarco` | -0.002709 | +0.000000 | +0.000000 | -0.009158 |
| `nfcorpus` | +0.000317 | +0.005386 | +0.011220 | +0.012368 |
| `nq` | -0.014286 | +0.000000 | +0.000000 | -0.000246 |
| `quora` | +0.000000 | +0.000000 | -0.001309 | -0.001032 |
| `scidocs` | +0.034286 | -0.000688 | -0.011594 | -0.006190 |
| `scifact` | +0.000000 | -0.001992 | -0.006710 | -0.003820 |
| `trec-covid` | +0.005316 | +0.000000 | +0.012543 | +0.011393 |
| `webis-touche2020` | +0.000041 | +0.000000 | -0.006317 | -0.006017 |

## Query-Level Classes

- `clean_gain`: 140
- `damage`: 184
- `mixed`: 52
- `unchanged`: 563

## Protect Underfill

For the selected policy, `0` queries had fewer base candidates than the protected rank count. In those cases the tail run fills the missing ranks.

## Verdict

If the selected protected-tail policy improves all macro metrics, promote it to the next validation route. It should still be replayed on another split/seed or a native path before becoming a training target.
