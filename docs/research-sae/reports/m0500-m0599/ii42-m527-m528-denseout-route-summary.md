# M527-M528 Denseout Route Summary

M527 and M528 revise the post-M526 route after finding that dense-derived
posting should not be trained as an independent hidden-state support head.

## M527 Dense Surface Alignment

M527 isolates the raw dense surface:

```text
raw AutoModel mean dense -> materialized official PPLX dense
```

Training data is corpus documents only.  There are no qrels, BM25, route
positives, or ranking losses.

| Run | Source | Cosine | Active Recall | Active Jaccard | Signed Active Recall | Sign Acc | MSE |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| FiQA smoke | `raw_mean` | 0.99171 | 0.95151 | 0.91737 | 0.95151 | 0.99992 | 0.00001620 |
| FiQA smoke | `linear_adapter` | 0.97757 | 0.84889 | 0.74163 | 0.84889 | 0.99991 | 0.00004382 |
| broad4 raw-only | `raw_mean` | 0.99155 | 0.93866 | 0.89398 | 0.93866 | 0.99997 | 0.00001651 |

The linear adapter regresses the surface.  The important correction is that
raw mean dense was already much stronger than M526 implied.  M526's
`~0.825` active recall was caused by training the dense/posting heads away
from a good raw surface.

## M528 Frozen Raw-Denseout Route Gate

M528 tests the simplest non-trained route:

```text
text -> PPLX AutoModel mean dense -> support=dense -> posting route
```

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m528_raw_denseout_candidates_p128` | 0.47695 | 0.66172 | 0.52316 | 0.24932 | 0.93010 | 0.99350 | 0.90563 |
| `m528_raw_denseout_candidates_p160` | 0.47697 | 0.66179 | 0.52316 | 0.24940 | 0.93141 | 0.99676 | 0.93624 |
| `m528_raw_denseout_candidates_p192` | 0.47697 | 0.66180 | 0.52316 | 0.24942 | 0.93220 | 0.99836 | 0.95538 |
| `m528_raw_denseout_candidates_p224` | 0.47697 | 0.66175 | 0.52316 | 0.24939 | 0.93261 | 0.99906 | 0.96798 |
| `route_subset_teacher_row_int8_dense` | 0.48032 | 0.66040 | 0.53348 | 0.25556 | 1.00000 | 1.00000 | 1.00000 |

## Interpretation

The encoder/support side is now close enough for this route family.  With no
training, p128 already recovers `0.99350` candidate recall, and p224 reaches
`0.99906`.  The remaining quality gap is not caused by missing relevant
documents; it is caused by candidate-internal scoring/order.

This changes the next target.  Further LoRA/support-head training is lower
value unless it is tied to a better candidate scorer.  The next useful gate is
M529:

1. Use the M528 candidate sets.
2. Rerank candidates with teacher dense scores as an upper bound.
3. If the upper bound matches dense, train a lightweight posting-evidence
   scorer/reranker.
4. If the upper bound still misses dense, the candidate generation path itself
   is not yet sufficient despite high aggregate candidate recall.

## Artifacts

- `scripts/research_sae_m527_dense_surface_alignment.py`
- `scripts/research_sae_m528_raw_denseout_route_gate.py`
- `outputs/m527/dense_surface_alignment/m527_fiqa_linear_smoke_seed5270.json`
- `outputs/m527/dense_surface_alignment/m527_broad4_raw_only_seed5270.json`
- `outputs/m528/raw_denseout_route_gate/m528_fiqa_p224_smoke_seed5090.json`
- `outputs/m528/raw_denseout_route_gate/m528_broad4_prefix_matrix_seed5090.json`
