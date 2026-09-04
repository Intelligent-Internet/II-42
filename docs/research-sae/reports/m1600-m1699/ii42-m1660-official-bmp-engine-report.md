# M1660 Official BMP Engine Report

Date: 2026-07-11

Decision: **the mature vocabulary-sparse representation and a single exact
inverted index are jointly viable; authorize one literature-grounded
medium-scale learned-sparse training line.**

## Surface

M1660 reused the frozen complete FiQA output of
`opensearch-project/opensearch-neural-sparse-encoding-v2-distill`:

- 57,638 documents and 648 evaluated queries;
- 30,522 vocabulary dimensions;
- 13,185,096 document postings;
- 8-bit corpus-global document impacts and BMP-native f32 query normalization
  to maximum impact 32;
- official BMP block size 16, compressed range maxima, exact top100;
- source document order, without BP reordering.

The engine is upstream commit `c0a17ffc` plus the exact-topK correctness patch
documented by M1660A. Patch SHA-256 is
`c9c8681075f9a8ff9c759e02161812e06608e2e78ed1500b198f739159d86818`.

## Quality

| Surface | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Float sparse reference | 0.370244 | 0.310860 | 0.656042 | 0.456006 | 0.849637 |
| Quantized exhaustive | 0.369782 | 0.311634 | 0.655266 | 0.455916 | 0.852981 |
| Patched official BMP | 0.369782 | 0.311632 | 0.655266 | 0.455916 | 0.852981 |

Quantized Recall@100 retained `99.8818%` of the float sparse reference. The
very small BMP/exhaustive MAP difference is caused only by deterministic
ordering inside equal integer-score ties.

## Exactness

All 648 queries returned 100 documents:

| Check | Ratio |
| --- | ---: |
| Known document IDs | 1.000000 |
| Returned document score equals exhaustive integer dot | 1.000000 |
| Top100 score multiset | 1.000000 |
| Strictly-above-boundary set | 1.000000 |
| Mean lexical-set overlap inside ties | 0.999861 |

The first run appeared to have one score mismatch. The cause was the auditor
using f64 for query scaling while the public Python binding accepts f32. Query
2264 had one exact ceil boundary (`13` in f64, `14` in binding f32). Repeating
the audit with binding-identical f32 arithmetic removed the mismatch without
changing the index or engine.

## Cost

| Measurement | Value |
| --- | ---: |
| Index build | 4.924 s |
| Peak build RSS | 1.365 GB |
| Serialized index | 208,532,064 B |
| Bytes/document | 3,617.96 B |
| Index load | 0.255 s |
| BMP latency mean / p50 | 12.288 / 13.222 ms |
| BMP latency p95 / p99 | 17.499 / 18.713 ms |
| Exhaustive p95 | 37.568 ms |
| p95 speedup | 2.147x |

This source-order result is not the paper's best cost surface because official
BP document ordering was not applied. It already passes the predeclared gate,
so BP is not authorized as a post-hoc rescue or tuning step.

## Interpretation

M1630's signed PCA representation required near-full work. M1640's Python block
proxy then exposed a traversal/metadata trade-off. M1660 resolves that
ambiguity: a retrieval-trained vocabulary output retains quality after 8-bit
quantization and executes exactly in one real inverted index. The project no
longer needs to prove that learned semantic postings are an engineerable
retrieval form.

The remaining bottleneck is model quality and training scale. This result does
not promote the public OpenSearch model as the project model, nor does it
authorize another arbitrary output head. It authorizes a controlled
learned-sparse foundation experiment whose cost is measured against this BMP
surface.

## Artifacts

- Contract: `docs/research-sae/reports/m1600-m1699/ii42-m1660-official-bmp-engine-contract.md`
- Correctness report: `docs/research-sae/reports/m1600-m1699/ii42-m1660a-bmp-correctness-patch-report.md`
- Patch: `patches/bmp-0.2-exact-topk.patch`
- Benchmark: `scripts/benchmark_m1660_official_bmp.py`
- Runner: `scripts/run_m1660_official_bmp_local.sh`
- Result: `/Volumes/Betty/Tmp/ii42-m1660-official-bmp-v1/runs/m1660b-opensearch-fiqa-source-order-b16-f32-v1/summary.json`

No M1660 process remains.
