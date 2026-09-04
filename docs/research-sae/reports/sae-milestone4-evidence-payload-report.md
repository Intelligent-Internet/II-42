# SAE Milestone 4 Evidence Payload Report

Date: 2026-05-13

## Purpose

Milestone 3 selected the production-shaped direction:

```text
base evidence atoms
+ impact-head candidate directory
+ exact sparse rerank over candidate docs
```

This milestone turns that idea into a concrete read-only binary payload
prototype. The goal is not yet PostgreSQL integration. The goal is to prove
that the unified atom namespace can be serialized, decoded, queried, and kept
bit-for-bit equivalent in ranking behavior to the in-memory research path.

Runner:

```text
scripts/research_sae_milestone4_evidence_payload.py
```

Smoke test:

```text
scripts/test_research_sae_evidence_payload.py
```

Artifacts:

```text
results/sae/milestone4/evidence-payload/summary.md
results/sae/milestone4/evidence-payload/milestone4_evidence_payload.json
```

## Payload Format

The new experimental payload is:

```text
EATMH001 version 1
```

It is deliberately separate from `SBMXM001` for now. `SBMXM001` is still an
SAE-dimension oriented packed payload. `EATMH001` proves the more general
source-blind evidence atom contract first.

The payload stores:

```text
doc_ids
atom dictionary:
  atom_id -> atom key + source code
doc-row sparse vectors:
  doc_ord -> [(atom_id, impact)]
impact-head directory:
  atom_id -> top head_size [(doc_ord, impact)]
```

Query path:

```text
query text
  -> token atoms + SAE atoms
  -> atom_id lookup
  -> read impact-head lists
  -> union candidate doc_ord values
  -> exact rerank candidates through doc-row sparse vectors
```

This is approximate candidate generation with exact candidate rerank. It is
not exact full-index WAND, and that is intentional for the RAG candidate
generation use case.

## Mean Quality Matrix

Five datasets:

```text
scifact, scidocs, nfcorpus, arguana, fiqa
```

| Run | Recall@100 | MRR@20 | Touched postings | Candidate docs | Rerank doc terms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `payload_head8` | 0.7856 | 0.6791 | 705.4 | 516.8 | 88089.6 |
| `payload_head16` | 0.7964 | 0.6790 | 1381.7 | 852.9 | 146385.1 |
| `payload_head32` | 0.7948 | 0.6790 | 2652.3 | 1268.1 | 219228.3 |

This exactly matches the earlier in-memory `head8/head16/head32` frontier.

Decision:

```text
payload_head16 remains the default candidate quality point.
payload_head8 remains the low-cost exploratory profile.
```

## Payload Shape

Mean payload size over the five 2k-document slices:

| Payload | Bytes | Atoms | Doc pairs | Head pairs |
| --- | ---: | ---: | ---: | ---: |
| `head8` | 3795610.4 | 20499.0 | 349644.4 | 71121.0 |
| `head16` | 4056935.2 | 20499.0 | 349644.4 | 103786.6 |
| `head32` | 4417877.6 | 20499.0 | 349644.4 | 148904.4 |

The head directory is relatively small. The main payload body is the doc-row
sparse vector section, because exact rerank currently stores both lexical and
SAE atom impacts per document.

Approximate scale from this prototype:

```text
head16 ~= 4.06 MB / 2k docs ~= 2.03 GB / 1M docs
```

This is plausible for a server-side resident research index, but the doc-row
section should be compressed before any mutable production design.

## Parity

The binary payload reader was compared against the in-memory reference head
reranker for all five datasets and all three head sizes.

Result:

```text
exact doc-order parity: 100%
max score delta: f32 round-trip noise only
```

The self-contained smoke test also validates encode/decode/query without
depending on external BEIR artifacts.

Verification commands:

```bash
python3 -m py_compile \
  scripts/research_sae_milestone4_evidence_payload.py \
  scripts/test_research_sae_evidence_payload.py

python3 scripts/test_research_sae_evidence_payload.py

python3 scripts/research_sae_milestone4_evidence_payload.py \
  --datasets scifact \
  --max-queries 10 \
  --output-dir results/sae/milestone4/evidence-payload-smoke

python3 scripts/research_sae_milestone4_evidence_payload.py \
  --output-dir results/sae/milestone4/evidence-payload
```

## Bottleneck

The candidate generator is now compact and stable. The next bottleneck is
rerank cost:

```text
head16 touched postings = 1381.7
head16 candidate docs   = 852.9
head16 rerank doc terms = 146385.1
```

This happens because the exact reranker scans each candidate document's full
evidence-atom sparse vector. Unlike the earlier SAE-only v4 path, this vector
contains lexical token atoms as well as SAE atoms, so each candidate has many
more terms.

This is acceptable for proving the payload contract, but it is the first
native optimization target.

## Follow-Up Status

The next technical step has been completed in:

```text
sae-milestone5-evidence-c-reader-report.md
tests/research_sae_evidence_payload_reader.c
scripts/research_sae_milestone5_evidence_c_reader.py
```

The C reader keeps 100% doc-order parity and reduces `head16` mean query time
from about 7.65 ms in Python to about 0.23 ms in standalone C on the
five-dataset slice.

The next implementation step is to port the same payload into the PostgreSQL
read-only resident surface. Do not add mutable deltas or maintenance yet.

The remaining storage/query optimization list is:

1. doc-row scan, current baseline;
2. reusable query-local scan state or merge fallback for large atom dictionaries;
3. compressed doc-row ids and quantized impacts;
4. optional query-local temporary hash/bitmap for atom ids;
5. optional candidate-only mixed atom booster, still reranked by base atoms.

## Decision

Milestone 4 confirms the technical fusion path:

```text
BM25 token atoms and SAE latent atoms can share one binary evidence-atom
payload and one impact-head candidate query path.
```

The mainline is now ready to leave Python-only payload simulation and move to
a standalone C reader, then a PostgreSQL read-only function.
