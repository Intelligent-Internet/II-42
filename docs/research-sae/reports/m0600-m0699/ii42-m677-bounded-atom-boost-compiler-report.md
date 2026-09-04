# M677 Bounded Atom Boost Compiler

## Purpose

M677 is the first trainable step after the M675/M676 audits.

It tests whether a global, non-oracle atom boost/expansion model can learn from
M675 promoted-positive events and improve held-out native DB rankings. Unlike
M676, M677 does not use positive document atoms at inference time. It only uses
query atoms plus globally learned rules.

This is intentionally small. The goal is to decide whether a shallow global
rule table is worth expanding before training a deeper query-conditioned
compiler.

## Artifacts

- Summary JSON: `runs/m677_bounded_atom_boost_compiler_v1/m677_summary.json`
- Generated markdown: `runs/m677_bounded_atom_boost_compiler_v1/m677_report.md`
- Script: `scripts/train_m677_bounded_atom_boost_compiler.py`

## Method

Training data:

- M675 teacher events.
- Query-level deterministic split: 5 buckets, bucket 0 held out.
- Training positives: promoted-positive document atoms.
- Training negatives: non-relevant displaced documents from M675.

Learned parameters:

- Global per-atom boost scores for existing query atoms.
- Global query-atom to expansion-atom rules.
- No dataset-specific thresholds.
- No qrels or positive-doc atoms at inference time.

Evaluated strengths:

- `0.05`
- `0.10`
- `0.15`

All evaluation uses the native PostgreSQL scorer path.

## Held-Out Result

Held-out event-query surface:

| Source | Queries | Target hit@100 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 27 | 0.000000 | 0.320970 | 0.284515 | 0.686215 | 0.854701 | 1.000000 |
| M674-k95 | 27 | 1.000000 | 0.385546 | 0.288034 | 0.686215 | 0.854701 | 1.000000 |
| M677 best | 27 | 0.000000 | 0.321962 | 0.284542 | 0.687308 | 0.854701 | 0.990253 |

Best M677 strength: `0.05`.

Delta vs baseline:

- Recall@100: `+0.000992`
- MAP@100: `+0.000026`
- NDCG@10: `+0.001093`
- MRR@20: `+0.000000`
- Candidate upper bound: `+0.000058`
- Top95 overlap: `0.990253`

Delta vs M674:

- Recall@100: `-0.063584`
- MAP@100: `-0.003493`
- NDCG@10: `+0.001093`
- MRR@20: `+0.000000`

Training signal size:

- Positive events used for training: `263`
- Non-relevant displaced docs used as negatives: `657`
- Learned boost atoms: `1753`
- Learned expansion rules: `11416`

## Interpretation

M677 is a weak positive but not a promoted route.

Positive:

- Held-out Recall, MAP, and NDCG move slightly above baseline.
- Top95 overlap remains high at `0.990253`.
- The model is not an oracle at inference time, so this proves some retrieval
  signal can be learned from M675-style events.

Negative:

- Target hit@100 remains `0.0`; the learned global rules do not reproduce the
  M674 promoted positives.
- Recall/MAP gains are tiny.
- The model is far below M674 deterministic rescue on Recall and MAP.
- Stronger boost strengths reduce Recall/MAP and disturb the head more.

The core limitation is not implementation detail. A global atom table is too
shallow: it cannot condition on query intent, local candidate shape, or which
boundary positive should be promoted. It mostly acts like a small global
calibration prior.

## Decision

Do not scale this exact global atom boost model.

Keep M677 as evidence that learned query-side posting deltas can move native
scores without immediate collapse, but reject it as the next promoted scorer.

The next stage should not be "larger M677". It should be one of:

1. Query-conditioned compiler: train a small model that consumes query atom
   features and native candidate-set context, then emits bounded atom boosts.
2. Broader teacher construction: add non-M674 under-ranked positives, corpus
   pseudo-positives, entity/term evidence, and dense-floor constraints before
   training.
3. Local candidate-aware rerank-to-delta teacher: learn which atom deltas are
   useful for a specific query boundary instead of a global table.

## Stop Rule

If the next query-conditioned version cannot beat baseline by a material margin
on held-out native event queries while preserving top95/head metrics, stop the
generated-query-delta line and return to teacher construction rather than model
scale.
