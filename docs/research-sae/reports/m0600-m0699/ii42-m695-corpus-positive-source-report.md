# M695 Corpus-Positive Source Audit

## Status

M695 is complete and rejected as a reason to start deeper compiler training.

The audit directly tests the current question: whether recent short training
iterations are abandoning a route too early, or whether the route is blocked by
an upstream signal/source issue that longer training would not fix.

The result supports the second explanation. The current qrels-free
corpus-positive sources do not expose enough M691 target atoms, even with the
source-aware quota mechanism that M694 validated.

## Setup

- Script: `scripts/audit_m695_corpus_positive_source.py`
- Full summary:
  `runs/m695_corpus_positive_source_v1/m695_summary.json`
- Full generated report:
  `runs/m695_corpus_positive_source_v1/m695_report.md`
- Smoke summary:
  `runs/m695_corpus_positive_source_smoke_v1/m695_summary.json`
- Surface: native shared15 PostgreSQL path.
- Target rows: M691 `rank_safe_recall` rows.
- Target atoms: atoms reconstructed from M691 strict safe+Recall oracle rows.

M695 uses qrels-derived M691 targets only for audit measurement. The tested
corpus-positive sources themselves are qrels-free:

- corpus support tail;
- corpus composite tail;
- BM25/P1/support mixed corpus quota;
- inference head quota.

The oracle teacher quota is included only as an upper-bound control.

## Result

Full shared15 target atom visibility:

| Mode | Rows | Atom visibility | Any-hit rows | Full-hit rows | Visible atoms | Target atoms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `quota_current_support_48` | 29 | 0.263393 | 29 | 0 | 118 | 448 |
| `quota_current_composite_48` | 29 | 0.261161 | 29 | 0 | 117 | 448 |
| `quota_multi_corpus_24` | 29 | 0.187500 | 25 | 0 | 84 | 448 |
| `quota_inference_heads_24` | 29 | 0.214286 | 26 | 0 | 96 | 448 |
| `quota_oracle_teacher_48` | 29 | 0.937500 | 29 | 22 | 420 | 448 |

The best qrels-free source improves over inference heads by only about
`+0.049` visibility (`0.263393 - 0.214286`). It remains far below the oracle
teacher-positive channel (`0.937500`).

## Interpretation

This is not primarily a training-depth failure.

The current atom proposal/visibility interface still cannot see the atoms that
the successful oracle path needs. Longer training on this source would mostly
learn a better classifier over an incomplete candidate set. That matches the
M692 failure pattern: the recall-bearing target was learnable, but unsafe and
visibility-limited.

The current exploration ratio is therefore acceptable:

1. Small probes are being used to test mechanism feasibility, not to claim final
   model quality.
2. The stop signals are structural: missing atom visibility and sparse
   recall-bearing rows, not just weak optimizer convergence.
3. A route should scale only after the qrels-free source exposes enough target
   atoms to give training a real chance.

## Decision

Do not start a deeper M692-style generated-posting compiler training run over
the current qrels-free corpus-positive sources.

Do not return to traditional SAE reconstruction. The failure is at source
construction / atom visibility, not at reconstruction capacity.

Keep source-aware quotas from M694 as a useful interface, but do not treat the
current qrels-free support/composite sources as sufficient.

## Next Step

M696 should improve pseudo-positive source construction before training:

1. Analyze the gap between `quota_oracle_teacher_48` and
   `quota_current_support_48` at the row/atom level.
2. Identify which target atoms are only visible through oracle teacher docs.
3. Build qrels-free approximations for those docs using corpus-derived signals:
   query term coverage, atom support sharing, P1/BM25 disagreement, document
   atom neighborhoods, and dense/P1 boundary proximity.
4. Re-run the M695 visibility audit.
5. Start training only if qrels-free visibility materially moves toward the
   oracle channel, with a practical target above `0.45` visibility before
   deeper training.

If M696 cannot improve qrels-free visibility beyond this M695 ceiling, the next
design must change the source generator more fundamentally rather than deepen
the compiler.
