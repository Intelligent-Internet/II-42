# M1912A External SPLARE Native BMP Report

Date: 2026-07-12

Decision: **native engine shape passes. Revise the old M1510 raw-touch cost
stop, but do not promote the frozen adapter because its retrieval quality
remains below the mature OpenSearch control.**

## Question Answered

M1510 produced an independent Gemma-Scope/SPLARE-like sparse surface with real
FiQA quality, but its original report stopped deployment because the union of
query posting lists touched almost every document. M1910 later showed that raw
union touch can substantially overstate the work of a block-max sparse engine.

M1912A converts the frozen M1510 FiQA documents and queries into the same
patched official BMP 0.2 path used by M1660/M1910. It performs no retraining,
re-encoding, pruning, threshold selection, or qrels-driven model choice.

## Namespace Closure

The first audit surface exposed an engine-adapter limit: M1510 declares exactly
65,536 latent dimensions, while BMP requires fewer than 65,536 indexed terms.
Only 34,510 dimensions occur in any frozen document or query posting.

The final v2 surface applies one deterministic qrels-free remap over the sorted
union of nonzero term IDs. It removes 31,026 globally empty columns, persists
the original term IDs, and preserves:

- all 18,779,119 document postings;
- all 25,911 query postings;
- every query-document dot product;
- source document and query order.

The failed uncompressed-namespace surface and log remain available for audit.

## Quality

| Surface | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Frozen M1510 float | 0.333815 | 0.275953 | 0.636311 | 0.405323 | 0.836936 |
| Quantized exhaustive | 0.333471 | 0.276175 | 0.635797 | 0.405907 | 0.835393 |
| Official BMP | 0.333471 | 0.276175 | 0.635797 | 0.405907 | 0.835393 |

Quantized Recall retention is `0.999192`. BMP and quantized exhaustive quality
are identical. The small float-to-u8/u32 changes are within the locked 98%
Recall-retention floor and were not tuned on FiQA.

## Exactness

| Check | Result |
| --- | ---: |
| Strict boundary match | 1.000000 |
| Score multiset match | 1.000000 |
| Returned score match | 1.000000 |
| Known document IDs | 1.000000 |
| Mean top100 set overlap | 0.999923 |
| Minimum returned results | 100 |

The tiny set-overlap difference is confined to equal-score ties; strict
top-100 score and boundary parity are exact for all 648 queries.

## Native Cost

| Metric | Value |
| --- | ---: |
| BMP index bytes | 263,320,680 |
| Bytes/document | 4,568.53 |
| Build time | 6.14 s |
| Load time | 0.49 s |
| Build peak RSS | 1.86 GB |
| BMP p50 | 13.35 ms |
| BMP p95 | 15.22 ms |
| Exhaustive p50 | 28.21 ms |
| Exhaustive p95 | 29.80 ms |
| p95 speedup | 1.96x |

The maximum integer score is 134,488 and 628 of 648 queries exceed u16, so the
predeclared u32 accumulator is necessary.

## Interpretation

M1912A resolves an old measurement error. A nearly full raw posting union does
not make this frozen learned-sparse surface equivalent to exhaustive sparse
scoring. BMP's block bounds skip enough work to nearly halve p95 latency while
retaining exact top-100 boundaries.

This does not make M1510 the next model parent. Its frozen FiQA quality remains
below the locally verified OpenSearch sparse-v2 values (`0.370244` NDCG@10,
`0.310860` MAP@100, and `0.656042` Recall@100), and its independent training
provenance is incomplete. The result changes the engine conclusion only:

- high raw touch is no longer a valid standalone stop rule;
- full native BMP cost must be measured before rejecting a learned-sparse
  representation;
- exact pretrained-SAE/SPLARE training remains scientifically meaningful, but
  it must beat the mature quality baseline rather than merely pass latency.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1912a-external-splare-native-bmp-contract.md`
- Converter: `scripts/prepare_m1912_external_splare_bmp_surface.py`
- Runner: `scripts/run_m1912_external_splare_bmp_closure_spark.sh`
- Frozen surface: `ii42-m1912-external-splare-native-bmp-v1/surface-v2-compact`
- BMP summary: `ii42-m1912-external-splare-native-bmp-v1/run-v2-compact/summary.json`
- Index: `fiqa-b16-u32-compact.bmp`
