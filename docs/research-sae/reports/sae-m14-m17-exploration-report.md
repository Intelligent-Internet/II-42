# SAE M14-M17 Exploration Report

Date: 2026-05-13

Current status has been consolidated in:

```text
docs/research-sae/reports/milestones/sae-current-status-and-engineering-readiness-report.md
```

Use that report as the current readiness snapshot. This file records the
M14-M17 exploration evidence.

## Scope

This phase tested the converged evidence-atom route before SQL/API
productization.

The active abstraction remains:

```text
BM25 token atoms
+ SAE latent atoms
  -> impact-head candidates
  -> exact doc-row rerank
```

M14-M17 asked four questions:

1. Can selectivity be improved with reliability or candidate-only expansion?
2. Can compact doc-row rerank become the next engineering path?
3. Do real arxiv/pubmed/commons validation artifacts exist?
4. Is the SQL model ready to draft?

## M14: Selectivity Frontier 2

Runs:

| Run | Recall@100 | MRR@20 | NDCG@10 | Candidates | Rerank terms | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `h16_base_full` | 0.7964 | 0.6790 | 0.6030 | 852.8940 | 146,385.0740 | 6.7631 |
| `h8_graph_candidate` | 0.7951 | 0.6789 | 0.6029 | 1,212.4680 | 211,860.5960 | 9.7916 |
| `h16_doc128` | 0.7937 | 0.6840 | 0.6104 | 852.8940 | 106,998.4100 | 6.3904 |
| `h8_mixed_candidate` | 0.7858 | 0.6791 | 0.6034 | 517.3120 | 88,162.8640 | 4.2069 |
| `h8_base_full` | 0.7856 | 0.6791 | 0.6034 | 516.8160 | 88,089.6020 | 4.2418 |
| `h16_qrel_rel_qb64` | 0.7827 | 0.6629 | 0.5830 | 730.7120 | 125,178.8080 | 4.7923 |
| `h16_dense_rel_qb64` | 0.7824 | 0.6543 | 0.5735 | 727.5920 | 124,574.1080 | 4.7287 |

Gate:

```json
{
  "passed": false,
  "selected": "h16_base_full",
  "target_recall": 0.792994671580683,
  "target_mrr": 0.6750125826787591
}
```

Interpretation:

- The current qrel/dense reliability selectors did not preserve enough quality.
- `h8_graph_candidate` nearly preserves quality but increases cost too much.
- `h8_mixed_candidate` behaves almost the same as `h8_base_full`; mixed atoms
  remain safe as candidate-only, but not yet valuable enough.
- M14 should not block the next engineering step. The selectivity problem is
  still open, but the current round did not find the breakthrough.

## M15: Compact Doc-Row Rerank

Runs:

| Run | Recall@100 | MRR@20 | NDCG@10 | Candidates | Rerank terms | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `h16_doc160` | 0.7968 | 0.6891 | 0.6117 | 852.8940 | 126,629.1360 | 6.6549 |
| `h16_doc128` | 0.7937 | 0.6840 | 0.6104 | 852.8940 | 106,998.4100 | 6.4705 |
| `h16_doc96` | 0.7919 | 0.6760 | 0.6013 | 852.8940 | 81,717.6840 | 5.9854 |

Gate:

```json
{
  "passed": true,
  "candidate": "h16_doc128",
  "recall_drop": 0.002670517384963822,
  "mrr_delta": 0.004968068043068152,
  "rerank_term_reduction": 0.2690620219927613
}
```

Interpretation:

- `doc128` passes the compact-rerank gate.
- It cuts rerank terms by about 27%.
- It keeps Recall@100 within the allowed band.
- It improves MRR@20 and NDCG@10 in this matrix.

This is now the primary engineering path.

## M16: Real Workload Readiness

The artifact scan still did not find canonical real-workload query/qrel pairs:

```text
canonical_ready = false
canonical_qrel_candidates = 0
canonical_query_candidates = 0
matched_artifact_count = 100
```

This keeps product-quality claims blocked. The benchmark matrix is useful, but
it is not enough for arxiv/pubmed/commons RAG quality claims.

The next real-workload step should therefore be split:

- M20 efficiency: run real arxiv/pubmed/commons corpora to measure latency,
  fanout, payload size, memory, cache behavior, and result-shape sanity.
- M20b quality: only report Recall/MRR/NDCG/MAP after qrels or defensible
  proxy-qrels exist.

## M17: SQL Model Gate

Gate:

```json
{
  "compact_rerank_passed": true,
  "selectivity_frontier_passed": false,
  "real_workload_ready": false,
  "ready_for_sql_model_draft": false,
  "draft_status": "blocked; keep read-only experimental functions"
}
```

Interpretation:

- Stable SQL/API productization and quality claims remain blocked.
- Mutable native index design remains blocked.
- A read-only compact payload prototype is now justified.

## Updated Route

The route should now narrow again:

```text
M18: EATMH002 compact doc-row payload
M19: C and PostgreSQL read-only doc128 parity/benchmark
M20: real workload efficiency matrix, no formal quality claim
M20b: future real workload qrel/proxy-qrel construction
M21: SQL model draft after M19 + M20 efficiency pass
```

M14-style selectivity exploration should continue as a side research track, but
it is not the next engineering blocker.

## Next Engineering Hypothesis

The next concrete hypothesis is:

```text
Top-128 compact doc rows preserve the evidence-atom quality frontier while
reducing exact rerank cost enough to justify a new read-only payload version.
```

The proposed payload direction is:

```text
EATMH001 = full doc-row reference
EATMH002 = compact top-128 doc-row candidate
```

The next report should validate:

- binary payload shape;
- Python parity against current doc128 simulation;
- standalone C reader parity;
- PostgreSQL by-id cached query parity;
- repeated five-dataset benchmark;
- memory and latency delta versus EATMH001.
