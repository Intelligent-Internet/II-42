# ii42 M1119 Fixed Additive-Alpha Profile Summary

Verdict: `promote_alpha_025_candidate`

- Best heldout profile: `additive_atom_0.25`

## Heldout Macro

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.25` | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| `additive_atom_0.5` | 0.699701 | 0.544098 | 0.482291 | 0.382090 |
| `lexical` | 0.685857 | 0.535684 | 0.480523 | 0.373882 |
| `additive_atom_0.75` | 0.701553 | 0.526631 | 0.466498 | 0.370820 |
| `additive_atom_1` | 0.706546 | 0.497089 | 0.442304 | 0.342909 |

## Heldout Delta Versus Additive 0.5

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | -0.013845 | -0.008414 | -0.001768 | -0.008208 |
| `additive_atom_0.25` | -0.011444 | +0.022063 | +0.017048 | +0.013275 |
| `additive_atom_0.75` | +0.001852 | -0.017467 | -0.015793 | -0.011271 |
| `additive_atom_1` | +0.006845 | -0.047009 | -0.039987 | -0.039181 |

## Per Dataset Heldout Delta: Additive 0.25 vs 0.5

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.044360 | +0.055605 | +0.042360 |
| `fiqa` | -0.063889 | -0.012778 | -0.011931 | -0.017617 |
| `nfcorpus` | +0.000000 | +0.000926 | -0.003587 | -0.000427 |
| `scidocs` | +0.006667 | +0.047066 | +0.021568 | +0.011563 |
| `scifact` | +0.000000 | +0.030741 | +0.023582 | +0.030496 |
