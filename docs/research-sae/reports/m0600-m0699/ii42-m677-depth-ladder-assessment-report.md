# M677 Depth-Ladder Assessment

## Scope

M677 tests the under-training hypothesis for the current first-stage line.

It keeps the M675/M676 first-stage scope:

- baseline: `P1.3 / M549U native signed-dot`
- frozen document posting/index geometry
- query-side/output compiler only
- no BM25
- no reranker
- no learned gate
- no qrels-driven loss

The only intended substantive change from M675 seed `6547` is training budget:

- M675: `12` epochs, `108` global steps, selected epoch `5`
- M677: `36` epochs, `324` global steps, selected epoch `32`

Config diff was audited.  Apart from run naming/output paths and the increased
epoch/step budget, M677 matches M675.

## Result Summary

M677 still passes the strict dense-equivalence gate:

- no O@10/O@50/O@100 regression
- no Recall@100 regression
- no CUB regression
- no dense top100 hit loss
- selected trained checkpoint, not epoch0 fallback

However, the deeper run does not improve on M675 seed `6547`.

## Test Split Delta Comparison

The test split contains `671` held-out shared15 queries.

| Metric | M675 seed6547 | M677 depth3x | M677 - M675 |
| --- | ---: | ---: | ---: |
| O@10 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@50 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@100 | `0.000000000` | `0.000000000` | `0.000000000` |
| O@256 | `+0.000101568` | `+0.000042085` | `-0.000059483` |
| Recall@100 | `0.000000000` | `0.000000000` | `0.000000000` |
| MAP@100 | `+0.000371414` | `-0.000001445` | `-0.000372859` |
| NDCG@10 | `+0.000335251` | `-0.000004702` | `-0.000339953` |
| MRR@20 | `+0.000740741` | `0.000000000` | `-0.000740741` |
| support cosine | `-0.000000159` | `-0.000000830` | `-0.000000672` |

## Full Shared15 Delta Comparison

The full split contains all `1342` shared15 qrels queries.

| Metric | M675 seed6547 | M677 depth3x |
| --- | ---: | ---: |
| O@10 | `0.000000000` | `0.000000000` |
| O@50 | `0.000000000` | `0.000000000` |
| O@100 | `0.000000000` | `0.000000000` |
| O@256 | `+0.000084287` | `+0.000075627` |
| Recall@100 | `0.000000000` | `0.000000000` |
| MAP@100 | `+0.000164443` | `-0.000008871` |
| NDCG@10 | `+0.000150863` | `-0.000002727` |
| MRR@20 | `+0.000333333` | `0.000000000` |
| support cosine | `-0.000000163` | `-0.000000827` |

## Interpretation

M677 answers the immediate training-depth question:

The current M675/M676 objective is not simply under-trained at 12 epochs.  A
3x training-budget probe keeps the dense-equivalence gates intact, but the
selected deeper checkpoint gives weaker O@256 and loses the small positive
MAP/NDCG/MRR deltas seen in M675 seed `6547`.

This suggests the current objective is already near its useful movement limit.
The remaining problem is not "run the same setup much longer"; it is the shape
of the supervised movement and checkpoint selection.  The model can safely
move inside the guard, but deeper optimization does not reliably convert that
movement into better dense-equivalent ranking behavior.

## Decision

Do not replace the M676 frozen candidate with M677.

Keep `m675_full_shared15_m674_guard_seed6547` as the current first-stage
candidate baseline.

Next useful work:

1. Keep M675/M676 as the frozen candidate for broader/native or official
   validation.
2. Do not spend a 10x run on the exact same objective before identifying why
   deeper selection turns ranking deltas negative.
3. If training-depth is revisited, use a ladder with explicit epoch-level
   diagnostics and select only checkpoints that preserve both dense gates and
   non-negative full-surface MAP/NDCG/MRR.
4. Continue treating BM25/reranker work as out of scope for this first-stage
   proof.
