# M680 Candidate Selector Full Shared15 Regression

## Purpose

M680 tests whether the M679 candidate-aware expansion selector survives outside
the M675 event-query slice.

M679 was a positive intermediate result: on held-out event queries, a learned
selector beat the M678 BM25 top-k reference. That was not enough to promote the
route. M680 applies the same selector and bounded query expansion to the full
shared15 qrels query surface through the native PostgreSQL unified posting
scorer.

This is the correct gate for the recent review direction:

- keep the direct/unified posting route;
- do not return to traditional SAE reconstruction;
- do not scale event-slice success without full native regression;
- check whether retrieval-constrained generated posting actually improves
  full-matrix Recall/MAP.

## Artifacts

- Summary JSON: `runs/m680_candidate_selector_full_shared15_v1/m680_summary.json`
- Generated markdown: `runs/m680_candidate_selector_full_shared15_v1/m680_report.md`
- Script: `scripts/evaluate_m680_candidate_selector_full_shared15.py`
- Tests: `tests/test_evaluate_m680_candidate_selector_full_shared15.py`

## Setup

- Surface: native shared15 qrels queries.
- Query count: `1342`.
- Training labels: M675 promoted-positive events on train split only.
- Inference: no qrels, no target-positive IDs.
- Baseline: P1-a0125 native hybrid ranking.
- Comparator: M674 top95-preserving deterministic BM25 tail reorder.
- Reference expansion: M678 BM25 top-k tail candidate atom expansion.
- Candidate model: M679 learned candidate selector + bounded atom expansion.

M680 completed in `58.65` seconds locally.

## Macro Result

| Source | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Candidate UB | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 0.864763 | 0.667856 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| M674 oracle tail reorder | 0.870086 | 0.668196 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| M678 BM25 top-k reference | 0.866056 | 0.666991 | 0.736871 | 0.820334 | 0.946935 | 0.966703 |
| M679 learned selector | 0.865162 | 0.668266 | 0.737594 | 0.821832 | 0.946167 | 0.959346 |

## Key Deltas

M679 selector vs baseline:

- Recall@100: `+0.000399`
- MAP@100: `+0.000410`
- NDCG@10: `+0.000308`
- MRR@20: `+0.000954`
- Candidate UB: `-0.000938`
- Top95 overlap: `0.959346`

M679 selector vs M678 BM25 top-k reference:

- Recall@100: `-0.000895`
- MAP@100: `+0.001275`
- NDCG@10: `+0.000723`
- MRR@20: `+0.001499`
- Candidate UB: `-0.000767`
- Top95 overlap: `-0.007357`

M679 selector vs M674:

- Recall@100: `-0.004924`
- MAP@100: `+0.000069`
- NDCG@10: `+0.000308`
- MRR@20: `+0.000954`
- Candidate UB: `-0.000938`

## Interpretation

M680 rejects the simple promotion of M679.

The selector does not produce the desired full-query recall breakthrough. It
slightly improves MAP/NDCG/MRR, but it loses candidate upper bound and fails to
beat the simpler M678 BM25 top-k reference on Recall@100. This means the current
selector is doing rank polishing and limited head/tail perturbation, not robust
retrieval expansion.

This does not invalidate the review direction. It sharpens it:

1. M675/M676/M678/M679 show that generated posting deltas can move native
   rankings.
2. M680 shows that an M675-only selector teacher is too narrow for full-query
   Recall.
3. The current blocker is teacher coverage and full-query generalization, not
   merely selector model depth.

The main evidence is label sparsity and coverage:

- M675 produced only `340` promoted-positive events.
- Several datasets had zero or near-zero M675 teacher events.
- M679 trained on only `263` positive candidate rows.
- Full shared15 has `1342` qrels queries, so the selector is learning from a
  narrow boundary-event subset.

## Decision

Do not promote M679 as a scorer or compiler.

Do not deepen this exact selector yet. Scaling a weak, sparse M675-only teacher
is likely to overfit the event surface and keep polishing ranking metrics
without improving Recall/CUB.

Preserve M679/M680 as evidence for the correct next move:

> broaden the retrieval-constrained teacher before increasing model depth.

## Next Breakthrough Candidate

M681 should build a broader generated-posting teacher, then rerun native
full-query validation.

Required teacher sources:

1. M675 support-safe promoted positives.
2. qrels positives present in native top1000 but missing top100.
3. BM25 high-rank positives and entity/term coverage positives.
4. Dense-hit positives that must remain protected as a floor.

Required gates:

1. No dataset-specific thresholds.
2. Full shared15 native Recall@100 and MAP@100 must improve together.
3. Candidate UB must not drop.
4. Top95/head preservation must remain above an explicit floor.
5. Event-query gains are not enough; full-query matrix is the promotion gate.

This keeps the main route aligned with the review:

- not traditional SAE reconstruction;
- not dense-only mimic;
- not fixed alpha tuning;
- not event-slice overfitting;
- yes to dense-root unified posting plus retrieval-constrained generated
  posting.
