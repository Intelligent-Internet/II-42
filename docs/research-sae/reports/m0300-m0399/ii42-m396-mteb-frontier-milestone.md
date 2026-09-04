# II-42 M396 MTEB Frontier Milestone

## Status

This is the latest closed milestone for the dense-tail posting route as of
2026-06-26.

The milestone closes the M392 -> M396 line:

- M392: runtime-shaped dense-tail + BM25 posting route on BEIR15
- M393: first MTEB 10-task validation
- M394/M395: three-seed robustness and qrels-free gate diagnostics
- M396: high-alpha frontier sweep

## Current Best

| Item | Value |
| --- | --- |
| Best source | `m394_bm25_tail_union_zblend_a018` |
| Eval surface | 10-task MTEB English retrieval |
| Mean NDCG@10 | 0.58804 |
| Seed std | 0.00099 |
| Seeds | `393`, `1393`, `2393` |
| Delta vs same-surface dense+BM25 | +0.00529 |
| Delta vs same-surface dense | +0.01541 |

## Interpretation

The result is a real but modest improvement over same-surface dense+BM25. It is
not a dataset-specific policy and it does not use qrels to construct the
retrieval route.

The alpha frontier is now understood:

- useful global band: `0.15-0.20`
- best tested point: `0.18`
- `0.25` and `0.30` degrade clearly

Therefore, the next research lever is no longer more alpha sweeping. The next
valuable direction is a search-optimized posting encoder that directly learns
the dense-tail posting shape under dense-teacher supervision and indexability
constraints.

## Committed Artifacts

- `scripts/research_sae_m393_mteb_tail_bm25_eval.py`
- `scripts/research_sae_m394_m395_mteb_aggregate.py`
- `scripts/run_m394_m395_mteb_full_spark.sh`
- `docs/research-sae/reports/m0300-m0399/ii42-m393-mteb-tail-bm25-eval-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m394-m395-mteb-robustness-report.md`
- `docs/research-sae/reports/m0300-m0399/ii42-m396-mteb-frontier-milestone.md`

## Next Research Line

Start M397 as a Dense-Distilled Posting Encoder canary:

- first stage: dense embedding to signed sparse postings
- later stage: text to signed sparse postings
- train without qrels
- distill dense teacher ranking and dense-neighborhood preservation
- enforce posting budget, document-frequency balance, and indexability
- test BM25-free and BM25-aware variants separately
