# SAE Full15 Native BM25+SAE Union Readiness Report

## Scope

This pass moves the BM25+SAE candidate-union result from pure Python quality
diagnostics into native/read-only index counters.

Quality source:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-bm25-sae-candidate-union-cost-sweep/
```

Native SAE counter source:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-native-sae-candidate-counters/
```

Read-only PostgreSQL smoke source:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-pg-d8-p128-smoke/
```

The current native payload still stores and traverses only SAE atom postings.
BM25 candidates are evaluated as a bounded lexical side input in Python. This
report therefore validates the native cost of the SAE side and the combined
quality/candidate frontier, but it is not yet a single physical BM25+SAE
payload.

## Current Retrieval Matrix

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 baseline | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| Snowflake-SAE teacher | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| Dense-pair student previous | 0.8232 | 0.8231 | 0.7086 | 0.6778 |
| Budget16 loss `0.01` | 0.8280 | 0.8264 | 0.7154 | 0.6837 |
| Budget16 loss `0.04` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| Budget16 loss `0.08` | 0.8327 | 0.8255 | 0.7223 | 0.6891 |

`budget16 loss=0.04` remains the balanced text-to-atoms training frontier.

## Native Counter Join

The table below joins BM25+SAE union quality with native C counters for the SAE
candidate side.

| Config | R@100 | MAP@100 | Union Candidates | Native SAE Candidates | Native SAE Postings | C ms/query | Native Overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `d8_p128_bm25200` | 0.8325 | 0.6913 | 739.1 | 615.9 | 782.5 | 1.126 | 0.8488 |
| `d16_p128_bm25100` | 0.8321 | 0.6910 | 1011.8 | 971.4 | 1449.5 | 1.346 | 0.9539 |
| `d8_p64_bm25200` | 0.8275 | 0.6909 | 531.7 | 387.3 | 454.9 | 0.994 | 0.6747 |
| `d8_p64_bm25300` | 0.8286 | 0.6915 | 606.2 | 387.3 | 454.9 | 0.994 | 0.6747 |
| `d32_p64_bm25200` | 0.8328 | 0.6922 | 1032.4 | 954.3 | 1404.2 | 1.344 | 0.9099 |

The balanced point is still:

```text
d8_p128_bm25200
```

It matches exact BM25+SAE quality while keeping the native SAE side near
`616` candidates, `783` postings, and `1.13 ms/query` in the standalone C
reader.

The latency-tier point is:

```text
d8_p64_bm25200
```

It reduces the native SAE side to about `387` candidates and `455` postings,
but Recall@100 falls from `0.8325` to `0.8275`. This is a real cost/quality
tradeoff, not just noise.

## PostgreSQL Read-Only Smoke

The selected balanced SAE candidate budget was also tested through the
read-only PostgreSQL resident generation by-id API:

```text
active_dims=8
postings_per_dim=128
impact_head_size=128
max_queries=20 per dataset
```

Full15 macro results:

| Metric | Value |
| --- | ---: |
| PG ms/query | 1.0125 |
| Exact overlap | 0.8482 |
| Candidate docs | 601.5367 |
| Decoded postings | 765.1267 |
| Rerank doc terms | 71238.2033 |
| Memory MB | 5.7304 |

The PG smoke matches the C counter shape closely enough for the current stage:
roughly `600` SAE candidates and `765` decoded postings per query. This means
the read-only PG surface can carry the current balanced SAE candidate budget.

## Exact Tie Note

`nfcorpus` exact C parity reports `98/100` strict matches because two queries
have adjacent equal-score documents ordered differently. The mismatch is a
tie-order issue and the benchmark now records it instead of failing the whole
counter run when `--allow-exact-mismatch` is set.

This flag should remain diagnostic-only. Production/readiness gates still need
strict or tie-aware parity tests, not silent mismatch acceptance.

## Decision

The next direction is no longer only training. The promising route is now:

```text
unified candidate generation:
  SAE impact-head candidates
  + BM25 token candidates
  -> exact normalized BM25+SAE rerank
```

Current targets:

```text
balanced:     d8_p128_bm25200
latency tier: d8_p64_bm25200
fallback:     d16_p128_bm25100
```

## Next Engineering Step

The next implementation should build a true unified physical payload simulator:

1. Add BM25 token impact-head candidate generation to the same native payload
   model instead of injecting BM25 candidates from Python.
2. Preserve the existing SAE impact-head counters.
3. Add source-level counters: BM25 candidates, SAE candidates, union overlap,
   final scored docs, and rerank score terms.
4. Re-run the full15 quality matrix on the same configs.
5. Only after that, port the unified payload to the read-only PostgreSQL
   generation function.

This keeps the implementation sequence disciplined: first one native simulator
that owns both sources, then PostgreSQL.
