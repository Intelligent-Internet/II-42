# SAE Milestone 8 PostgreSQL Workspace And Exploration Report

Date: 2026-05-13

## Purpose

Milestone 7 removed repeated by-id generation bytea fetch/decode from the
PostgreSQL path. Milestone 8 tests the next bottleneck without committing to a
product API or mutable index design.

The immediate systems question is:

```text
Once EATMH001 is parsed and backend-local, what still scales with every query?
```

The current answer is:

```text
query-local working set
+ candidate fanout
+ exact doc-row rerank scan
```

## Implementation

The evidence-atom read-only query path now has an adaptive execution strategy.

Small payloads keep the original palloc path:

```text
candidate_seen[doc_count]
candidate_docs[doc_count]
scores[doc_count]
scored_seen[doc_count]
query_weights[max_atom_id + 1]
```

This path is still best for the current 2k-document BEIR-style research
payloads because full zeroing is cheap and the mark-based workspace adds branch
cost.

Larger payloads use a backend-local reusable workspace:

```text
candidate_marks[doc_count]
candidate_docs[doc_count]
scored_docs[doc_count]
scores[doc_count]
query_weight_marks[max_atom_id + 1]
query_weight_dims[touched_query_atoms]
query_weights[max_atom_id + 1]
```

The workspace uses an epoch mark instead of clearing full arrays. Query weights
are reset through a touched-dimension list. Ranking collects from `scored_docs`
instead of scanning all document ordinals again.

The current switch point is intentionally conservative:

```text
SAE_EVIDENCE_WORKSPACE_MIN_DOCS = 8192
```

This is an exploration threshold, not a final product default.

## Benchmark

Runner:

```text
scripts/research_sae_milestone8_pg_exploration.py
```

It reuses the Milestone 7 PostgreSQL benchmark and adds synthetic document-row
expansion. Expansion is not a quality benchmark. It isolates doc-count-scaled
resident query working-set cost.

Configuration:

```text
datasets              = scifact, fiqa
doc_expansion_factors = 1, 5
queries               = 30
head_size             = 16
```

Result:

| Dataset | Expansion | Docs | Payload MB | Cached ms | Candidates | Rerank doc terms | Cold->cached | Direct->cached |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 1 | 2,000 | 4.3752 | 0.5184 | 788.9000 | 146,487.0667 | 2.9542x | 5.5546x |
| `scifact` | 5 | 10,000 | 17.4587 | 0.7341 | 1,057.2333 | 195,137.1333 | 6.2574x | 11.6811x |
| `fiqa` | 1 | 2,000 | 3.3983 | 0.5077 | 755.3000 | 113,341.6000 | 2.5974x | 4.5819x |
| `fiqa` | 5 | 10,000 | 13.8457 | 0.6557 | 1,018.5000 | 152,530.9667 | 6.0999x | 9.7723x |

All rows kept exact direct/cold/cached parity.

## Interpretation

The adaptive split is necessary. For small payloads, the original palloc path
is still the right choice. For larger payloads, generation caching plus the
workspace keeps cached by-id latency below 1 ms in this synthetic 10k-doc
stress shape while cold decode and direct bytea scale much worse.

The result also shows that the next breakthrough is not another cache. The
dominant counters are now:

```text
candidate_docs
rerank_doc_terms
```

For the 10k-doc synthetic rows, cached query time is still controlled because
impact-head candidate generation only opens about 1k documents. If a real
larger corpus produces much higher candidate_docs or much wider doc vectors,
workspace reuse will not be enough.

## Selectivity Sweep

The same runner can sweep `head_size`. A SciFact 30-query smoke shows the
expected cost frontier:

| Head size | Cached ms | Candidates | Rerank doc terms |
| ---: | ---: | ---: | ---: |
| 8 | 0.4712 | 463.9333 | 86,069.2333 |
| 16 | 0.5265 | 788.9000 | 146,487.0667 |
| 32 | 0.6469 | 1,208.0667 | 224,995.4667 |

This confirms that `head_size` is the simplest first selectivity lever. It is
not enough by itself because smaller heads may hurt Recall@100 on harder
queries, but it gives the next experiments a clear objective: retain the recall
of larger heads while keeping candidate and rerank counters closer to the
head-8 cost envelope.

## Medium-Term Exploration Direction

Before productizing an SQL/API surface, keep exploration focused on these five
tracks:

1. Workspace and memory lifecycle: keep the adaptive workspace, then test
   larger and more realistic payloads before changing the threshold.
2. Cross-dataset PostgreSQL benchmark: expand the M8 runner to all five
   datasets and a real commons/arxiv/pubmed query set when qrels exist.
3. Candidate selectivity: sweep impact-head size, atom reliability, and
   mixed/candidate-only atoms against `candidate_docs` and Recall@100.
4. Exact rerank cost: compress or restructure doc-row vectors so rerank scans
   fewer terms without losing the source-blind evidence-atom contract.
5. SQL model gate: keep the current read-only function surface experimental
   until quality, cost, and deep-fusion direction are stable.

The strategic goal remains broader than productizing BM25+SAE. The target is a
single evidence-atom retrieval engine where BM25 tokens, SAE latents, SPLADE or
BGE-M3 sparse features, graph-expanded atoms, and future learned mixed atoms
can share one physical sparse-impact abstraction.
