# ii42 M1119 Fixed Additive-Alpha Profile Summary

Verdict: `keep_alpha_05_baseline`

- Best heldout profile: `additive_atom_0.25`

## Heldout Macro

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.25` | 0.703053 | 0.553144 | 0.496855 | 0.392528 |
| `additive_atom_0.5` | 0.712690 | 0.543900 | 0.494837 | 0.391132 |
| `additive_atom_0.75` | 0.725942 | 0.546375 | 0.491363 | 0.389887 |
| `additive_atom_1` | 0.727392 | 0.536679 | 0.476797 | 0.381246 |
| `lexical` | 0.676728 | 0.535706 | 0.480523 | 0.374963 |

## Heldout Delta Versus Additive 0.5

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | -0.035962 | -0.008194 | -0.014314 | -0.016169 |
| `additive_atom_0.25` | -0.009637 | +0.009244 | +0.002018 | +0.001396 |
| `additive_atom_0.75` | +0.013252 | +0.002476 | -0.003473 | -0.001245 |
| `additive_atom_1` | +0.014702 | -0.007221 | -0.018040 | -0.009886 |

## Per Dataset Heldout Delta: Additive 0.25 vs 0.5

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.012712 | +0.010265 | +0.012712 |
| `fiqa` | -0.038095 | -0.019737 | -0.015449 | -0.030036 |
| `nfcorpus` | -0.003423 | +0.004167 | -0.018582 | -0.008610 |
| `scidocs` | -0.006667 | +0.017729 | +0.010544 | +0.004602 |
| `scifact` | +0.000000 | +0.031349 | +0.023310 | +0.028313 |
