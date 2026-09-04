# M631 / P1.5 Feature-Channel Probe Report

Status: M631-B completed; no feature family promoted.

## Objective

M631-B tests whether individual P1/posting feature families can distinguish
dense-tail positives from displaced head negatives in a way that is useful for
first-stage top100 recovery.

The probe deliberately avoids a mixed large model.  Each family is trained as a
small global logistic probe over M631-A contrast rows, then evaluated through a
conservative top100 swap simulation.

## Generated Artifacts

| Artifact | Path |
| --- | --- |
| Probe JSON | `runs/m631_p1p5_dense_tail_probe_smoke_v1/m631_feature_probe.json` |
| Probe MD | `runs/m631_p1p5_dense_tail_probe_smoke_v1/m631_feature_probe.md` |
| Eval-style rows | `runs/m631_p1p5_dense_tail_probe_smoke_v1/eval_style/m631_p1p5_smoke/` |

## Probe Matrix

| Family | Gate | Eval AUC | Pairwise | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap | dKL |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `p1_geometry` | `no_recall_signal` | 1.000000 | 1.000000 | -0.657403 | -0.368603 | -0.035577 | -0.740585 | -0.100000 | +20.723266 |
| `atom_overlap` | `no_recall_signal` | 0.530982 | 0.543237 | -0.612535 | -0.359032 | -0.005951 | -0.635701 | -0.090784 | +0.070852 |
| `signed_coordinate_margin` | `no_recall_signal` | 0.531566 | 0.624908 | -0.631761 | -0.360632 | -0.004272 | -0.636543 | -0.095294 | +0.064644 |
| `support_conflict_penalty` | `no_recall_signal` | 0.763064 | 0.872136 | -0.659686 | -0.367503 | -0.016090 | -0.722894 | -0.100000 | +19.193574 |
| `query_local_atom_residual` | `no_recall_signal` | 0.821695 | 0.997044 | -0.652737 | -0.368640 | -0.035624 | -0.709976 | -0.100000 | +20.663668 |
| `lexical_coverage` | `no_recall_signal` | 0.517671 | 0.525499 | -0.138108 | -0.045380 | -0.001821 | -0.112151 | -0.087451 | -0.112893 |

## Key Finding

The important result is negative but informative: high separability does not
translate into safe top100 promotion.

`p1_geometry` has perfect AUC and pairwise agreement, but its swap simulation
destroys ranking metrics.  `query_local_atom_residual` also has very high
pairwise agreement, but Recall@100 decreases.  The only family that reduces KL
is `lexical_coverage`, and it still decreases Recall@100, MAP@100, NDCG@10,
MRR@20, and dense overlap.

This means the current hand-designed feature channels can identify a dense-tail
contrast shape, but they cannot safely decide which P1 top100 head documents
should be displaced.

## Decision

M631-B stops with:

- Status: `stop_no_recall_bearing_feature_channel`
- Best family by guarded selection: `lexical_coverage`
- Reason: no feature family produced positive Recall@100 delta within the smoke
  guard.

Do not train M631-C.  Do not create M631-D posting compiler correction from
these feature families.  The next useful step is not a larger reranker over the
same features; it is an output-head/posting-compiler training stage that learns
a better first-stage support channel directly.
