# M1913 Granite 30M Sparse Native Control Report

Date: 2026-07-12

Decision: **Granite 30M Sparse is the strongest compact dense-root control and
the preferred research optimization parent, but it narrowly fails the frozen
balanced-quality gate and does not replace OpenSearch sparse-v2 as the product
default.**

## Question Answered

M1913 evaluated the released Apache-2.0
`ibm-granite/granite-embedding-30m-sparse` checkpoint at pinned revision
`ad82b1fd09541c998c8d45045d601c51fdb8a9b7`. No FiQA training or parameter
selection was performed.

The local encoder reproduces the official sparse modules exactly:
masked-language-model logits, attention masking, `ReLU`, `log1p`, max pooling,
and fixed top active dimensions of 192 for documents and 50 for queries. A
two-text parity audit against Sentence Transformers had identical support,
nnz, and impacts with maximum absolute error `0.0`.

## Complete Official FiQA

| Model | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 | Doc nnz | Query nnz | maxDF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Granite float | 0.352158 | 0.294734 | **0.666484** | 0.438824 | **0.861251** | 190.39 | 49.59 | 0.852372 |
| OpenSearch sparse-v2 float | **0.370244** | **0.310860** | 0.656042 | **0.456006** | 0.849637 | 228.76 | not reported | not reported |
| M1911 Nomic Latent Terms mean | 0.367006 | 0.309725 | 0.655292 | 0.448176 | 0.850710 | 519.42 | 99.51 | 1.000000 |

Granite improves Recall over OpenSearch by `+0.010442` and CUB by `+0.011614`
while using fewer document and query postings. It gives back `0.018086` NDCG,
`0.016126` MAP, and `0.017182` MRR.

The locked gate allowed at most 5% relative loss against OpenSearch:

- NDCG floor: `0.351732`; Granite passes by `0.000426`.
- MAP floor: `0.295317`; Granite fails by only `0.000583`.
- Recall floor: `0.656042`; Granite passes by `0.010442`.

This is a near miss, not a balanced-quality pass. The checkpoint is
recall-oriented on local FiQA, whereas OpenSearch remains the stronger head
ranking control.

## Exact BMP Closure

| Surface | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Granite float | 0.352158 | 0.294734 | 0.666484 | 0.438824 | 0.861251 |
| Granite BMP | 0.352112 | 0.294284 | 0.665725 | 0.438845 | 0.861416 |

| Check | Value |
| --- | ---: |
| Known IDs | 1.000000 |
| Returned-score parity | 1.000000 |
| Score-multiset parity | 1.000000 |
| Strict-boundary parity | 1.000000 |
| Mean set overlap | 0.999907 |
| Recall retention | 0.998862 |

The quantized BMP result preserves the model's retrieval shape. No approximate
engine or rescue surface is involved.

## Native Cost

| Measurement | Granite 30M | OpenSearch sparse-v2 | M1911 selected seed |
| --- | ---: | ---: | ---: |
| Document postings | 10,973,637 | 13,185,096 | 29,917,201 |
| Index bytes | **156,338,660** | 208,532,064 | 327,735,171 |
| Bytes/document | **2,712.42** | 3,617.96 | 5,686.10 |
| Build time | **3.624 s** | 4.924 s | 8.726 s |
| Peak build RSS | **1.109 GB** | 1.365 GB | 2.461 GB |
| Load time | 0.307 s | **0.255 s** | 0.529 s |
| BMP p50 | **11.491 ms** | 13.222 ms | 23.415 ms |
| BMP p95 | **12.555 ms** | 17.499 ms | 27.495 ms |
| Exhaustive p95 | **23.328 ms** | 37.568 ms | 55.894 ms |
| BMP p95 speedup | 1.858x | **2.147x** | 2.033x |

Granite is 25.0% smaller and 28.3% faster at BMP p95 than the OpenSearch
control on the same 57,638-document corpus. It is 52.3% smaller and 54.3%
faster than M1911's selected latent surface. The published top-dimension
constraint therefore produces a materially healthier native cost shape than
the local qrels-free latent SAE.

## Interpretation

This result supplies the mature starting point missing from the earlier
project-specific loss searches:

1. a small retrieval-trained dense root;
2. a standard sparse MLM output that is already distilled from a larger dense
   teacher;
3. explicit query/document activation budgets;
4. released Apache-2.0 weights;
5. full-corpus native evidence showing that the representation is practical.

Granite does not pass the contract's all-metrics product promotion gate, so
OpenSearch sparse-v2 remains the frozen balanced-quality default. However,
Granite is closer to the project's intended compact dense-root unified posting
encoder and offers a precise optimization target: improve boundary ordering
and head metrics without losing its `192/50` posting budgets, Recall, or BMP
cost. That is a much better-grounded next experiment than inventing another
from-scratch SAE or output loss.

The original Granite training is not fully reproducible because its data mix
contains internal and generated sources. Future training should therefore use
the checkpoint as a frozen regression anchor and implement comparable
dense-root distillation in an auditable framework such as Unified LSR. Any
adaptation must preserve this report's native quality and cost matrix.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1913-granite-30m-sparse-native-control-contract.md`
- Surface builder: `scripts/prepare_m1913_granite_sparse_bmp_surface.py`
- Runner: `scripts/run_m1913_granite_sparse_native_spark.sh`
- Model revision: `ad82b1fd09541c998c8d45045d601c51fdb8a9b7`
- Remote surface: `ii42-m1913-granite-30m-sparse-native-v1/surface/summary.json`
- Remote BMP result: `ii42-m1913-granite-30m-sparse-native-v1/run/summary.json`

## Sources

- Granite sparse checkpoint: <https://huggingface.co/ibm-granite/granite-embedding-30m-sparse>
- Granite Embedding Models: <https://arxiv.org/abs/2502.20204>
- Granite repository: <https://github.com/ibm-granite/granite-embedding-models>
