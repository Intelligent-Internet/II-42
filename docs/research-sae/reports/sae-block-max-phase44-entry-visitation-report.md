# SAE Block-Max Phase 4.4 Entry-Visitation and MVCC Report

Date: 2026-05-12

## Purpose

Phase 4.4 completes the three read-only checks after the doc-bound fast path:

1. run a larger PostgreSQL-resident profile and record `query_terms`,
   `block_entry_visits`, payload bytes, resident memory, query time, opened
   docs, and decoded postings;
2. decide whether high-frequency dimensions need an additional skip layer;
3. simulate MVCC-visible top-k continuation without adding mutable index
   maintenance.

The phase stays read-only. It does not add delta overlay, mutable maintenance,
or heap visibility callbacks.

## Large-Scale Resident Profile

The profile uses deterministic tiling of the prepared benchmark latents. This
is intentionally not a quality benchmark: every tiled corpus has generated
exact top-k labels, and each PostgreSQL query is checked for exact equality.
The goal is to expose scaling behavior in the resident traversal counters.

Command:

```bash
PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/research_sae_large_scale_entry_profile.py \
  --temp-postgres \
  --library ./ii42.dylib \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --scale-factors 1,2,4 \
  --output-dir results/sae/phase44/entry-visitation
```

Results:

| Dataset | Scale | Docs | Exact | Query ms | Query terms | Block-entry visits | Visits/term | Opened docs | Payload MB | Memory MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 1 | 2000 | 100/100 | 1.372 | 64.00 | 7761.96 | 121.28 | 100.00 | 1.150 | 1.592 |
| `scifact` | 2 | 4000 | 100/100 | 4.665 | 64.00 | 15523.92 | 242.56 | 100.00 | 2.149 | 3.126 |
| `scifact` | 4 | 8000 | 100/100 | 7.001 | 64.00 | 31047.84 | 485.12 | 100.00 | 4.148 | 6.193 |
| `scidocs` | 1 | 2000 | 100/100 | 1.409 | 63.99 | 7318.08 | 114.36 | 100.00 | 1.155 | 1.598 |
| `scidocs` | 2 | 4000 | 100/100 | 4.704 | 63.99 | 14636.16 | 228.73 | 100.00 | 2.155 | 3.132 |
| `scidocs` | 4 | 8000 | 100/100 | 6.918 | 63.99 | 29272.32 | 457.45 | 100.00 | 4.154 | 6.199 |
| `nfcorpus` | 1 | 2063 | 100/100 | 1.361 | 63.77 | 6761.80 | 106.03 | 100.00 | 1.181 | 1.640 |
| `nfcorpus` | 2 | 4126 | 100/100 | 4.806 | 63.77 | 13523.60 | 212.07 | 100.04 | 2.212 | 3.222 |
| `nfcorpus` | 4 | 8252 | 100/100 | 6.806 | 63.77 | 27047.20 | 424.14 | 100.00 | 4.274 | 6.385 |
| `arguana` | 1 | 2000 | 100/100 | 3.125 | 64.00 | 7719.65 | 120.62 | 100.00 | 1.151 | 1.590 |
| `arguana` | 2 | 4000 | 100/100 | 4.756 | 64.00 | 15439.30 | 241.24 | 100.02 | 2.151 | 3.124 |
| `arguana` | 4 | 8000 | 100/100 | 7.040 | 64.00 | 30878.60 | 482.48 | 100.00 | 4.150 | 6.191 |
| `fiqa` | 1 | 2000 | 100/100 | 1.245 | 64.00 | 6620.86 | 103.45 | 100.01 | 1.154 | 1.604 |
| `fiqa` | 2 | 4000 | 100/100 | 4.480 | 64.00 | 13241.72 | 206.90 | 100.00 | 2.154 | 3.138 |
| `fiqa` | 4 | 8000 | 100/100 | 6.770 | 64.00 | 26483.44 | 413.80 | 100.00 | 4.153 | 6.205 |

The important result is the shape, not the absolute timing. `opened_docs`
remains pinned around `top_k`, and `decoded_postings` remains zero. However,
`block_entry_visits` and `visits/term` scale almost exactly with corpus size.
That means Phase 4.3 solved the opened-document selectivity problem, but it
exposed a new high-frequency dimension visitation problem.

## High-Frequency Dimension Skip Layer

The current doc-bound layout stores one block entry per matching document per
dimension. This is exact and simple, but for high-DF dimensions the traversal
still scans every matching document entry before it can form safe document
upper bounds.

The next physical layer should be a dimension-local super-block directory:

```text
dimension directory
  -> super-block range sorted by max impact or doc_ord
     -> doc-bound block entries
        -> exact one-doc score contribution
```

Each super-block stores:

- first entry offset;
- entry count;
- maximum impact in the super-block;
- optional doc-ordinal min/max for range diagnostics;
- optional compressed prefix metadata for later resident memory work.

Query traversal uses the query weight times super-block `max_impact` as a safe
upper bound. It expands a super-block only while that bound can still change
the visible top-k threshold. Exactness is preserved because the bound is over
the contained doc-level entries, not an approximate learned score.

This should be applied selectively:

- always keep the doc-bound exact entry as the scoring unit;
- use a direct flat entry range for low-DF dimensions;
- use a super-block directory for high-DF dimensions;
- classify high-DF dimensions by `entry_count`, `entry_count / doc_count`, and
  query-time `abs(query_weight)`.

## MVCC-Visible Continuation

The read-only facade already accepts an optional `tid[]` mapping. Phase 4.4
uses NULL TIDs inside that array to simulate invisible high-score documents.
The C traversal now:

- treats missing `doc_tids` as the no-MVCC fast path;
- treats NULL or out-of-range `doc_tids` entries as invisible;
- skips invisible docs when filling top-k;
- continues traversal until the visible top-k threshold is safe.

For the doc-bound fast path, `opened_docs` still counts visited candidate docs,
while `scored_docs` counts visible docs considered for top-k. This distinction
is useful because real heap visibility can force continuation beyond the first
`top_k` physical entries.

## Decision

Steps 1-3 are complete:

- resident exactness holds across the scaled profile;
- doc-bound opened-doc selectivity remains good;
- MVCC-visible continuation is covered by the PostgreSQL smoke test;
- the next bottleneck is high-frequency dimension entry visitation.

Step 4 should not be delta overlay or mutable maintenance yet. The next useful
implementation phase is a read-only super-block skip directory over doc-bound
entries, with the same exactness and diagnostics contract as the current
resident generation.

Phase 4.5 implemented this idea and found that document-range super-block
bounds are too loose to become the default. Keep this Phase 4.4 conclusion as
the motivation, but use `sae-block-max-phase45-superblock-skip-report.md` for
the superseding implementation decision.

## Verification

Commands run:

```bash
python3 -m py_compile \
  scripts/research_sae_large_scale_entry_profile.py \
  scripts/test_research_sae_resident_generation_pg.py \
  scripts/research_sae_resident_generation_benchmark.py

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
make

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/test_research_sae_resident_generation_pg.py \
  --temp-postgres \
  --library ./ii42.dylib

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/test_research_sae_packed_microblock_pg_generation.py \
  --temp-postgres \
  --library ./ii42.dylib

PATH=/opt/homebrew/opt/postgresql@18/bin:/opt/homebrew/opt/libpq/bin:$PATH \
python3 scripts/research_sae_large_scale_entry_profile.py \
  --temp-postgres \
  --library ./ii42.dylib \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --scale-factors 1,2,4 \
  --output-dir results/sae/phase44/entry-visitation
```
