# ii42 M1119 Fixed Additive-Alpha Profile Summary

Verdict: `promote_alpha_025_candidate`

- Best heldout profile: `additive_atom_0.25`

## Heldout Macro

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.25` | 0.686644 | 0.550144 | 0.490277 | 0.386123 |
| `additive_atom_0.5` | 0.693647 | 0.537218 | 0.479138 | 0.375236 |
| `lexical` | 0.676472 | 0.535758 | 0.480523 | 0.374146 |
| `additive_atom_0.75` | 0.694638 | 0.517818 | 0.464006 | 0.364490 |
| `additive_atom_1` | 0.703220 | 0.486040 | 0.432343 | 0.341442 |

## Heldout Delta Versus Additive 0.5

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | -0.017175 | -0.001460 | +0.001385 | -0.001090 |
| `additive_atom_0.25` | -0.007003 | +0.012926 | +0.011140 | +0.010887 |
| `additive_atom_0.75` | +0.000991 | -0.019400 | -0.015131 | -0.010747 |
| `additive_atom_1` | +0.009573 | -0.051178 | -0.046794 | -0.033794 |

## Per Dataset Heldout Delta: Additive 0.25 vs 0.5

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.045784 | +0.052152 | +0.044774 |
| `fiqa` | -0.033333 | -0.005159 | +0.001764 | -0.010187 |
| `nfcorpus` | -0.008346 | -0.005093 | -0.020797 | -0.007176 |
| `scidocs` | +0.006667 | +0.009376 | +0.009321 | +0.011582 |
| `scifact` | +0.000000 | +0.019722 | +0.013258 | +0.015440 |
