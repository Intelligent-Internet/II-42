# M679 Candidate-Aware Expansion Selector

## Purpose

M679 tests whether the retrieval-constrained generated-posting route can move
past hand-written BM25 tail expansion.

The prior M678 result showed that query-local BM25 tail context is useful, but
the policy selected tail candidates by BM25 rank only. M679 adds a small global
selector over native candidate features, trained on a query-level split of M675
promoted-positive events.

This is not a final scorer. It is a leakage-guarded check of whether candidate
selection can be learned without reading qrels or target positives at inference
time.

## Setup

- Teacher/eval surface: M675 promoted-positive event queries.
- Split: deterministic query-level holdout, bucket `0` of `5`.
- Train rows: BM25 tail candidates outside the preserved top95 head.
- Labels: candidate doc is a promoted-positive event on training queries.
- Inference: select tail candidate docs by learned selector, read their native
  posting atoms, and apply the M678 bounded query expansion.
- Native scorer: PostgreSQL unified posting path.

Feature set:

- BM25 rank and score.
- P1/fused rank and score.
- shared query/document atom support.
- selected document head missing/shared atom signals.
- source count.

## Held-Out Result

| Source | Queries | Target hit@100 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 27 | 0.000000 | 0.320970 | 0.284515 | 0.686215 | 0.854701 | 1.000000 |
| M674 oracle tail reorder | 27 | 1.000000 | 0.385546 | 0.288034 | 0.686215 | 0.854701 | 1.000000 |
| M678 BM25 top-k reference | 27 | 0.092593 | 0.321856 | 0.284967 | 0.686984 | 0.855556 | 0.969201 |
| M679 learned selector | 27 | 0.116049 | 0.324164 | 0.285272 | 0.688237 | 0.855556 | 0.966472 |

M679 vs M678 BM25 top-k reference:

- Target hit@100: `+0.023456`
- Recall@100: `+0.002308`
- MAP@100: `+0.000305`
- NDCG@10: `+0.001253`
- MRR@20: unchanged
- Top95 overlap: `-0.002729`

M679 vs baseline:

- Recall@100: `+0.003194`
- MAP@100: `+0.000757`
- NDCG@10: `+0.002022`
- MRR@20: `+0.000855`

M679 vs M674 remains far behind:

- Target hit@100: `0.116049` vs `1.000000`
- Recall@100: `0.324164` vs `0.385546`
- MAP@100: `0.285272` vs `0.288034`

## Train Signal

- Candidate rows: `2180`
- Positive rows: `263`
- Positive share: `0.120642`

Top learned weights:

- `bm25_rank_recip`: `1.191620`
- `bm25_score_log`: `0.717412`
- `p1_score_log`: `0.550015`
- `shared_atom_count`: `0.514852`
- `shared_doc_abs_impact`: `0.508556`
- `shared_query_abs_impact`: `0.501972`
- `top_head_missing`: `-0.456229`
- `top_head_shared`: `0.456229`

The selector did not just learn target membership. It selected fewer explicit
target-positive candidates than the BM25 reference, but produced better native
ranking metrics after expansion. That suggests the useful signal is the
candidate atom geometry, not only whether the selected candidate is the exact
promoted positive.

## Interpretation

The review direction is useful and matches the observed bottleneck:

- Traditional SAE reconstruction is still not the main route.
- Dense-only mimic is already at the boundary.
- Retrieval-constrained posting deltas are the right next proof surface.
- Candidate competition and native index evaluation must stay in the loop.

M679 adds one concrete positive signal:

> A learned query-local tail-candidate selector can beat BM25 top-k candidate
> selection on held-out event queries.

But the signal is still small. M679 should not be promoted as a main scorer or
reported as a full-route improvement. It only clears the narrow criterion that
candidate-aware selector learning is not dead.

## Decision

Preserve M679 as a positive intermediate result.

Do not scale it blindly. The next stage must answer a stricter question:

> Does the learned selector improve the full shared15 native matrix, not just
> the M675 event-query slice?

If the full-query matrix regresses, the route needs broader teacher
construction before more model depth.

## Next Step

M680 should run the selector through a full-query native shared15 regression:

1. Apply M679 expansion to all shared15 queries, not only event queries.
2. Keep P1-a0125 baseline and M678 BM25-reference rows.
3. Report per-dataset and macro Recall@100, MAP@100, NDCG@10, MRR@20,
   candidate upper bound, and top95 overlap.
4. Preserve the query-level split discipline; do not train on eval event facts.
5. Reject if gains are isolated to event queries or if head/top95 preservation
   drops materially.

Only if M680 improves the full native matrix should the next stage become a
deeper retrieval-constrained generated-posting compiler.
