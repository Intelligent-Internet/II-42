# M1701 Listwise Hard-Negative Curriculum Report

## Status

M1701 keeps the product boundary unchanged: one vocabulary-sparse encoder, one
posting map, and one exact native/BMP inverted index. RankT5 is used only to
construct training supervision; it is absent at inference.

Current decision: **stop M1701A at native shared3**. The deterministic depth
audit selected step 3 without using qrels, but its exact native replay reduced
NFCorpus candidate upper bound by `0.007267`. This failed the locked
non-negative macro-quality gate even though macro NDCG, MAP, Recall, and MRR
were positive. No full-FiQA run or native promotion is authorized.

## Teacher Gate

The initial MiniLM cross-encoder was informative but failed the predeclared
teacher-strength gate on 2,048 rows. RankT5-3B was therefore the single allowed
escalation. It passed first on 512 rows and then on the locked 2,048-row surface.

| Surface | Root top1 | Teacher top1 | Gain | Root pairwise | Teacher pairwise | Gain |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| RankT5 512 | 0.736328 | 0.810547 | +0.074219 | 0.926302 | 0.959961 | +0.033659 |
| RankT5 2,048 | 0.747559 | 0.816895 | +0.069336 | 0.937305 | 0.964893 | +0.027588 |

The 2,048-row artifact contains 1,673 eligible rows where RankT5 places the
known positive first. The model revision is
`40e2b98fcbc6d60457c88508bd775fcb8395a5b0`.

## Objective Transfer Audit

Directly transferring CADET's dense temperatures failed before training:

| Term | Raw loss | Applied weight | Representation-gradient norm |
| --- | ---: | ---: | ---: |
| listwise, student tau 0.05 | 236.925659 | 1.0 | 26.205457 |
| contrastive, tau 0.01 | 0.000000 | 0.1 | 0.000000 |
| document FLOPS | 1.213009 | 0.05 | 0.000897 |

The sparse root's locked-temperature entropy was `0.026769`, while the teacher
entropy was `2.397814`. CADET's encoder normalizes dense representations, but
the native sparse root uses an unnormalized dot product. The mismatch made the
listwise loss almost one-hot and the contrastive loss exactly saturated.

M1701A applies one deterministic correction rather than a grid. The student
temperature is derived from the root/teacher median row-score standard
deviations:

```text
tau_student = 9.063416 / 0.219615 * 0.3 = 12.380894
```

Contrastive learning operates on L2-normalized representations, matching the
CADET implementation, while native inference remains raw sparse dot product.
Document FLOPS becomes a GEM hard constraint in parameter space.

| Term | Raw loss | Applied role | Gradient norm |
| --- | ---: | --- | ---: |
| listwise | 0.064001 | primary | 0.017513 |
| contrastive | 0.024765 | auxiliary, weight 0.1 | 0.030266 |
| document FLOPS | 1.213009 | hard constraint | 0.017947 |

Retrieval-term dominance fell to `1.728x`; listwise/contrastive cosine was
`+0.1237`. Retrieval/FLOPS cosine was `-0.0010`, so a projected four-update
canary was authorized.

## Projected Four-Update Canary

The exact GradCache trainer computes listwise, contrastive, and cost gradients
separately, replays them through the model, protects the primary listwise
gradient, and projects the combined retrieval gradient into the non-conflicting
FLOPS half-space before the optimizer step.

| Step | KL | Contrastive | FLOPS | Top1 | MRR | Cost projection |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | 0.057987 | 0.040001 | 1.292375 | 0.906250 | 0.945312 | yes |
| 2 | 0.065403 | 0.266936 | 1.298974 | 0.906250 | 0.942708 | yes |
| 3 | 0.062265 | 0.003859 | 1.293203 | 0.968750 | 0.984375 | yes |
| 4 | 0.072353 | 0.307028 | 1.127126 | 0.875000 | 0.927083 | no |

All gradients were finite. Peak reserved GPU memory was `6.77%`. The four rows
are different training batches and are not interpreted as a learning curve.
ClearML initialization was unavailable for this canary, so the immutable run
summary, log, checkpoint, and hashes are the audit record.

## Disjoint Teacher Surface

Rows `160-671` are disjoint from audit rows `0-31` and training rows `32-159`.

| Metric | Root | Step 4 | Delta |
| --- | ---: | ---: | ---: |
| listwise KL | 0.061704 | 0.059622 | -0.002082 |
| positive top1 | 0.855469 | 0.867188 | +0.011719 |
| positive MRR | 0.906481 | 0.912015 | +0.005534 |
| pairwise accuracy | 0.968359 | 0.969401 | +0.001042 |

Query/document FLOPS ratios are `0.9598/0.9714`; the maximum cost ratio over
all tracked fields is `1.0405`. The conjunctive teacher-surface gate passes.

## Broad And RLHN Regression

