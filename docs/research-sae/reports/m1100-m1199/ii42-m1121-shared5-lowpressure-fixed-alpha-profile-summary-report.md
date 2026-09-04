# ii42 M1119 Fixed Additive-Alpha Profile Summary

Verdict: `keep_alpha_05_baseline`

- Best heldout profile: `additive_atom_0.25`

## Heldout Macro

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.25` | 0.698231 | 0.551322 | 0.492654 | 0.390480 |
| `additive_atom_0.5` | 0.715565 | 0.540206 | 0.485608 | 0.386185 |
| `lexical` | 0.676767 | 0.535706 | 0.480540 | 0.373828 |
| `additive_atom_0.75` | 0.712528 | 0.526439 | 0.471640 | 0.372902 |
| `additive_atom_1` | 0.714145 | 0.500386 | 0.449488 | 0.353192 |

## Heldout Delta Versus Additive 0.5

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | -0.038797 | -0.004500 | -0.005067 | -0.012357 |
| `additive_atom_0.25` | -0.017333 | +0.011117 | +0.007047 | +0.004296 |
| `additive_atom_0.75` | -0.003037 | -0.013767 | -0.013968 | -0.013283 |
| `additive_atom_1` | -0.001420 | -0.039819 | -0.036120 | -0.032993 |

## Per Dataset Heldout Delta: Additive 0.25 vs 0.5

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.036918 | +0.035981 | +0.036000 |
| `fiqa` | -0.100000 | -0.018771 | -0.020458 | -0.022598 |
| `nfcorpus` | +0.000000 | +0.006614 | -0.002409 | -0.002828 |
| `scidocs` | +0.013333 | +0.025266 | +0.019444 | +0.007676 |
| `scifact` | +0.000000 | +0.005556 | +0.002676 | +0.003229 |
