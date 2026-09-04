# M693 Retrieval Teacher Atom Source Coverage Report

## Objective

M692 found a learnable recall-bearing atom signal, but the generated query
updates were not support-safe. The main suspected blocker was target atom
visibility: the current inference atom proposal pool exposed only part of the
teacher target.

M693 audits where the missing target atoms can be seen before launching another
training stage.

## Method

Input target rows are the 29 M691 `rank_safe_recall` rows: M691 rank-safe oracle
updates that improved Recall@100 while preserving the top95 head.

For each target query, M693 reconstructs the M691 target atoms and compares
visibility under these atom-candidate sources:

- `current_tail`: current M686/M692 native tail proposal pool.
- `current_plus_bm25_head`: current pool plus top BM25 documents.
- `current_plus_p1_head`: current pool plus top P1 documents.
- `current_plus_fused_head`: current pool plus current fused head documents.
- `current_plus_bm25_teacher`: current pool plus M681 BM25-high teacher docs.
- `current_plus_teacher_positive`: current pool plus all selected teacher
  positive docs.
- `teacher_positive_only`: selected teacher positive docs only.

This is a visibility audit, not a deployable model. Teacher-positive-only is an
oracle upper bound used to localize the bottleneck.

## Aggregate Coverage

| Source | Rows | Atom visibility | Any-hit rows | Full-hit rows | Visible atoms | Target atoms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `current_tail` | 29 | 0.401786 | 29 | 2 | 180 | 448 |
| `current_plus_bm25_head` | 29 | 0.359375 | 29 | 2 | 161 | 448 |
| `current_plus_p1_head` | 29 | 0.359375 | 29 | 1 | 161 | 448 |
| `current_plus_fused_head` | 29 | 0.381696 | 29 | 1 | 171 | 448 |
| `current_plus_bm25_teacher` | 29 | 0.401786 | 29 | 2 | 180 | 448 |
| `current_plus_teacher_positive` | 29 | 0.406250 | 29 | 2 | 182 | 448 |
| `teacher_positive_only` | 29 | 1.000000 | 29 | 29 | 448 | 448 |

## Interpretation

The important finding is not simply that current visibility is low. It is that
teacher-positive-only recovers all target atoms, while mixing those same
teacher-positive docs into the current pool recovers almost none of the missing
atoms.

This means the bottleneck is not just document source coverage. The target atoms
exist in the teacher documents, but the current atom-candidate ranking and
truncation scheme suppresses them when they compete with the native tail pool.

BM25/P1/fused head expansion does not solve it:

- BM25 head lowers visibility from `0.401786` to `0.359375`.
- P1 head also lowers visibility to `0.359375`.
- Fused head reaches only `0.381696`.
- Adding all teacher-positive docs reaches only `0.406250`.

So "add more documents to the same mixed atom candidate pool" is not a real
breakthrough path. The candidate atom admission interface is wrong.

## Decision

Do not proceed with a larger M692-style atom classifier over the same mixed atom
candidate ranking. It will keep trading Recall against top95/CUB because the
source interface hides most recall-bearing target atoms.

Do not return to traditional SAE reconstruction. The failure mode is at the
retrieval atom admission boundary, not reconstruction capacity.

Promote a new M694 direction: source-aware atom admission. The next compiler
should allocate atom-candidate quotas by source or teacher channel before
ranking, so teacher/corpus-derived atoms are not immediately drowned by native
tail vote mass.

## Next Step

M694 should test source-aware atom admission before deeper model training:

1. Build atom candidate rows with per-source quotas:
   current tail, BM25 head, P1/fused head, teacher/corpus-positive channel.
2. Preserve source identity as features.
3. Train/evaluate the recall-bearing atom compiler with the same native
   shared15 path.
4. Accept only if top95/CUB floor is preserved and Recall/MAP improves on
   holdout.
5. Stop if source-aware admission still cannot make target atoms visible under
   inference-compatible sources.
