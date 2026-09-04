# M673 Native BM25 Rescue Same-Query Evaluation

## Objective

M673 tests whether the M670 BM25 rescue slot-admission rule survives true
native same-query execution.  It uses PostgreSQL native hybrid rows as the
source of fused rank, semantic rank/score, and BM25 rank/score.  It does not
train a model and does not perform Python full-corpus scanning.

The rule remains fixed:

- Preserve fused top `98`.
- Reorder only the tail by native BM25 raw score.
- Accept only if query identity matches and Recall/MAP improve without
  NDCG/MRR regression.

## Implementation

Added `scripts/evaluate_m673_native_bm25_rescue_pg.py`.

The evaluator calls native:

- `ii42_model_query_atoms(...)`
- `ii42_hybrid_bm25_candidates(...)`
- `ii42_hybrid_fuse_candidates(...)`

It extracts the fused hit `source_names`, `raw_values`, `weighted_scores`, and
`ranks`, applies the frozen M670 rule, and writes eval-style artifacts that pass
the M672 same-query contract.

## Smoke And Matrix Results

### Individual Native Rows

| Dataset | Queries | Guard | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `fiqa` smoke10 | 10 | rejected | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `nfcorpus` | 100 | accepted | +0.007908 | +0.000449 | +0.000000 | +0.000000 |
| `webis-touche2020` | 49 | accepted | +0.003823 | +0.000561 | +0.000000 | +0.000000 |
| `cqadupstack` | 100 | accepted | +0.002803 | +0.000154 | +0.000000 | +0.000000 |
| `quora` | 100 | rejected | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

The rejected rows are no-change rows, not harm rows.

### Available4 Same-Query Native Matrix

Artifact:
`runs/m673_native_bm25_rescue_same_query_v1/m673_shared15_available4_matrix.json`

Datasets:
`cqadupstack`, `nfcorpus`, `quora`, `webis-touche2020`

Same-query gate:

- Query-count consistency: `true`
- Query identity checked: `true`
- Query identity consistent: `true`

| Source | Queries | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 349 | 0.876284 | 0.756713 | 0.626559 | 0.698137 | 0.803408 |
| `dense` | 349 | 0.879404 | 0.784005 | 0.681996 | 0.747263 | 0.863784 |
| `P1-a0125` | 349 | 0.914194 | 0.815674 | 0.686432 | 0.753530 | 0.861858 |
| `P1-a0125+M670` | 349 | 0.914194 | 0.819308 | 0.686723 | 0.753530 | 0.861858 |

Macro deltas for `P1-a0125+M670`:

| Comparison | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| vs `P1-a0125` | +0.000000 | +0.003634 | +0.000291 | +0.000000 | +0.000000 |
| vs `BM25` | +0.037910 | +0.062595 | +0.060164 | +0.055393 | +0.058449 |
| vs `dense` | +0.034790 | +0.035303 | +0.004727 | +0.006267 | -0.001926 |

## Interpretation

M673 converts the previous M670 replay signal into a native same-query signal.
This is the first clean evidence in this branch that a deterministic
second-stage rule can improve P1 native ranking without changing candidates or
touching the first-stage encoder.

The signal is still small, but it is useful because:

1. It is same-query comparable against BM25, dense, and P1.
2. It improves Recall@100 and MAP@100 over P1-a0125.
3. It does not regress NDCG@10 or MRR@20 relative to P1-a0125.
4. On the available4 matrix it remains above both BM25 and dense on Recall,
   MAP, NDCG, and CUB.  MRR is slightly below dense by 0.001926.

## Limitations

This is not the full matrix.  The current surface is four datasets, all marked
as seen-regression evidence.  It justifies expansion, not final promotion.

The rule is also intentionally weak: it only reorders rank `99+`.  That is why
top metrics do not move.  The next stage should test whether a slightly wider
but still guarded boundary can improve MAP/MRR without hurting NDCG.

## Next Step

Proceed to M674:

- Expand M673 to all available shared15 native rows.
- Keep M672 same-query identity as a hard gate.
- Sweep conservative `preserve_top_k` values around the safe boundary:
  `95`, `96`, `97`, `98`, `99`.
- Accept only variants with no dataset harm on NDCG/MRR and positive
  macro Recall/MAP.

If the signal vanishes or harms rows outside available4, stop the M670 family
and return to first-stage generated-posting/output-head work.
