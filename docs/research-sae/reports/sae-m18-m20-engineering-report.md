# SAE M18-M20 Engineering Report

Date: 2026-05-13

## Direction Confirmed

The current implementation direction is a unified physical engine, not a
two-stage late-fusion system.

```text
BM25 token atoms
+ SAE latent atoms
  -> one source-blind evidence-atom namespace
  -> impact-head candidate generation
  -> exact candidate rerank through doc-row sparse vectors
```

There is no second BM25-score plus SAE-score fusion stage in the physical
prototype. BM25 and SAE only differ as upstream atom producers. Once they enter
the payload, both are scored as weighted sparse evidence atoms.

## Implemented Artifacts

### M18: `EATMH002` Compact Payload

`EATMH002` adds a compact doc-row payload variant:

- impact-head candidate lists are still built from full rows;
- exact rerank doc vectors are compacted to a top-N doc-row budget;
- the current promoted budget is `doc128`;
- the binary layout stays compatible with the `EATMH001` reader shape, with a
  different magic string for diagnostics and versioning.

This is the key design choice: compacting candidate heads would directly cut
recall. Compacting only doc-row rerank vectors reduces rerank cost while
preserving the candidate pool.

### M19: C And PostgreSQL Read-Only Parity

The standalone C reader now accepts both:

```text
EATMH001
EATMH002
```

The PostgreSQL read-only generation loader also accepts both magics. The smoke
test covers:

- direct bytea query;
- resident by-id generation query;
- compact `EATMH002` by-id generation query;
- cache invalidation after upsert/delete;
- TID visibility filtering.

### M20: Efficiency Matrix Runner

The M20 runner is:

```text
scripts/research_sae_m20_efficiency_matrix.py
```

It consumes existing SAE artifacts:

```text
documents.jsonl
queries.jsonl
<run-name>/doc_latents.jsonl
<run-name>/query_latents.jsonl
```

It reports only efficiency and systems counters:

- payload bytes;
- atom count;
- doc pairs;
- head pairs;
- touched postings;
- candidate docs;
- rerank doc terms;
- Python prototype latency;
- standalone C latency;
- C/Python expected-order parity.

It intentionally does not report Recall, MRR, NDCG, or MAP. Real workload
quality still requires qrels or defensible proxy-qrels.

## Five-Dataset Result Matrix

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_milestone4_evidence_payload.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --head-sizes 16 \
  --doc-row-budgets 128 \
  --output-dir results/sae/m18/eatmh002
```

### Quality Guardrail

| Run | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `payload_head16` | 0.7088 | 0.7964 | 0.6790 | 0.6030 | 0.5009 |
| `payload_head16_doc128` | 0.7015 | 0.7937 | 0.6840 | 0.6104 | 0.5099 |

Decision:

```text
doc128 passes the compact-rerank gate.
```

Recall@100 drops by `0.0027`, while MRR@20, NDCG@10, and MAP@100 improve in
this matrix.

### Cost Counters

| Run | Touched postings | Candidate docs | Rerank doc terms | Python mean ms |
| --- | ---: | ---: | ---: | ---: |
| `payload_head16` | 1,381.6780 | 852.8940 | 146,385.0740 | 7.6619 |
| `payload_head16_doc128` | 1,381.6780 | 852.8940 | 106,998.4100 | 5.6893 |

The candidate stage is intentionally unchanged. The cost win comes from the
doc-row rerank section:

```text
rerank doc terms: -26.9%
```

### Payload Shape

| Payload | Magic | Bytes | Atoms | Doc budget | Doc pairs | Head pairs |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `head16` | `EATMH001` | 4,056,935.2000 | 20,499.0000 | 0 | 349,644.4000 | 103,786.6000 |
| `head16_doc128` | `EATMH002` | 3,279,632.8000 | 20,499.0000 | 128 | 252,481.6000 | 103,786.6000 |

Payload bytes drop by about `19.2%`, and doc pairs drop by about `27.8%`.
Head pairs are unchanged by design.

## C Reader Result

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_milestone5_evidence_c_reader.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --head-sizes 16 \
  --doc-row-budgets 128 \
  --strategies scan \
  --output-dir results/sae/m19/eatmh002-c
```

| Run | Exact ratio | Mean ms | Touched postings | Candidate docs | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `head16_scan` | 1.0000 | 0.2350 | 1,381.6780 | 852.8940 | 146,385.0740 |
| `head16_doc128_scan` | 1.0000 | 0.1726 | 1,381.6780 | 852.8940 | 106,998.4100 |

Decision:

```text
standalone C EATMH002 parity passes.
```

## PostgreSQL Smoke Result

Command:

```bash
make PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
  PG_SYSROOT="$(xcrun --show-sdk-path)"

PYTHONPATH=scripts python3 scripts/test_research_sae_evidence_atom_pg_generation.py \
  --temp-postgres \
  --library ./ii42.dylib
```

Result:

```text
PostgreSQL EATMH001/EATMH002 evidence-atom smoke passed
```

Decision:

```text
read-only PostgreSQL by-id EATMH002 path is implemented enough for the next
experimental engineering step.
```

## M20 Efficiency-Only Matrix

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m20_efficiency_matrix.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --head-sizes 16 \
  --doc-row-budgets 128 \
  --strategies scan \
  --output-dir results/sae/m20/efficiency-matrix
```

| Run | Python mean ms | C mean ms | Candidate docs | Rerank terms |
| --- | ---: | ---: | ---: | ---: |
| `head16` | 10.0596 | 0.2821 | 852.8940 | 146,385.0740 |
| `head16_doc128` | 7.4082 | 0.2356 | 852.8940 | 106,998.4100 |

This run validates the M20 harness and cost-reporting shape. It is still a
benchmark-artifact efficiency pass, not the final arxiv/pubmed/commons real
corpus pass.

## Parity Caveat

`fiqa/head16_doc128` has one strict Python-reference doc-order mismatch after
binary round-trip:

```text
strict exact = 0.9900
tie-tolerated exact = 1.0000
```

The mismatch is a float32 near-tie. The two swapped documents differ by about
`1.5e-8` after payload quantization. This does not indicate a reader bug:

- the C reader is exactly consistent with the serialized payload expected
  ranking;
- the PostgreSQL smoke passes;
- the tie-tolerated parity now makes this diagnostic explicit.

## Current Gate State

| Milestone | State | Decision |
| --- | --- | --- |
| M18 `EATMH002` payload | Implemented | Passed on five-dataset matrix |
| M19 C reader | Implemented | Passed deterministic expected-order parity |
| M19 PostgreSQL read-only smoke | Implemented | Passed direct/by-id/cache/TID smoke |
| M20 efficiency runner | Implemented | Passed benchmark-artifact efficiency matrix |
| M20 real arxiv/pubmed/commons corpus run | Pending data artifact | Requires dump plus SAE/query latent artifact |
| M20b real workload quality | Pending qrels | No Recall/MRR claim until labels or proxy labels exist |

## Next Engineering Step

The next useful step is not another late-fusion experiment. It is to prepare a
real workload efficiency artifact:

```text
elm commons dump
-> local documents/queries artifact
-> doc/query SAE latents
-> M20 efficiency runner
```

If real qrels or defensible proxy-qrels are added later, the same artifact can
be promoted to M20b quality validation.
