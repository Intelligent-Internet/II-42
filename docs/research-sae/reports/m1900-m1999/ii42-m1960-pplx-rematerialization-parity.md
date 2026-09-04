# M1960 PPLX Rematerialization Parity

## Purpose

The historical M310 MSMARCO `documents.jsonl` symlink in the active Spark root
is broken. This is an input-lineage defect, not a model result. M1960 first
tested whether the historical PPLX embedding surface could be rematerialized.
On 2026-07-15, the complete physical artifact copied with `rsync --copy-links`
was recovered on Betty, so rematerialization is now a fallback and parity
diagnostic rather than the primary replay source.

The parity source and target use:

- `perplexity-ai/pplx-embed-v1-0.6B` snapshot
  `2c4d510dd4a732063c31a0f70193e35067b51fd8`;
- identical hashes for `config.json`, `modules.json`, pooling config, and
  `model.safetensors`;
- SentenceTransformers `5.5.0` and Transformers `5.8.1`;
- no prefix and no output normalization;
- `max_seq_length=512` and `max_text_chars=6000`.

The source embeddings were materialized on the historical M150 hardware. The
parity target was recomputed on spark-2, so small floating-point differences
across architecture and CUDA kernels are expected.

## Recovered Historical Artifact

The preferred MSMARCO source is now:

`/Volumes/Betty/Tmp/ii42-beir15-pg-staging/msmarco/documents.jsonl`

Its strict recovery gate established:

| Measure | Result |
| --- | --- |
| Physical file, not symlink | yes |
| Bytes | 66,063,233,827 |
| Document rows | 8,841,823 |
| Document SHA-256 | `fa6e515b03add6cff23d3e4f19c91b7061f5b805dfeefaabbad94057dca9c4ba` |
| Query rows | 43 |
| Query SHA-256 | `b95b63fe5824909712ce6aa8d3417849db5cd2d714a41200d25eb8290b030555` |
| Query-ID digest | `ac460c58cd20175c656beb801d67729baa9ca72faf65ad02707b3bd70d87ffac` |
| Positive qrel pairs | 4,102 |
| Qrels SHA-256 | `f1d6165470bf61df7f0aecc6664bfc521509aa61edf42079b8cbfebf46279512` |
| Embedding dimension | 1,024 |
| Qrel document rows checked | 4,102 |

The verifier also checked fixed first, middle, and final IDs, periodic corpus
samples, finite embedding values, metadata identity, every qrel-referenced
document row, and exact query/qrels identity. The preparation summary binds
the physical document artifact to the historical `m150-beir-full-pplx` root.
The machine-readable evidence is
`/Volumes/Betty/II42/m1960-historical-selftrain-replay-v1/provenance/msmarco_historical_embedding_validation.json`.

## Dense And Posting Parity

On 32 held-out NFCorpus document rows:

| Measure | Result |
| --- | ---: |
| PPLX cosine, minimum | 0.999972 |
| PPLX relative L2, maximum | 0.007493 |
| M190 document support overlap, mean | 0.996094 |
| M190 document support overlap, minimum | 0.984375 |
| M190 document impact cosine, mean | 0.997455 |
| M190 document self top-10 overlap, mean | 0.990625 |

The stronger query-side test re-embedded all 323 evaluable NFCorpus queries,
encoded them through the frozen M190 query SAE, and scored them against the
same complete 3,633-document M190 surface:

| Measure | Result |
| --- | ---: |
| M190 query support overlap, mean | 0.997485 |
| M190 query support overlap, minimum | 0.975000 |
| M190 query impact cosine, mean | 0.998427 |
| Sparse ranking top-10 overlap, mean | 0.995975 |
| Sparse ranking top-100 overlap, mean | 0.995263 |
| Sparse ranking top-100 overlap, 5th percentile | 0.970000 |
| Sparse ranking top-1000 overlap, mean | 0.996749 |

## Decision

**USE THE RECOVERED HISTORICAL ARTIFACT.** It is the exact available M150 PPLX
lineage and avoids the measurable cross-hardware drift in the spark-2
rematerialization. PostgreSQL `halfvec(1024)` is not an admissible replacement
because it would introduce a new precision conversion before M190 export.

**PASS for rematerialization only as a fallback.** The exact model/software
contract retains more than 99.5% mean native ranking overlap, but its first
document embedding is not byte-identical to the recovered artifact. If the
historical artifact fails transfer or remote verification, the rematerialized
row may be reported only with the original uncertainty disclosure.
