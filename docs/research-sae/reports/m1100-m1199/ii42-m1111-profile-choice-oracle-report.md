# ii42 M1111 Profile Choice Oracle

Verdict: `proceed_selector_smoke`

## Heldout Metrics

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `alternate` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |
| `oracle` | 0.522862 | 0.423594 | 0.352961 | 0.284616 |

## Oracle Delta Versus Base

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.010316 | +0.022390 | +0.017346 | +0.018486 |

## Feature Separability

| Feature | AUC |
| --- | ---: |
| `atom_nonzero` | 0.702978 |
| `atom_std` | 0.699888 |
| `lex_std` | 0.683663 |
| `candidate_count` | 0.676183 |
| `lex_nonzero` | 0.590638 |
| `atom_minus_lex_std` | 0.555907 |
| `lex_atom_top100_jaccard` | 0.501545 |
