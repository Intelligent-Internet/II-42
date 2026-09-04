# M1530C Hard-Soft Distillation And Route Decision

Decision: **stop the M1530 cross-encoder-supervised retrieval-head branch**.
Do not tune the hard-loss weight, increase rank, add epochs, or scale this
teacher surface to 10K/1K.

## Question

M1530B showed that a rank-8 shared basis could reduce soft teacher KL by almost
10% without damaging posting cost, but teacher top1 regressed. M1530C kept the
same model and added a fixed weight-1 teacher-argmax cross-entropy term to test
whether soft KL simply underweighted the winning candidate.

Every other input and training choice was locked. The accepted runtime smoke
reproduced M1530A step 0 with maximum metric difference `0.0`. The formal
trainer SHA-256 was
`33f6280161e6a342a6ae3370758f715c98804cae0f326aa328a1b4626d26e43d`
and the ClearML task was `1e650644aa554076adf00463a61192c1`.

## Result

| Step | KL | Teacher top1 | Pair order | Positive top1 | Touch mean | Max DF | Active concepts |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 1.508126 | 0.476562 | 0.837573 | 0.460938 | 0.078369 | 0.100694 | 21,315 |
| 100 | 1.501148 | **0.484375** | 0.836595 | 0.468750 | 0.078010 | 0.100694 | 21,303 |
| 300 | 1.440785 | 0.460938 | 0.834638 | 0.445312 | 0.078356 | 0.101562 | 21,344 |
| 500 | 1.429096 | 0.468750 | 0.837573 | 0.437500 | 0.080288 | 0.102431 | 21,387 |
| 650 | 1.419739 | 0.476562 | 0.837573 | 0.453125 | 0.081943 | 0.105035 | 21,313 |
| 800 | **1.397904** | 0.476562 | **0.839530** | 0.445312 | 0.083110 | 0.106771 | 21,235 |

At the lowest-KL checkpoint:

- KL improved by 7.31%, below the required 10%;
- teacher top1 did not improve;
- pair-order improved by only 0.001957;
- positive top1 regressed by 0.015625;
- touch increased by 6.05% relative but remained far below its gate;
- max DF increased by 0.006076 absolute;
- active vocabulary decreased by 0.38% relative.

The hard term restored M1530B's best top1 only to the baseline while giving up
M1530B's better KL. No checkpoint passed the strict gate or dominated the
M1530A frontier. All numerical and posting-integrity checks passed.

## Cross-Stage Evidence

| Route | Best diagnostic step | KL | KL change | Teacher top1 | Top1 change | Pair order | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M1530A independent 32K head | 250 | 1.441690 | -4.41% | 0.500000 | +0.023438 | 0.835616 | fail |
| M1530B rank-8 soft KL | 500 | **1.357449** | **-9.99%** | 0.468750 | -0.007812 | 0.838552 | fail |
| M1530C rank-8 hard+soft | 800 | 1.397904 | -7.31% | 0.476562 | 0.000000 | **0.839530** | fail |

The experiments isolate three facts:

1. Independent centroid updates have enough freedom to move some winners but
   generalize poorly on the bounded surface.
2. A shared low-rank basis is much more sample-efficient for distribution
   fitting and preserves posting cost.
3. Explicit teacher-winner supervision does not turn that fit into stable
   winner agreement. The remaining failure is not just KL weighting.

## Structural Diagnosis

The M1518 teacher artifact contains frozen cross-encoder scores. A
cross-encoder computes query-document interaction features jointly. M1530's
student must factor every score into two independent representations:

```text
student_score(q, d) = dot(sparse_query(q), sparse_document(d))
```

Cross-encoder scores are not guaranteed to admit this factorization, before
the additional constraints of a shared 32K vocabulary and hard TopK are even
applied. The observed frontier is consistent with this mismatch: the student
can fit the soft distribution, a subset of winners, or local pair order, but
not all three at once. More rank or a different hard-loss weight would add
capacity around a teacher that may be structurally unrealizable.

This result closes cross-encoder-supervised M1530. It does **not** show that a
posting compiler cannot preserve a dense dual encoder, because that teacher
has the same factored query/document score form as the target.

## Recommended New Route

Start a new route, not M1530D:

**M1540 factorized-dense URSI distillation**

1. Use a frozen dense dual encoder to generate query and document embeddings
   and their dot-product score matrix. Do not read cross-encoder scores, BM25,
   or qrels in the first stage.
2. Train a bounded posting compiler against dense score margins, candidate
   distribution, and dense topK membership. The teacher and student are then
   mathematically score-factorized in the same way.
3. Begin with an oracle/capacity audit on the same 256/128 texts, then increase
   independent text coverage rather than repeating a tiny dataset for more
   epochs.
4. Require dense-overlap, score-correlation, margin, touch, DF, and active
   vocabulary gates before any retrieval expansion or BM25 stage.
5. Only after dense equivalence is demonstrated should a second objective add
   retrieval usefulness beyond dense.

This route returns to the project's core first-stage question with a
realizable teacher and preserves the useful M1530 engineering: exact identity
controls, shared low-rank adaptation, bounded posting measurements, ClearML,
artifact hashes, and conjunctive selection.

## Artifacts

Bounded JSON, JSONL, and logs are mirrored under
`runs/m1530c_hard_soft_distillation_v1/`. Small failed checkpoints remain on
`spark-1` and are not promoted.
