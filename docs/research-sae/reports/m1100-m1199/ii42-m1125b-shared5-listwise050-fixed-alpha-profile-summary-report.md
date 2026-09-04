# ii42 M1119 Fixed Additive-Alpha Profile Summary

Verdict: `keep_alpha_05_baseline`

- Best heldout profile: `additive_atom_0.5`

## Heldout Macro

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.5` | 0.715489 | 0.562618 | 0.498005 | 0.391700 |
| `additive_atom_0.25` | 0.698409 | 0.557445 | 0.498674 | 0.385855 |
| `additive_atom_0.75` | 0.722504 | 0.555430 | 0.488303 | 0.380428 |
| `lexical` | 0.670057 | 0.535730 | 0.480523 | 0.374258 |
| `additive_atom_1` | 0.726973 | 0.523477 | 0.452921 | 0.356220 |

## Heldout Delta Versus Additive 0.5

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | -0.045432 | -0.026888 | -0.017482 | -0.017442 |
| `additive_atom_0.25` | -0.017080 | -0.005173 | +0.000669 | -0.005846 |
| `additive_atom_0.75` | +0.007015 | -0.007188 | -0.009701 | -0.011272 |
| `additive_atom_1` | +0.011484 | -0.039141 | -0.045083 | -0.035480 |

## Per Dataset Heldout Delta: Additive 0.25 vs 0.5

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.018241 | +0.033975 | +0.018241 |
| `fiqa` | -0.071429 | -0.026599 | -0.019524 | -0.023763 |
| `nfcorpus` | -0.000638 | +0.005435 | -0.006367 | -0.004094 |
| `scidocs` | -0.013333 | -0.009052 | +0.005512 | -0.004396 |
| `scifact` | +0.000000 | -0.013889 | -0.010249 | -0.015215 |
