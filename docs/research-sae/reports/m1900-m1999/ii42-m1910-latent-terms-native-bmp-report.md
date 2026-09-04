# M1910 Latent Terms Native BMP Report

Date: 2026-07-12

Decision: **the Latent Terms index shape is engine-viable, but the old M1540
checkpoint fails full-corpus quality. Close M1540 and retain a clean
Nomic/FineWeb reproduction as a separate published route.**

## Complete Official Surface

M1910 reused the frozen M1540A BGE checkpoint without retraining and encoded
the complete official FiQA surface:

- 57,638 documents;
- 648 test queries with qrels;
- 32,768 latent dimensions and token TopK-16;
- sum pooling followed by square root;
- latent BM25 with fixed `k1=8`, `b=0.7`;
- qrels used only after frozen encoding.

| Surface | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 | Doc nnz | Query nnz | maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| full float | 0.281955 | 0.234144 | 0.572944 | 0.372689 | 0.788315 | 247.4 | 94.8 | 0.980811 |
| prune1 float | 0.257636 | 0.211465 | 0.534492 | 0.334759 | 0.753267 | 210.3 | 82.6 | 0.068913 |

Removing the corpus-global highest-DF 1% features reduced maxDF dramatically,
but retained only `93.29%` of full float Recall@100. It fails the predeclared
`98%` quality-retention gate. The 2,000-document M1540 canary substantially
overestimated full-corpus quality.

## Patched Official BMP

The exact M1660 BMP implementation was reused unchanged: upstream
`c0a17ffc`, exact-topK patch, block size 16, source document order, global u8
document quantization, per-query f32 scaling, and u32 score accumulation.

| Surface | Strict parity | Quant Recall retention | Index bytes | Bytes/doc | BMP p50 | BMP p95 | Exhaustive p95 | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| full | 1.000000 | 0.999416 | 252,933,594 | 4,388.31 | 15.727 ms | 17.905 ms | 21.628 ms | 1.208x |
| prune1 | 1.000000 | 1.007766 | 230,575,706 | 4,000.41 | 12.623 ms | 14.822 ms | 17.898 ms | 1.208x |

All 648 queries returned exact top-100 score multisets and exact
strict-boundary sets. Quantized full Recall@100 was `0.572610`; prune1 was
`0.538643`.

The full Latent Terms index is only `1.21x` the bytes per document of the
M1660 OpenSearch control and its p95 is only `1.02x`. Despite maxDF near one,
the real BMP engine remains practical. The old raw posting-union touch metric
was therefore not a valid stop signal.

## Interpretation

M1910 separates two previously mixed conclusions:

1. **Engine/representation shape passes.** Latent BM25 can be compiled into one
   exact non-negative posting index with practical size and latency.
2. **M1540 model quality fails.** A 262,144-token BGE SAE trained on 4,096
   documents does not reproduce the paper's full-corpus retrieval quality.

The result does not justify more M1540 tuning. The published Latent Terms route
uses Nomic retrieval token states, FineWeb-Edu, a 32,768/TopK-16 SAE, and much
larger qrels-free training exposure. A future reproduction must use those
inputs and multiple fixed seeds; it cannot claim continuity from the M1540
canary.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1910-latent-terms-native-bmp-contract.md`
- Surface compiler: `scripts/prepare_m1910_latent_terms_bmp_surface.py`
- BMP runner: `scripts/run_m1910_latent_terms_bmp_benchmark_spark.sh`
- Full remote summary: `m1910-full-fiqa-b16-u32-v1/summary.json`
- Pruned remote summary: `m1910-prune1-fiqa-b16-u32-v1/summary.json`
