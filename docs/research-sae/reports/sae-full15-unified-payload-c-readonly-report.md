# Unified BM25+SAE Read-Only Payload C Report

Date: 2026-05-14

## Scope

This phase converts the unified native payload simulator into a read-only C
payload prototype. The new payload stores BM25 token dimensions and SAE atom
dimensions in the same binary structure:

```text
dimension source tags
doc vectors
impact heads
query dimensions
expected top-k rows
```

The C reader reconstructs the same physical query path as the Python exporter:

```text
selected SAE atom heads
+ selected BM25 token heads
  -> candidate union
  -> source-aware normalized BM25+SAE rerank
```

This is still read-only research payload work. It does not add mutable index
maintenance yet.

Artifacts:

```text
scripts/research_sae_unified_payload_export.py
scripts/research_sae_unified_payload_c_benchmark.py
tests/research_sae_unified_payload_reader.c
scripts/test_research_sae_unified_payload_pg.py
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-unified-payload-c-full15-smoke/
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-unified-payload-c-full15-bm25term16-smoke/
```

## C/Python Parity

The 15-dataset smoke uses the first 50 queries per dataset, except datasets
with fewer available queries. Every dataset reached exact C/Python top-k parity.

| Variant | Exact Match Rate | Mean Candidates | SAE Posts | BM25 Posts |
| --- | ---: | ---: | ---: | ---: |
| All BM25 query terms | 1.0000 | 740.9 | 787.4 | 679.6 |
| Top-16 BM25 query terms | 1.0000 | 738.5 | 787.4 | 381.4 |

The quality columns in the benchmark markdown are smoke-test diagnostics over
the capped query subset. They should not replace the full 15-BEIR quality
matrix from the Python simulator.

## BM25 Query-Term Cap

The first C smoke exposed a long-query cost problem. `arguana` averaged about
98 BM25 query tokens, so even `bt64` still read about 4,392 BM25 postings/query.

Adding `bm25_active_terms=16` fixes this bottleneck:

| Dataset | Variant | BM25 Terms | BM25 Posts | Candidates | Recall@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | all terms | 98.5 | 4392.4 | 714.0 | 0.9800 |
| `arguana` | top-16 terms | 16.0 | 226.3 | 676.7 | 0.9800 |

Across the 15-dataset capped smoke, top-16 BM25 terms reduce BM25 posting reads
from about 680/query to about 381/query without changing the macro Recall@100
on this smoke subset. This mirrors the SAE-side active-dims strategy and should
be part of the next native payload contract.

## Current Native Contract

The best current engineering target is:

```text
sae_active_dims = 8
sae_postings_per_dim = 128
bm25_active_terms = 16
bm25_postings_per_term = 64
bm25_candidate_k = 200
```

This target keeps candidate rows around 738/query in the capped full15 C smoke,
with about 787 SAE postings/query and 381 BM25 postings/query.

## Productization Implications

The engineering direction is now clearer:

1. Keep one physical payload with source tags rather than separate BM25 and SAE
   candidate indexes.
2. Keep source-aware scoring in the reader, because source-blind scoring loses
   the normalized BM25+SAE score surface.
3. Add independent query-side budgets for semantic atoms and lexical tokens.
4. Preserve detailed diagnostics by source: BM25 terms, SAE terms, source
   postings, source candidates, overlap, score visits, and parity.
5. Port this read-only payload shape into PostgreSQL after the C reader grows
   larger-corpus memory and latency checks.

## PostgreSQL Read-Only Follow-Up

The first PostgreSQL integration now exists as:

```text
ii42_unified_payload_query(bytea, query_filter, tid[])
ii42_unified_payload_query_by_id(regclass, generation_id, query_filter, tid[])
```

It keeps the same unified candidate-union and source-aware rerank path as the
Python exporter and standalone C reader. The smoke test validates direct bytea
query parity, resident-table by-id parity, query filtering, TID mapping, and
diagnostic counters on an isolated temporary PostgreSQL instance.

The PostgreSQL path intentionally decodes `UBMXM001` directly for now instead
of adding a new backend cache kind. A cache should be added only after larger
payload benchmarks show decode cost is the next bottleneck.

## Next Steps

The next phase should focus on larger-payload SQL benchmarking and runtime
query shape, not training:

```text
Python exporter -> compact bytea payload -> C reader parity
PostgreSQL bytea/by-id generation loading -> SQL-callable read-only query
larger corpus payload -> memory, decode cost, traversal cost, and p95 latency
runtime query atoms -> production-shaped read-only API draft
```

Training should resume only if the PostgreSQL payload shows a representation
gap that cannot be explained by candidate budget or payload traversal.
