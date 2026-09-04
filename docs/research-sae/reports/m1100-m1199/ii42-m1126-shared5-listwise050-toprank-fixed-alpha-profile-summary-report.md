# ii42 M1119 Fixed Additive-Alpha Profile Summary

Verdict: `keep_alpha_05_baseline`

- Best heldout profile: `additive_atom_0.75`

## Heldout Macro

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `additive_atom_0.75` | 0.740822 | 0.580870 | 0.509204 | 0.405299 |
| `additive_atom_0.5` | 0.721448 | 0.577975 | 0.510147 | 0.404917 |
| `additive_atom_0.25` | 0.709274 | 0.568016 | 0.507646 | 0.399619 |
| `additive_atom_1` | 0.743572 | 0.556419 | 0.491337 | 0.390538 |
| `lexical` | 0.673136 | 0.535684 | 0.480523 | 0.373937 |

## Heldout Delta Versus Additive 0.5

| Profile | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | -0.048312 | -0.042291 | -0.029624 | -0.030980 |
| `additive_atom_0.25` | -0.012174 | -0.009959 | -0.002501 | -0.005299 |
| `additive_atom_0.75` | +0.019374 | +0.002895 | -0.000942 | +0.000382 |
| `additive_atom_1` | +0.022124 | -0.021556 | -0.018810 | -0.014379 |

## Per Dataset Heldout Delta: Additive 0.25 vs 0.5

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | +0.014616 | +0.011635 | +0.014616 |
| `fiqa` | -0.061111 | -0.015991 | -0.019143 | -0.024489 |
| `nfcorpus` | -0.001425 | -0.001509 | -0.003275 | -0.005599 |
| `scidocs` | +0.001667 | -0.030246 | +0.005822 | -0.001123 |
| `scifact` | +0.000000 | -0.016667 | -0.007543 | -0.009898 |
