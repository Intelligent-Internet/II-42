# SAE M9-M13 Exploration Report

Date: 2026-05-13

## Scope

This phase keeps the SAE/evidence-atom line in research mode. It does not
freeze a product SQL API, mutable index layout, or final scoring policy.

The tested abstraction remains:

```text
BM25 token atoms
+ SAE latent atoms
  -> impact-head candidates
  -> exact doc-row rerank
```

The goal was to push the M9-M13 medium-term plan far enough to answer whether
the current direction should move toward productization or keep exploring.

## M9: Quality/Cost Matrix

The full run covered five datasets:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

It swept:

```text
head_size       = 8, 16, 32
query_budget    = unlimited, 32, 64
selector        = weight, cost, idf_cost
doc_budget      = 32, 64, 128
```

Top mean rows:

| Run | Recall@100 | MRR@20 | NDCG@10 | Candidates | Rerank terms | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `h16_qb0_weight` | 0.7964 | 0.6790 | 0.6030 | 852.8940 | 146,385.0740 | 6.5475 |
| `h32_qb0_weight` | 0.7948 | 0.6790 | 0.6029 | 1,268.1340 | 219,228.2660 | 9.5108 |
| `h32_qb64_weight` | 0.7923 | 0.6732 | 0.5915 | 1,135.3640 | 196,099.2860 | 7.0189 |
| `h16_qb64_idf_cost` | 0.7904 | 0.6633 | 0.5839 | 728.3700 | 124,549.3080 | 4.5624 |
| `h8_qb0_weight` | 0.7856 | 0.6791 | 0.6034 | 516.8160 | 88,089.6020 | 4.0254 |
| `h16_qb32_weight` | 0.7800 | 0.6635 | 0.5838 | 485.3560 | 82,260.0540 | 2.5691 |

Interpretation:

- `head16_qb0_weight` remains the best overall Recall@100 point.
- `head32` does not improve enough to justify its much higher candidate and
  rerank cost.
- `head8` is a strong low-cost point: it cuts candidate docs by about 39% and
  rerank terms by about 40%, while Recall@100 drops about 0.0108.
- Query-budget selectors are useful cost levers, but they currently lose too
  much first-page quality to replace the unlimited query path.

## M10: Selectivity Frontier

The strict frontier rule was:

```text
lowest candidate count within 0.005 Recall@100 of the best M9 row
```

Result:

```text
best_recall = h16_qb0_weight
best_cost_within_0.005_recall = h16_qb0_weight
```

This is an important negative result. The current simple query-budget and
DF/IDF-cost selectors do not improve the frontier under a tight recall
tolerance. The next selectivity work should not keep tuning these selectors in
place. It should test better atom reliability and candidate-only expansion
routes:

```text
head16 quality
+ head8-like cost
```

This points to:

- atom reliability learned from qrel or dense-teacher lift;
- token-latent graph expansion used only for candidate widening;
- mixed atoms as candidate boosters only;
- query-conditioned atom gating instead of global budget heuristics.

## M11: Rerank-Cost Compression

M11 kept candidate generation fixed at:

```text
head_size = 16
query_budget = unlimited
```

Then it compressed doc-row vectors before exact rerank.

| Run | Recall@100 | MRR@20 | NDCG@10 | Candidates | Rerank terms | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `h16_qball_doc128` | 0.7937 | 0.6840 | 0.6104 | 852.8940 | 106,998.4100 | 6.2760 |
| `h16_qball_doc64` | 0.7849 | 0.6466 | 0.5760 | 852.8940 | 54,585.2160 | 5.8160 |
| `h16_qball_doc32` | 0.7758 | 0.6300 | 0.5589 | 852.8940 | 27,292.6080 | 5.5816 |

Interpretation:

- `doc128` is promising. It reduces rerank terms by about 27% versus the full
  doc-row path, keeps Recall@100 within about 0.0027, and improves MRR/NDCG in
  this matrix.
- `doc64` and `doc32` are too aggressive as final scoring paths.
- The next rerank layout should prioritize a top-128 style compact doc vector,
  then verify whether the MRR/NDCG improvement is stable or only benchmark
  noise.

## M12: Real Workload Readiness

The artifact scan inspected 5,000 files and found 100 broad matches, but it did
not find canonical arxiv/pubmed/commons query plus qrel pairs:

```text
canonical_ready = false
canonical_qrel_candidates = 0
canonical_query_candidates = 0
```

This blocks productization. Benchmark matrices are useful, but they are not
enough for our RAG and memory-retrieval workload.

The next data milestone should create a stable real-workload validation set:

- arxiv query set with known relevant documents or dense-teacher proxy qrels;
- pubmed query set with known relevant documents or citation/MeSH-derived
  proxy qrels;
- commons/policy query set only if its workflow is representative enough;
- exact metrics matching the current matrix: Recall@20/100, MRR@20, NDCG@10,
  MAP@100, candidate_docs, rerank_doc_terms, and PG latency.

## M13: Gate

Gate result:

```json
{
  "candidate_cost_path_ok": true,
  "doc64_rerank_compression_open": true,
  "pg_synthetic_latency_ok": true,
  "productization_ready": false,
  "quality_ok": true,
  "real_workload_artifacts_present": false,
  "recommendation": "continue exploration; do not freeze SQL/API or mutable index yet"
}
```

Productization is intentionally blocked. The direction is technically
promising, but the best near-term work is still exploration:

```text
selectivity frontier
+ doc128 compact rerank
+ real workload qrels
```

## Medium-Term Plan

The detailed active roadmap is now maintained in:

```text
sae-converged-research-and-engineering-plan.md
```

The plan below is the condensed version.

### M14: Selectivity Frontier 2

Goal:

```text
match h16_qb0 Recall@100/MRR
with candidate_docs closer to h8_qb0
```

Work:

- Train or estimate atom reliability using qrel/dense-teacher lift.
- Use token-latent graph expansion only as candidate widening.
- Keep mixed atoms candidate-only.
- Compare against `h16_qb0_weight`, `h8_qb0_weight`, and `h16_qb64_weight`.

Gate:

```text
Recall@100 >= 0.7930
MRR@20     >= 0.6750
candidate_docs <= 650
rerank_doc_terms <= 110,000
```

### M15: Compact Doc-Row Rerank

Goal:

```text
keep doc128 quality
and make it a real EATMH payload/layout option
```

Work:

- Add a compact-doc-vector payload variant.
- Benchmark doc128 against full doc rows in C and PostgreSQL.
- Confirm MRR/NDCG improvement across repeated runs.

Gate:

```text
Recall@100 drop <= 0.005
rerank_doc_terms reduction >= 20%
C and PostgreSQL parity preserved
```

### M16: Real Workload Matrix

Goal:

```text
make arxiv/pubmed/commons validation first-class
```

Work:

- Build or import query/qrel/proxy-qrel artifacts.
- Reuse the same matrix runner on real workload data.
- Do not move to product API until this exists.

Gate:

```text
canonical_ready = true
real workload direction agrees with five-dataset matrix
```

### M17: SQL Model Draft

Only after M14-M16:

- Draft SQL/read-only generation API.
- Define payload versioning for compact doc rows.
- Define diagnostics required by the planner/API layer.
- Still defer mutable maintenance until the read-only model is stable.

Explicitly do not start:

- mutable index maintenance;
- public API stabilization;
- final-score mixed atoms;
- dense fallback removal.
