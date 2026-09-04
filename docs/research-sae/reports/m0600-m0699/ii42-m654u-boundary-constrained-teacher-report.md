# M654U Boundary-Constrained Teacher

Status: `canary_positive_not_promoted`

M654U is a first-stage dense-equivalence probe.  It keeps the P1.3/M549U
document posting geometry frozen, trains only the query-side compiler, and
does not use BM25, rerankers, learned gates, or qrels-driven objectives.

## Change

M654S showed that ridge-teacher transfer had a narrow safe window: the best
scale improved tail overlap but failed held-out O@100 on one query.  M654U
changes the teacher construction instead of enlarging the model:

- fit the same ridge teacher on dense/P1 candidate docs;
- generate small blended teacher variants between P1 and ridge;
- accept only per-query variants that preserve dense O@100 and support versus
  P1 baseline;
- select the safest variant by O@256 improvement and dense score-fit;
- train the same `p1_support_residual` query compiler on those targets.

Qrels are used only for held-out metric reporting.

## Runs

### Smoke: `m654u_smoke_fiqa_seed6544`

The FiQA smoke selected a trained checkpoint and passed the held-out
dense-equivalence gate.  The final test deltas were effectively neutral:
O@100, O@256, Recall@100, CUB, support, MAP, NDCG, and MRR did not regress,
but the test split did not show meaningful movement.

### Canary: `m654u_canary_boundary_seed6544`

Surface: `fiqa,arguana,scidocs,cqadupstack` with all-query split.

Selected checkpoint: epoch 3, global step 15.

| Metric | P1 native | M654U | Delta |
| --- | ---: | ---: | ---: |
| CUB | 0.975000 | 0.975000 | 0.000000 |
| Dense O@100 | 0.947560 | 0.947560 | 0.000000 |
| Dense O@256 | 0.947433 | 0.947666 | +0.000233 |
| NDCG@10 | 0.670993 | 0.670993 | 0.000000 |
| MAP@100 | 0.605641 | 0.605650 | +0.000009 |
| Recall@100 | 0.906005 | 0.906005 | 0.000000 |
| MRR@20 | 0.712582 | 0.712582 | 0.000000 |
| Support cosine | 0.989813 | 0.989813 | -0.000000 |
| Active support | 1.000000 | 1.000000 | 0.000000 |

Decision: `dense_equivalence_gate_passed`.

Teacher summary on the canary test split:

- moved query rate: 1.000000;
- mean selected scale: 0.0082875;
- teacher O@100 delta: 0.000000;
- teacher O@256 delta: +0.000292969;
- teacher fit MSE delta: -0.000200184;
- teacher support cosine delta: -0.000000687.

## Boundary Ledger

The query-level ledger shows no O@100-loss query in the selected checkpoint.

| Split | Rows | Boundary classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| dev | 40 | no_overlap_movement=38, tail_gain_head_flat=2 | 0.000000000 | +0.000195313 | 0.000000000 | 0.000000000 |
| test | 40 | no_overlap_movement=38, tail_gain_head_flat=2 | 0.000000000 | +0.000195313 | 0.000000000 | +0.000008735 |

This directly addresses the M654S failure mode: the tail movement now happens
without the held-out O@100 boundary loss.

## Conclusion

M654U is the first M654 variant with a trained checkpoint that passes the
four-dataset held-out dense-equivalence gate.  The effect size is small, but it
is directionally correct and matches the intended shape: preserve O@100,
support, CUB, and Recall while moving a few tail documents closer to dense.

This was not enough for promotion to the main P1 baseline, so it was expanded
to shared15.

## Shared15 Follow-Up

Run: `m654u_shared15_boundary_seed6545`.

Surface: all 15 shared15 datasets, all-query split.

Selected checkpoint: epoch 11, global step 187.

Decision: `dense_equivalence_gate_failed`.

Failed check: `dense_overlap_100_safe`.

| Metric | P1 native | M654U | Delta |
| --- | ---: | ---: | ---: |
| CUB | 0.954421 | 0.954421 | 0.000000 |
| Dense O@100 | 0.945478 | 0.945430 | -0.000048 |
| Dense O@256 | 0.949211 | 0.949233 | +0.000022 |
| NDCG@10 | 0.744424 | 0.744424 | 0.000000 |
| MAP@100 | 0.629055 | 0.629061 | +0.000006 |
| Recall@100 | 0.827751 | 0.827751 | 0.000000 |
| MRR@20 | 0.842907 | 0.842907 | 0.000000 |
| Support cosine | 0.990142 | 0.990142 | -0.000000 |
| Active support | 1.000000 | 1.000000 | 0.000000 |

The teacher itself remained mostly safe on held-out test: moved query rate
0.992537, teacher O@100 delta +0.0000746, teacher O@256 delta +0.000379, and
fit MSE delta -0.000242.  The failure happens during global compiler transfer,
not during teacher construction.

Shared15 boundary ledger:

| Split | Rows | Boundary classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| dev | 134 | head_gain_tail_flat=2, no_overlap_movement=125, o100_flat_o256_loss=1, tail_gain_head_flat=6 | +0.000149254 | +0.000145756 | 0.000000000 | -0.000008920 |
| test | 134 | head_gain_tail_flat=1, no_overlap_movement=128, o100_flat_o256_loss=1, o100_loss_o256_flat=2, tail_gain_head_flat=2 | -0.000074627 | +0.000029151 | 0.000000000 | +0.000005676 |

The two held-out O@100-loss queries were in `climate-fever` and `scidocs`.
Recall, CUB, MRR, and NDCG did not regress, but the strict first-stage rule
rejects the checkpoint because dense O@100 is negative.

## Next Step

M654U should not be promoted.  It should be kept as a useful teacher-shaping
fix: it proves boundary-constrained teacher construction can remove the M654S
canary failure, but shared15 shows the global compiler still needs an explicit
ranking-boundary preservation objective.

The next first-stage attempt should not be another scale sweep.  It should add
a dense-only boundary preservation loss over P1/dense top100 membership during
training, so the compiler learns not to drop baseline dense-overlap documents
while distilling the boundary-constrained teacher.
