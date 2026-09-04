# ii42 M1140 Tail-Only Expansion Replay

## Objective

Test a structural rank-composition policy instead of another gamma, alpha, or selector loop. The policy keeps the base top ranks fixed and only uses the tail run as an expansion source.

## Macro Results

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.658735 | 0.673472 | 0.578677 | 0.441150 |
| `tail` | 0.665262 | 0.680417 | 0.582149 | 0.444132 |
| `protect5_then_tail` | 0.665757 | 0.674095 | 0.582223 | 0.441537 |
| `protect10_then_tail` | 0.666648 | 0.673708 | 0.578677 | 0.441713 |
| `protect15_then_tail` | 0.671016 | 0.673978 | 0.578677 | 0.443023 |
| `protect20_then_tail` | 0.671660 | 0.673472 | 0.578677 | 0.442717 |
| `protect25_then_tail` | 0.671567 | 0.673472 | 0.578677 | 0.442745 |
| `protect30_then_tail` | 0.672114 | 0.673472 | 0.578677 | 0.442613 |
| `protect40_then_tail` | 0.670370 | 0.673472 | 0.578677 | 0.442443 |
| `protect50_then_tail` | 0.668008 | 0.673472 | 0.578677 | 0.442472 |

## Delta Versus Base

| Policy | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tail` | +0.006527 | +0.006945 | +0.003472 | +0.002982 |
| `protect5_then_tail` | +0.007022 | +0.000623 | +0.003546 | +0.000388 |
| `protect10_then_tail` | +0.007913 | +0.000236 | +0.000000 | +0.000564 |
| `protect15_then_tail` | +0.012281 | +0.000506 | +0.000000 | +0.001873 |
| `protect20_then_tail` | +0.012925 | +0.000000 | +0.000000 | +0.001568 |
| `protect25_then_tail` | +0.012832 | +0.000000 | +0.000000 | +0.001595 |
| `protect30_then_tail` | +0.013379 | +0.000000 | +0.000000 | +0.001463 |
| `protect40_then_tail` | +0.011635 | +0.000000 | +0.000000 | +0.001293 |
| `protect50_then_tail` | +0.009273 | +0.000000 | +0.000000 | +0.001322 |

## `protect5_then_tail` Per-Dataset Delta

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | -0.033333 | -0.005991 | -0.010997 | -0.005991 |
| `cqadupstack` | +0.071829 | -0.000918 | +0.007512 | +0.015057 |
| `fiqa` | +0.008333 | +0.009921 | +0.001793 | +0.002422 |
| `nfcorpus` | -0.014988 | -0.002447 | +0.002951 | -0.011601 |
| `scidocs` | +0.035000 | +0.006164 | -0.004047 | +0.002818 |
| `scifact` | +0.000000 | -0.001746 | -0.010657 | -0.003987 |
| `trec-covid` | +0.006245 | +0.000000 | +0.024778 | +0.002195 |
| `webis-touche2020` | -0.016910 | +0.000000 | +0.017033 | +0.002188 |

## Query-Level Classes

- `clean_gain`: 54
- `damage`: 46
- `mixed`: 17
- `unchanged`: 93

## Protect Underfill

For the selected policy, `1` queries had fewer base candidates than the protected rank count. In those cases the tail run fills the missing ranks.

## Verdict

If the selected protected-tail policy improves all macro metrics, promote it to the next validation route. It should still be replayed on another split/seed or a native path before becoming a training target.
