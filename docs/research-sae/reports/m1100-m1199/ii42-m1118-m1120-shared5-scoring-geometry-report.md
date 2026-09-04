# ii42 M1118-M1120 Shared5 Scoring Geometry Report

## Verdict

M1118 successfully broadened the M1110-M1117 scoring-geometry probe from the
original three-dataset export to a shared5 smoke surface.

The selector/blend direction did not survive the broader surface. The useful
new signal is different: the atom score is helpful, but the current training
pushes atom pressure too hard. On heldout, a smaller fixed additive alpha is
much better for top-rank quality than the previous `0.5` baseline.

Do not continue selector/blend micro-tuning on this surface.

## Surface

Remote run:

`/home/huoju/leask/runs/ii42-m1118-shared5-profile-export-v1`

Local copy:

`/Volumes/Betty/Tmp/ii42-m1000/m1118-shared5-profile-export-v1`

Datasets:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`

This is a smoke-size shared5 surface. Each row has about 2k docs and 100 qrels
queries. It is broader than M1110, but not full shared15.

## M1118 Fixed Profile Replay

Heldout macro:

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | 0.685857 | 0.535684 | 0.480523 | 0.373882 |
| `additive_atom_0.25` | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| `additive_atom_0.5` | 0.699701 | 0.544098 | 0.482291 | 0.382090 |
| `additive_atom_0.75` | 0.701553 | 0.526631 | 0.466498 | 0.370820 |
| `additive_atom_1` | 0.706546 | 0.497089 | 0.442304 | 0.342909 |

`additive_atom_0.25` is the best top-rank profile on this surface. It improves
over lexical on all four metrics and improves strongly over `0.5` on MRR,
NDCG, and MAP.

Against `additive_atom_0.5`:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.011444 |
| MRR@20 | +0.022063 |
| NDCG@10 | +0.017048 |
| MAP@100 | +0.013275 |

This is a tradeoff: lower atom pressure improves rank quality but gives back
some Recall@100.

## M1115/M1116 Selector Replay

The rank-geometry selector failed on shared5.

Heldout selector versus `additive_atom_0.5`:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.000711 |
| MRR@20 | -0.020812 |
| NDCG@10 | -0.019822 |
| MAP@100 | -0.016956 |

LODO macro also failed:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.000896 |
| MRR@20 | -0.011376 |
| NDCG@10 | -0.016931 |
| MAP@100 | -0.015800 |

Conclusion: the M1115 positive result was local to the original 3-dataset
export. It should not be scaled.

## M1117 Blend Replay

Fixed base/alternate blending also failed on shared5.

Train-selected blend weight: `0.6`

Heldout delta versus base:

| Metric | Delta |
| --- | ---: |
| Recall@100 | -0.000711 |
| MRR@20 | -0.003721 |
| NDCG@10 | -0.006931 |
| MAP@100 | -0.007142 |

Conclusion: blending `additive_atom_0.5` and `lex_residual_atom_0.75` is not
the right broader-surface scoring geometry.

## M1120 Fine Alpha Grid

Fine grid shows a train/heldout mismatch.

Train split monotonically prefers larger alpha; the selected alpha is `0.75`.
Heldout best alpha is `0.25`.

Train-selected `0.75` is bad on heldout:

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical_alpha_0` | +0.015697 | -0.009053 | -0.014025 | -0.003063 |
| `additive_alpha_0.5` | +0.001852 | -0.017467 | -0.015793 | -0.011271 |

This is the key diagnosis: the atom rank objective overfits. It keeps buying
more training MAP/NDCG/MRR as alpha increases, but heldout top-rank quality
peaks at much lower atom pressure.

## Route Decision

Stop these sub-routes:

- M1115 rank-geometry selector;
- M1116 LODO selector validation;
- M1117 base/alternate blending;
- train-selected alpha tuning on the current split.

Keep these facts:

1. Atom signal is real: even alpha `0.25` beats lexical on all macro metrics.
2. The previous `0.5` default is too aggressive on shared5 top-rank metrics.
3. High atom pressure mainly buys Recall but spends MRR/NDCG/MAP.
4. Training split is not a reliable alpha-selection surface.

## Next Direction

The next useful experiment is not another alpha/selector replay. It should
change the training objective or checkpoint selection:

1. Add a validation-aware checkpoint gate for atom utility training.
2. Test lower-rank-pressure or stronger background/lexical preservation.
3. Keep alpha `0.25` as a diagnostic heldout-best candidate, but do not promote
   it as a default until it survives broader validation or qrels-independent
   selection.

Practical next smoke:

- run a lower-pressure M1121 training on the same shared5 surface;
- compare the fine alpha grid again;
- accept only if the train-selected alpha no longer overfits and heldout
  improves over both lexical and current `0.5` without severe per-dataset loss.
