# SAE M21 Runtime Evidence-Atom Mainline Report

Date: 2026-05-17

## Decision

M21 is now centered on the source-blind evidence-atom payload family:

```text
EATMH001 full doc-row payload
EATMH002 compact doc-row payload
```

The canonical runtime SQL contract is:

```text
ii42_evidence_atom_query_by_id(
    generation_table,
    generation_id,
    query_atoms[],
    query_weights[],
    k,
    doc_tids
)
```

`UBMXM001` remains useful as a source-aware embedded-query research/control
payload, but it is not the M27 evaluation harness and must not define the
product API.

## Implemented Harness

The new M21 harness is:

```text
scripts/research_sae_m21_runtime_evidence_atom_parity.py
```

It builds `EATMH001/EATMH002` payloads, emits runtime query atom arrays from
the existing query atom builder, and compares:

```text
Python EvidencePayload.query
standalone C evidence payload reader
PostgreSQL ii42_evidence_atom_query_by_id
```

The report includes:

- top-k parity;
- tie-tolerated parity;
- score delta;
- candidate docs;
- candidate postings;
- rerank doc terms;
- memory bytes;
- generation cache hits.

The runner defaults to the local full15 root:

```text
/Volumes/Betty/Tmp/ii42_sae_beir15_shared
```

It can also be run against smaller dataset subsets for smoke checks.

## Smoke Result

Command:

```bash
PYTHONPATH=scripts python3 \
  scripts/research_sae_m21_runtime_evidence_atom_parity.py \
  --datasets scifact \
  --max-queries 2 \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_reports/m21-runtime-evidence-smoke \
  --temp-postgres
```

Result:

```text
Python/C/PG parity smoke passed
```

Smoke output:

| Payload | PG strict exact | PG tied exact | PG mean ms | Candidate docs | Candidate postings | Rerank terms | Cache hits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16` | 1.0000 | 1.0000 | 3.2557 | 669.0000 | 952.0000 | 123261.0000 | 1 |
| `head16_doc128` | 1.0000 | 1.0000 | 1.6604 | 669.0000 | 952.0000 | 85415.0000 | 1 |

The existing PostgreSQL regression smoke also passed:

```bash
PYTHONPATH=scripts python3 \
  scripts/test_research_sae_evidence_atom_pg_generation.py \
  --temp-postgres \
  --library ./ii42.dylib
```

Result:

```text
PostgreSQL EATMH001/EATMH002 evidence-atom smoke passed
```

## Full15 Result

Command:

```bash
PYTHONPATH=scripts python3 \
  scripts/research_sae_m21_runtime_evidence_atom_parity.py \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_reports/m21-runtime-evidence-full15 \
  --temp-postgres
```

Result:

```text
full15 Python/C/PostgreSQL by-id runtime evidence-atom parity passed
```

Python quality guardrail:

| Payload | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16` | 0.8365 | 0.8416 | 0.7506 | 0.7148 | 743.5798 | 109,857.3877 |
| `head16_doc128` | 0.8354 | 0.8414 | 0.7492 | 0.7136 | 743.5798 | 84,820.7950 |

Standalone C runtime-query parity:

| Run | Exact ratio | Mean ms | Candidate postings | Candidates | Rerank terms | Max score delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16_scan` | 1.0000 | 0.1658 | 1,048.4625 | 743.5798 | 109,857.3877 | 0.0000 |
| `head16_doc128_scan` | 1.0000 | 0.1333 | 1,048.4625 | 743.5798 | 84,820.7950 | 0.0000 |

PostgreSQL by-id runtime-query parity:

| Payload | Strict exact | Tie-tolerated | Mean ms | Candidates | Candidate postings | Rerank terms | Cache hits | Memory bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16` | 1.0000 | 1.0000 | 0.8112 | 743.5798 | 1,048.4625 | 109,857.3877 | 88.4667 | 6,668,155.8000 |
| `head16_doc128` | 1.0000 | 1.0000 | 0.7460 | 743.5798 | 1,048.4625 | 84,820.7950 | 88.4667 | 5,646,971.2667 |

This promotes `EATMH002/head16_doc128` as the current read-only engineering
mainline: it keeps the same candidate generation and near-identical ranking
quality while reducing rerank terms and resident memory. It also validates the
product-shaped runtime interface where SQL receives external query atom ids and
weights rather than an embedded query vector.

## M20 Real-Corpus Position

The existing M20 efficiency runner remains the correct path for real
arxiv/pubmed/commons artifacts:

```text
scripts/research_sae_m20_efficiency_matrix.py
```

Until qrels or defensible proxy-qrels exist, real-corpus output must stay
efficiency-only:

```text
quality_claim_allowed = false
```

The next M20 work is gated by M27. Real-corpus scale-up should wait until the
model lineage is chosen; until qrels or defensible proxy-qrels exist, it must
remain efficiency-only.

The first M20 pilot is now complete:

```text
sae-m20-real-corpus-pilot-report.md
```

It used 5k documents each from arxiv, pubmed, and policy CA chunks. Because
commons stores 256-dimensional vectors while the full15 teacher is 768
dimensional, the pilot trained a separate `shared_sae_4096_64` model only for
efficiency validation. `EATMH002/head16_doc128` remains the better payload
shape: C mean latency was `0.2503 ms`, and mean rerank terms dropped to
`136,740.0833`.

## M26 Side Track

The new M26 simulation runner is:

```text
scripts/research_sae_m26_dynamic_budget_sim.py
```

It implements post-hoc adaptive query/doc/head budget simulation over existing
artifacts. It does not train a model.

Smoke command:

```bash
PYTHONPATH=scripts python3 \
  scripts/research_sae_m26_dynamic_budget_sim.py \
  --datasets scifact \
  --max-queries 2 \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_reports/m26-dynamic-budget-smoke
```

Result:

```text
dynamic budget simulation smoke passed
```

The smoke is not a quality conclusion. It only validates that the side-track
runner can generate quality/cost rows before any expensive adaptive-k training.

Full15 command:

```bash
PYTHONPATH=scripts python3 \
  scripts/research_sae_m26_dynamic_budget_sim.py \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_reports/m26-dynamic-budget-full15
```

Full15 decision:

```text
adaptive budget is not promoted yet
```

The best initial adaptive heuristic, `adaptive_fanout`, cuts postings and
rerank terms, but loses too much quality versus `fixed_q96_d128_h16`. The
medium fixed profile `fixed_q64_d96_h12` is currently the cleaner low-cost
reference. M26 should continue as a learned-selector track inside M27 and
should not trigger product engineering by itself.

## Remaining Optional Command

Full M26 fixed sweep, expected to be more expensive and not required for the
current M27 closure plan:

```bash
PYTHONPATH=scripts python3 \
  scripts/research_sae_m26_dynamic_budget_sim.py \
  --fixed-sweep \
  --output-dir /Volumes/Betty/Tmp/ii42_sae_reports/m26-dynamic-budget-full15-sweep
```
