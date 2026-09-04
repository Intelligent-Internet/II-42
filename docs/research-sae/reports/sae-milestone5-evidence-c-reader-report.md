# SAE Milestone 5 Evidence C Reader Report

Date: 2026-05-13

## Purpose

Milestone 4 proved the `EATMH001` evidence-atom payload in Python. Milestone 5
moves the same payload contract into a standalone C reader so the next
PostgreSQL read-only function can reuse the native traversal and parity
checks instead of depending on Python timing.

The payload represents BM25 token atoms and SAE latent atoms in one physical
namespace:

```text
query atom ids
  -> impact-head candidate union
  -> exact sparse rerank over candidate doc rows
  -> top-k doc ids and scores
```

This milestone still does not add mutable deltas or maintenance. It is a
read-only native execution checkpoint.

## Artifacts

```text
tests/research_sae_evidence_payload_reader.c
scripts/research_sae_milestone5_evidence_c_reader.py
results/sae/milestone5/evidence-c-reader/summary.md
results/sae/milestone5/evidence-c-reader/milestone5_evidence_c_reader.json
```

The benchmark harness writes a companion query payload:

```text
EATMQ001
```

That query payload stores sorted query atom ids, query weights, and expected
Python payload top-k rows. The C reader must match the expected doc order and
scores for every query.

## Verification

Commands:

```bash
python3 -m py_compile \
  scripts/research_sae_milestone4_evidence_payload.py \
  scripts/research_sae_milestone5_evidence_c_reader.py \
  scripts/test_research_sae_evidence_payload.py

cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  tests/research_sae_evidence_payload_reader.c \
  -o /tmp/research_sae_evidence_payload_reader

python3 scripts/research_sae_milestone5_evidence_c_reader.py \
  --datasets scifact \
  --max-queries 10 \
  --output-dir results/sae/milestone5/evidence-c-reader-smoke

python3 scripts/research_sae_milestone5_evidence_c_reader.py \
  --output-dir results/sae/milestone5/evidence-c-reader
```

Result:

```text
exact doc-order parity: 100%
max score delta: f32 round-trip noise only
```

## Quality Guardrail

Five datasets:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

| Head | Recall@100 | MRR@20 | Python mean ms |
| --- | ---: | ---: | ---: |
| `head8` | 0.7856 | 0.6791 | 4.6466 |
| `head16` | 0.7964 | 0.6790 | 7.6516 |
| `head32` | 0.7948 | 0.6790 | 11.4570 |

`head16` remains the default quality point. `head8` is still the lower-cost
exploratory profile, but it gives up about 1.1 Recall@100 points.

## C Reader Matrix

| Run | Exact ratio | Mean ms | Touched postings | Candidates | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `head8_scan` | 1.0000 | 0.1431 | 705.4 | 516.8 | 88089.6 |
| `head8_merge` | 1.0000 | 0.4459 | 705.4 | 516.8 | 126122.0 |
| `head8_seek` | 1.0000 | 0.8310 | 705.4 | 516.8 | 392846.1 |
| `head16_scan` | 1.0000 | 0.2300 | 1381.7 | 852.9 | 146385.1 |
| `head16_merge` | 1.0000 | 0.7210 | 1381.7 | 852.9 | 207056.6 |
| `head16_seek` | 1.0000 | 1.3608 | 1381.7 | 852.9 | 632871.6 |
| `head32_scan` | 1.0000 | 0.3485 | 2652.3 | 1268.1 | 219228.3 |
| `head32_merge` | 1.0000 | 1.0757 | 2652.3 | 1268.1 | 305622.8 |
| `head32_seek` | 1.0000 | 1.9387 | 2652.3 | 1268.1 | 913106.4 |

## Rerank Strategy Decision

Three exact rerank strategies were tested:

```text
scan:
  build a query-local dense atom weight array
  scan each candidate doc row once

merge:
  merge sorted query atom ids with sorted candidate doc rows
  avoids atom_count-sized query arrays

seek:
  binary-search every query atom in every candidate doc row
```

`scan` is the fastest current C path. For `head16`, it is about 33x faster
than the Python payload reader:

```text
Python head16 mean ms: 7.6516
C head16_scan mean ms: 0.2300
```

`merge` is slower on the 2k-document research payload, but it is still useful
as the scale-up fallback because it does not require allocating or clearing an
array sized by the full atom dictionary. `seek` is consistently worse because
query atom counts are high and repeated binary searches are cache-unfriendly.

## Implementation Implication

The next PostgreSQL read-only payload should start with the `scan` semantics,
but should not blindly allocate an atom-count dense array per SQL call once the
atom dictionary becomes large. A production-shaped implementation should use
one of these query-local forms:

```text
small/medium atom dictionary:
  reusable dense weight array + touched-atom reset list

large atom dictionary:
  sorted query atoms + merge fallback
```

The important point is that this is no longer a Python scorer bottleneck. The
candidate generation and exact candidate rerank loop are already fast enough
in C for the current research slices. The remaining systems work is PostgreSQL
residency, memory/cache ownership, and doc-row compression.

## Follow-Up Status

The `EATMH001` reader has now been ported into a PostgreSQL read-only SQL
surface:

```text
sae-milestone6-pg-evidence-atom-report.md
ii42_evidence_atom_query
ii42_evidence_atom_query_by_id
```

The next step is not another scorer change. It is parsed payload residency and
cache invalidation for generation-table reads, so SQL calls do not pay bytea
decode cost on every query.
