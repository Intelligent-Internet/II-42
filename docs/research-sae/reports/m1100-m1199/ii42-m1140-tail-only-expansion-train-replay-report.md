# ii42 M1140 Tail-Only Expansion Replay

## Objective

Test a structural rank-composition policy instead of another gamma, alpha, or selector loop. The policy keeps the base top ranks fixed and only uses the tail run as an expansion source.

## Macro Results

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.864721 | 0.889357 | 0.830948 | 0.750745 |
| `tail` | 0.867144 | 0.885913 | 0.825487 | 0.743246 |
| `protect10_then_tail` | 0.867323 | 0.889248 | 0.830948 | 0.751074 |
| `protect20_then_tail` | 0.867554 | 0.889357 | 0.830948 | 0.751708 |
| `protect50_then_tail` | 0.867444 | 0.889357 | 0.830948 | 0.752002 |
| `protect75_then_tail` | 0.869621 | 0.889357 | 0.830948 | 0.751532 |
| `protect90_then_tail` | 0.870004 | 0.889357 | 0.830948 | 0.751354 |

## Delta Versus Base

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tail` | +0.002423 | -0.003444 | -0.005461 | -0.007500 |
| `protect10_then_tail` | +0.002602 | -0.000108 | +0.000000 | +0.000329 |
| `protect20_then_tail` | +0.002833 | +0.000000 | +0.000000 | +0.000963 |
| `protect50_then_tail` | +0.002724 | +0.000000 | +0.000000 | +0.001257 |
| `protect75_then_tail` | +0.004900 | +0.000000 | +0.000000 | +0.000786 |
| `protect90_then_tail` | +0.005283 | +0.000000 | +0.000000 | +0.000609 |

## `protect10_then_tail` Per-Dataset Delta

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `climate-fever` | +0.007143 | +0.000128 | +0.000000 | +0.000484 |
| `cqadupstack` | -0.002821 | -0.001299 | +0.000000 | -0.001109 |
| `dbpedia-entity` | +0.012188 | +0.000000 | +0.000000 | +0.004121 |
| `fever` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `fiqa` | -0.003333 | -0.001767 | +0.000000 | -0.002603 |
| `hotpotqa` | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `msmarco` | -0.002709 | +0.000000 | +0.000000 | -0.009158 |
| `nfcorpus` | +0.006159 | +0.003222 | +0.000000 | +0.009386 |
| `nq` | -0.014286 | +0.000000 | +0.000000 | -0.000246 |
| `quora` | +0.000000 | +0.000000 | +0.000000 | -0.000306 |
| `scidocs` | +0.031429 | -0.000752 | +0.000000 | -0.002059 |
| `scifact` | +0.000000 | -0.001158 | +0.000000 | -0.000853 |
| `trec-covid` | +0.005227 | +0.000000 | +0.000000 | +0.011217 |
| `webis-touche2020` | +0.000041 | +0.000000 | +0.000000 | -0.003942 |

## Query-Level Classes

- `clean_gain`: 145
- `damage`: 173
- `mixed`: 10
- `unchanged`: 611

## Protect Underfill

For the selected policy, `0` queries had fewer base candidates than the protected rank count. In those cases the tail run fills the missing ranks.

## Verdict

If the selected protected-tail policy improves all macro metrics, promote it to the next validation route. It should still be replayed on another split/seed or a native path before becoming a training target.
