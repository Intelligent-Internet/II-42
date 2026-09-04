# M653 First-Stage Evidence Synthesis

Status: synthesis completed; no model promoted.

This note converts prior II-42 results plus relevant sparse-retrieval
literature into testable next steps for the first-stage P1 dense-equivalence
problem.  It intentionally excludes BM25, rerankers, qrels-driven optimization,
and learned gates.

## Current Evidence

M653-A measured `P1.3 / M549U native signed-dot` on shared15:

| Signal | Value |
| --- | ---: |
| Dense top10 in P1 top10 | 0.932563 |
| Dense top50 in P1 top50 | 0.940075 |
| Dense top100 in P1 top100 | 0.942787 |
| Dense top100 inside P1 top256 | 1.000000 |
| Dense top100 missing from P1 candidate rows | 0.000000 |
| Dense/P1 rank corr on dense top100 | 0.955226 |
| Native candidate upper bound | 0.949036 |
| Native Recall@100 | 0.872661 |

The primary gap is not support absence.  It is boundary score/rank
deformation: about `5.72%` of dense top100 documents are present inside P1
top256 but displaced below P1 top100.

M653-B adds a canary top256/raw-score dense teacher export.  On
`fiqa,arguana,scidocs,cqadupstack`, true dense/P1 O@256 is `0.950479`, dense
top100 is still fully inside P1 top256, and the raw dense margin between dense
ranks 80-100 and false P1 top100 documents is only about `0.009843`.  This
turns the next compiler step into a narrow boundary-calibration problem rather
than a broad support-recovery problem.

This repeats the smaller M630 signal, where dense top100 candidate coverage was
`1.000000` but dense top100 in P1 top100 was only `0.931600`.  M630 also showed
non-trivial dense distribution KL, so membership alone is not enough.

M615 separates this from qrels recovery.  Most under-ranked qrels positives are
not dense top100 hits, so first-stage dense-equivalence training should not try
to solve those examples directly.  They belong to a later retrieval-expansion
stage.

## Prior Route Lessons

M549U proved the target/root distinction:

- The deterministic M549 teacher is valid.
- The wrong raw `AutoModel` root produced misleading failures.
- The official ST/PPLX int8 root is the valid first-stage interface.

M546/M600 proved that KL-only or support-cosine-only improvement is unsafe:

- Monotonic/power transforms can reduce KL.
- Output heads can preserve active recall under some gates.
- Dense overlap can still regress, so rank membership must be an explicit loss
  and selection gate.

M653 local-delta work proved that per-query projected deltas are the wrong
abstraction:

- strict and relaxed dense-overlap gates accepted no useful crossing;
- lenient crossing did not improve Recall and spent dense overlap;
- the next compiler cannot be a local query delta or scalar sweep.

## Literature Signals

SPLADE v2 and related learned sparse retrieval work support the general
engineering form: a neural encoder can produce sparse inverted-index vectors,
but the useful variants rely on distillation, pooling/expansion design, and
explicit sparse retrieval objectives rather than SAE reconstruction.
Reference: <https://arxiv.org/abs/2109.10086>.

Recent score-distribution distillation work argues that hard-negative or top-k
only sampling can miss the teacher's full preference structure.  The relevant
lesson for us is to export raw dense scores and train over stratified score
bands, not just top100 membership.
Reference: <https://arxiv.org/html/2604.04734v1>.

Sparse/dense scaling studies indicate that sparse retrieval can be robust, but
KD alone may not scale as expected; contrastive and KD signals play different
roles.  For M653 this means dense score distillation should be a geometry
constraint, not a blind deeper-training recipe.
Reference: <https://arxiv.org/html/2502.15526v1>.

DeepImpact/DeeperImpact reinforces the importance of learned impact scoring and
teacher signals for sparse learned indexes.  The lesson is not to add query
expansion now, but to treat atom weights as impact scores whose rank boundary
must be learned from a teacher distribution.
Reference: <https://arxiv.org/abs/2405.17093>.

IDF-aware sparse regularization is directly relevant to our fanout/sparsity
problem.  Uniform sparsity pressure can underweight semantically important rare
dimensions; an IDF/fanout-aware penalty is a better guard if the compiler
starts moving support weights.
Reference: <https://arxiv.org/html/2411.04403v1>.

## Working Hypothesis

The current P1 first-stage bottleneck is:

```text
dense support preserved
-dense top100 boundary ordering partially deformed
-raw dense score distribution not available in current audit artifacts
```

Therefore the next breakthrough is not more capacity first.  It is better
teacher instrumentation and a boundary-aware dense-preserving objective.

## Next Testable Step: M653-B

Build aligned dense teacher artifacts with deeper rankings and raw scores:

- dense top256 or top512 doc ids;
- raw dense scores for each query/doc;
- P1 ranks/scores for the same docs;
- false P1 top100 docs not in dense top100;
- support/active metrics from the compiler surface, if training is run.

Then train only query-side/output compiler changes with:

- `top100_membership_loss`: dense top100 must beat P1 false top100;
- `rank_pair_loss`: preserve dense ordering inside top100/top256;
- `score_distribution_loss`: KL or MarginMSE over stratified dense score bands;
- `boundary_margin_loss`: dense ranks 80-100 must beat false P1 top100;
- `support_floor_loss`: support cosine and active support cannot drop;
- `fanout/idf_penalty`: penalize high-fanout movement more than rare useful
  dimensions;
- `qrels_loss = 0`.

## Acceptance

Promote only if shared15/native shows:

- dense overlap@100 does not regress versus P1.3;
- dense top100 inside P1 top100 improves, or rank correlation/margin improves
  without overlap loss;
- candidate upper bound does not regress;
- Recall@100 does not regress;
- support cosine and active support pass their floor;
- selected checkpoint is trained, not epoch0 fallback.

## Stop Conditions

Stop this first-stage compiler route if:

- top100 boundary movement still requires dense-overlap loss;
- raw dense score calibration improves but dense top100 membership regresses;
- gains are only qrels metrics with dense-equivalence degradation;
- support metrics cannot be emitted reliably enough to gate checkpoints.

## Decision

M653-A establishes the bottleneck precisely enough to avoid guessing.  The next
valid experiment is M653-B teacher instrumentation plus dense score/rank
distribution preserving compiler training.  It should not include BM25,
reranker, qrels loss, learned gate, or per-dataset tuning.
