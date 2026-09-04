# M655 First-Stage Evidence Roadmap

Status: `planning`

This roadmap freezes the current evidence before the next training probe.  The
goal is to avoid loss-search by intuition.  Every next experiment must map to a
measured first-stage blocker.

## Scope

This is first-stage only:

- no BM25 training or selection;
- no reranker;
- no learned fusion gate;
- no qrels-driven objective;
- frozen document posting/index geometry unless a later isolated proof shows
  query-side-only is insufficient.

The target is dense-faithful unified posting generation.  A checkpoint is useful
only if dense-equivalence gates pass before retrieval metrics are considered.

## Current Evidence

### Preserved Baseline

`P1.3 / M549U native signed-dot` remains the frozen baseline.

### Negative Results

M653F showed that adding a dense top256 tail-listwise auxiliary loss is not
enough.  Weak tail loss produced no material movement, and stronger KL harmed
overlap.

M654R showed that the original root-only compiler is the wrong transfer shape
for the ridge teacher.  Loss can decrease while dense overlap moves in the
wrong direction.

M654R also showed that the all-query canary split is too small to certify
tail preservation.  Active-locked P1-support adapters can improve O@100,
MAP, NDCG, and MRR, but held-out O@256 still regresses.

### Positive Results

M653G is the key positive proof.  A per-query ridge oracle inside frozen P1
document postings and original query support improves dense overlap and
Recall.  Therefore the current problem is not mathematically impossible.

M654R adds a second positive signal: a trainable P1-support residual adapter can
learn useful ranking movement from ridge teachers.  The unresolved part is
robust transfer of O@256 tail preservation, not whether query-side movement is
possible.

### Later Retrieval-Expansion Evidence

M690-M694 are important, but they are not first-stage evidence.  They address
retrieval-constrained generated atom proposals after dense-root posting already
exists.

Those reports narrow the second-stage blocker:

- M690 showed the current atom proposal surface has far fewer strict
  safe+Recall rows than the retrieval teacher.
- M691 showed retrieval-teacher proposal expansion can raise strict
  safe+Recall rows from `5` to `29`, but it is an oracle/proposal audit rather
  than a deployable model.
- M692 showed the recall-bearing atom target is learnable but not safely
  deployable with the current atom source.
- M693 showed missing target atoms exist in teacher-positive documents but are
  suppressed by mixed atom-candidate ranking.
- M694 showed source-aware quotas fix that truncation if a teacher-positive
  channel is available, but inference-compatible BM25/P1/fused channels do not
  yet expose enough target atoms.

This supports a later M695 direction, but M695 belongs to retrieval-expanded
generated-posting work.  It must not be used to select or train the current
first-stage dense-equivalence compiler.

## Paper Signals

Recent learned sparse retrieval work supports the same boundary:

- SAE-SPLADE shows SAE-style concept spaces can be useful for learned sparse
  retrieval, but also notes that SAE objectives are not explicitly aligned with
  IR.  This supports reusing SAE/index ideas, not returning to reconstruction
  SAE loss as the main objective.
- CSPLADE and related LLM-based LSR work emphasize staged adaptation and
  instability control for large sparse retrievers.  This supports separating a
  stable dense-faithfulness stage from later retrieval optimization.
- DREAM shows that a frozen judge and candidate competition can provide useful
  retrieval supervision, but that signal is a second-stage or teacher-expansion
  idea.  It should not be mixed into the first-stage dense-equivalence gate.
- Efficient dense retriever pruning work suggests preserving retrieval-specific
  structure is not the same as generic compression.  This supports output-head
  or posting-compiler transfer from the dense root rather than training a small
  unrelated encoder from scratch.

Primary references:

- https://arxiv.org/html/2604.21511v1
- https://aclanthology.org/2025.ijcnlp-long.7.pdf
- https://arxiv.org/html/2606.24667v1
- https://arxiv.org/html/2512.20612v1

## Diagnosed Bottleneck

The current blocker is:

> query-side trainable compiler can move useful rankings, but cannot yet
> preserve dense top256 tail membership on held-out queries.

This is more precise than "loss is wrong" or "model is too small".

The immediate hypotheses are:

1. The teacher corpus is too small.  Per-query ridge teachers are stable, but
   320 train queries do not cover enough boundary shapes.
2. The gate is partly mismatched across surfaces.  Absolute support cosine
   floors can be impossible when the baseline itself is below the floor.
3. Checkpoint selection should prioritize O@256 before MAP/NDCG/MRR.
4. The adapter shape is closer than the root-only compiler, so the next probe
   should scale this architecture before changing architecture again.

## Next Probe: M654S

M654S should be a scaled ridge-teacher transfer probe.

Required design:

1. Keep `p1_support_residual + active-lock`.
2. Build ridge teachers for all available shared15 queries, not only the small
   M653-D row split.
3. Use relative support gates:
   - active support must not regress versus P1;
   - support cosine must not regress versus P1 on the same eval surface.
4. Select checkpoints with this priority:
   - O@256 non-regression;
   - O@100 non-regression;
   - CUB non-regression;
   - Recall@100 non-regression;
   - then MAP/NDCG/MRR improvement.
5. Do not run full shared15 replay until canary held-out test passes all dense
   gates.

## Stop Conditions

Stop this branch if scaled M654S still cannot preserve O@256 while moving O@100
or MAP.  That would mean query-side-only transfer is insufficient and the next
isolated proof should test whether a frozen-doc constraint is too strict.

Do not stop because MAP/NDCG improvements are small.  In this stage, passing
dense-equivalence gates is the primary signal.

Do not promote any checkpoint that trades O@256 or support safety for visible
ranking gains.

## Decision

The next useful action is not another speculative loss.  It is a larger,
relative-gated M654S transfer experiment that tests whether the M653G oracle can
be learned by a global query-side compiler when the teacher corpus is large
enough and checkpoint selection is aligned with the actual blocker.

M690-M694 should remain parked as second-stage evidence until the first-stage
dense-equivalence question is answered.  Mixing their qrels/retrieval-expansion
signals into M654S would make the result impossible to interpret.
