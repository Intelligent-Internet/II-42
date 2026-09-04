# M694 Source-Aware Atom Quota Report

## Objective

M693 showed that M691 target atoms exist in teacher-positive documents, but the
mixed atom-candidate ranking truncates them away. M694 tests whether a simple
source-aware quota interface can fix that truncation before launching another
compiler training stage.

This is a mechanism audit, not a deployable model.

## Method

Target rows are the same 29 M691 `rank_safe_recall` rows used by M693. For each
query, M694 reconstructs target atoms and compares quota modes:

- `mixed_current_teacher_top96`: current mixed ranking over current pool plus
  teacher-positive docs, top 96 atoms.
- `quota_current_teacher_48`: top 48 atoms from current pool plus top 48 atoms
  from teacher-positive docs.
- `quota_current_teacher_32`: top 32 atoms from each of those two channels.
- `quota_inference_heads_24`: top 24 atoms from current, BM25-head, P1-head,
  and fused-head channels.
- `quota_bm25_teacher_48`: current plus BM25-high teacher docs.

Teacher-positive channels are oracle diagnostics. They are not deployable unless
replaced by a qrels-free corpus-derived pseudo-positive source.

## Results

| Mode | Rows | Atom visibility | Any-hit rows | Full-hit rows | Visible atoms | Target atoms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `mixed_current_teacher_top96` | 29 | 0.406250 | 29 | 2 | 182 | 448 |
| `quota_current_teacher_48` | 29 | 0.937500 | 29 | 22 | 420 | 448 |
| `quota_current_teacher_32` | 29 | 0.790179 | 29 | 15 | 354 | 448 |
| `quota_inference_heads_24` | 29 | 0.214286 | 26 | 0 | 96 | 448 |
| `quota_bm25_teacher_48` | 29 | 0.243304 | 29 | 0 | 109 | 448 |

## Findings

1. Source-aware quota fixes the truncation mechanism. Moving from mixed top96
   to current+teacher per-source quota raises atom visibility from `0.406250`
   to `0.937500`.

2. This does not yet solve deployment. The successful channel is
   teacher-positive/oracle. Inference-compatible BM25/P1/fused head channels
   expose only `0.214286` of target atoms.

3. BM25-high teacher is not enough. It reaches only `0.243304`, so simply
   leaning harder on high-rank BM25 positives will not recover the missing
   atoms.

4. The next breakthrough condition is now precise: create a qrels-free
   corpus-derived pseudo-positive channel whose atom coverage behaves like the
   teacher-positive channel, then apply source-aware quotas before ranking.

## Decision

Do not train another M692-style compiler over the existing mixed atom pool.

Do not train source-aware quotas using qrels-positive teacher docs as if they
were deployable.

Proceed to M695: build a qrels-free corpus-positive channel and test whether it
can expose the same missing target atoms. Only after that should we train a new
retrieval-constrained generated-posting compiler.

## M695 Direction

M695 should test corpus-positive source construction:

1. Generate pseudo-positive docs from qrels-free signals:
   BM25/entity match, query-document lexical coverage, support-sharing, and
   dense/P1 disagreement surfaces.
2. Compare their atom coverage against M694 teacher-positive quota coverage.
3. Accept the source only if it materially improves atom visibility over current
   pool without using qrels or dataset-specific tuning.
4. Then train a source-aware atom compiler through native shared15 evaluation.