| Surface | Metric | Root | Step 4 | Delta |
| --- | --- | ---: | ---: | ---: |
| broad 2,048 | InfoNCE | 0.015496 | 0.015492 | -0.000004 |
| broad 2,048 | top1 | 0.996216 | 0.996094 | -0.000122 |
| broad 2,048 | MRR | 0.997579 | 0.997537 | -0.000043 |
| RLHN 1+15 | InfoNCE | 1.666256 | 1.567902 | -0.098354 |
| RLHN 1+15 | top1 | 0.743164 | 0.752930 | +0.009766 |
| RLHN 1+15 | MRR | 0.822418 | 0.828030 | +0.005612 |
| RLHN 1+15 | pairwise | 0.934440 | 0.936458 | +0.002018 |

Broad top1/MRR remain inside the predeclared `0.001` floor while every RLHN
metric improves. This authorizes the M1691 `2,048 x 100` ordering/cost gate,
not native retrieval by itself.

## Step-4 Ordering And Cost Gate

| Metric | Root | Step 4 | Delta | Gate |
| --- | ---: | ---: | ---: | --- |
| normalized KL | 1.656219 | 1.638493 | -0.017726 | pass |
| top1 agreement | 0.522461 | 0.518555 | -0.003906 | pass |
| pairwise agreement | 0.785161 | 0.782613 | -0.002549 | pass |
| row Spearman | 0.738735 | 0.733118 | -0.005616 | fail |

The maximum sparse-cost ratio is `1.0254`; query/document mean nnz and FLOPS
all decrease. The checkpoint is nevertheless rejected because the Spearman
floor is `-0.005`, not because of cost or teacher quality. The miss is
`0.000616` beyond the floor. The gate is not relaxed.

M1700 established that this root has a narrow early-update window. M1701A
therefore reran the identical four-update trajectory while persisting every
checkpoint. ClearML initialization initially changed the random stream; that
run was quarantined, tracking initialization was moved before an explicit
reseed, and the corrected step-4 SHA then matched the original exactly.

## Locked Depth Audit

The fixed selector first applies the unchanged M1691 ordering/cost constraints,
then requires disjoint-teacher improvement and broad/RLHN floors. It does not
use qrels or native retrieval scores.

| Step | Ordering KL delta | Top1 delta | Pairwise delta | Spearman delta | Max cost ratio | Gate |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | -0.009066 | -0.004883 | -0.000226 | -0.000533 | 1.004879 | pass |
| 2 | -0.015919 | -0.001953 | -0.000731 | -0.001633 | 1.012732 | pass |
| 3 | -0.017085 | -0.000977 | -0.001366 | -0.003037 | 1.014485 | pass |
| 4 | -0.017726 | -0.003906 | -0.002549 | -0.005616 | 1.025400 | fail |

On the disjoint teacher surface, steps 1-3 reduced KL by
`0.000992/0.001326/0.001747`; step 3 also improved top1 by `0.003906`, MRR by
`0.001707`, and pairwise accuracy by `0.000391`. Its RLHN top1/MRR/pairwise
deltas were `+0.000977/+0.001067/+0.001302`; broad top1/MRR stayed inside the
locked floor at `-0.000122/-0.000043`. The fixed selector therefore chose step
3. This is evidence for a real but narrow model-side window ending between
updates 3 and 4, not authorization to train deeper.

## Native Shared3 Stop

The selected step-3 checkpoint was replayed through the exact native block
engine on the locked NFCorpus, SciFact, and FiQA shared slices. All 648 engine
comparisons retained exact parity and the maximum cost ratio was `1.014925`.

| Dataset | NDCG delta | MAP delta | Recall delta | MRR delta | CUB delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | +0.001342 | +0.000105 | +0.000498 | -0.000216 | -0.007267 |
| SciFact | +0.007471 | +0.009670 | +0.000000 | +0.010544 | +0.000000 |
| FiQA | -0.004749 | -0.003416 | +0.000000 | -0.005761 | +0.000000 |
| Macro | +0.001355 | +0.002120 | +0.000166 | +0.001522 | -0.002422 |

The candidate passed exactness, row floors, and cost, but failed the conjunctive
macro-quality gate solely because candidate upper bound was negative. Full
FiQA is intentionally not run. Trying steps 1 or 2 after observing this native
result would be checkpoint shopping, so M1701A stops rather than beginning a
temperature, loss-weight, or depth sweep.

## Conclusion

RankT5 listwise supervision, sparse-score rescaling, normalized contrastive
learning, and parameter-space cost projection jointly produce a reproducible
early-update mechanism signal. They do not produce a deployable improvement:
the qrels-free selector's chosen checkpoint fails the first native CUB gate.
The route is closed at shared3. A future experiment must change the supervision
or source construction in a way that directly explains candidate-support loss;
ordinary depth, learning-rate, temperature, or loss-weight tuning is not
authorized by these results.
