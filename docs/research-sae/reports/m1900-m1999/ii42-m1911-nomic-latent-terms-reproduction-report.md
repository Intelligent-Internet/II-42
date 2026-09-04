# M1911 Nomic Latent Terms Reproduction Report

Date: 2026-07-12

Decision: **the paper-shaped Nomic Latent Terms route is a real qrels-free
quality milestone and passes native BMP closure, but it narrowly misses the
predeclared five-seed Recall gate and is not yet the product parent. A larger
source-corpus run is evidence-authorized; post-hoc high-DF pruning is rejected.**

## Question Answered

M1910 established that latent BM25 can run as one exact non-negative posting
index, but its small project SAE reached only `0.572944` FiQA Recall@100.
M1911 replaced that checkpoint with the published representation recipe:

- frozen `nomic-ai/nomic-embed-text-v1.5` token states;
- a width-32,768, TopK-16 SAE;
- reconstruction-only training on FineWeb-Edu activations;
- sum pooling, square root, then latent BM25 with `k1=8`, `b=0.7`;
- five fixed seeds and qrels-free checkpoint selection;
- complete official FiQA and exact BMP evaluation.

The result closes both the representation and native-engine questions at the
10M-token pilot scale. It is not a claim to reproduce the paper's much larger
training exposure.

## Data And Training Audit

The pinned corpus contained `9,864,674` valid non-special activation tokens,
so the original `9.8M` train plus `200k` validation request was impossible.
A tokenizer-only capacity audit was added before model loading, and the frozen
split was corrected to `9.6M` train plus `200k` validation tokens. Three epochs
therefore exposed `28.8M` training-token presentations.

Five seeds completed 7,032 optimizer steps with an effective token batch of
4,096, a peak learning rate of `1e-3`, 5% warmup, and cosine decay. Retrieval
labels were never used in activation preparation, training, or seed selection.

The initial seed-1911 process became impractically slow after dead features
activated AuxK-512 decoding. The decoder was changed from one materialized
`[batch, aux_k, hidden]` tensor to exact chunked einsums. Forward values and
gradients were tested against the materialized reference. Because the original
seed had reached step 6,000 under the old kernel, it was preserved separately
and seed 1911 was rerun from step zero under the final code. The canonical
rerun changed normalized MSE by `-0.0000135`, cosine by `+0.0000075`, document
mean nnz by `+1.01`, head-1% share by `-0.000221`, and active-feature ratio by
`+0.00229`. These small differences do not alter the route conclusion.

## Qrels-Free Five-Seed Diagnostics

| Seed | Normalized MSE | Reconstruction cosine | Doc nnz | Head 1% share | Active features | maxDF |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1911 | 0.370356 | 0.869549 | 1,007.66 | 0.195339 | 0.626923 | 1.000000 |
| 1912 | 0.369945 | 0.869715 | 1,007.88 | 0.194974 | 0.627075 | 1.000000 |
| 1913 | **0.369197** | **0.870012** | 1,009.72 | **0.192961** | 0.628632 | 1.000000 |
| 1914 | 0.370497 | 0.869487 | 1,007.21 | 0.193979 | **0.631317** | 1.000000 |
| 1915 | 0.370055 | 0.869727 | **1,005.80** | 0.194895 | 0.628845 | 1.000000 |

Seed 1913 was selected before retrieval by the locked lexicographic rule:
validation normalized MSE, document maxDF, never-active ratio, then negative
usage entropy. All seeds used every feature during training, while roughly 63%
were active on held-out validation documents. Every seed still contained at
least one corpus-universal latent.

## Complete Official FiQA

| Seed | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 | Doc nnz |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1911 | 0.362546 | 0.305708 | 0.656321 | 0.443118 | 0.847016 | 519.26 |
| 1912 | 0.364398 | 0.307946 | 0.654052 | 0.445319 | 0.854068 | 517.14 |
| 1913 | 0.369016 | 0.311524 | **0.659994** | 0.448420 | 0.845799 | 519.05 |
| 1914 | **0.372682** | **0.314451** | 0.652127 | **0.454365** | 0.848405 | 520.22 |
| 1915 | 0.366389 | 0.308998 | 0.653968 | 0.449659 | **0.858262** | 521.41 |
| mean | 0.367006 | 0.309725 | 0.655292 | 0.448176 | 0.850710 | 519.42 |
| std | 0.003979 | 0.003369 | 0.003020 | 0.004310 | 0.005277 | 1.45 |
| OpenSearch sparse-v2 | 0.370244 | 0.310860 | 0.656042 | 0.456006 | 0.849637 | 177.41 |

The five-seed mean is within 5% of the OpenSearch control for NDCG and MAP,
and its candidate upper bound is slightly higher. Mean Recall misses the
locked `0.656042` gate by `0.000750`; selected seed 1913 exceeds the gate, but
retrieval was not allowed to select that seed and therefore cannot rescue the
five-seed milestone decision.

Relative to M1910's small SAE, the mean Recall improves by `+0.082349` and
NDCG improves by `+0.085051`. This is strong evidence that backbone choice,
qrels-free corpus scale, and paper-shaped training matter. It also shows why
the earlier small-SAE failure could not close the Latent Terms family.

## Selected-Seed Exact BMP Closure

| Surface | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| full float | 0.369016 | 0.311524 | 0.659994 | 0.448420 | 0.845799 |
| full BMP | **0.369506** | **0.312466** | 0.658295 | **0.450808** | **0.848217** |
| prune1 float | 0.340246 | 0.285384 | 0.631865 | 0.411320 | 0.838590 |
| prune1 BMP | 0.340612 | 0.286333 | 0.634979 | 0.413400 | 0.837487 |

| Surface | Strict parity | Recall retention | Index bytes | Bytes/doc | BMP p50 | BMP p95 | Exhaustive p95 | Speedup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| full | 1.000000 | 0.997425 | 327,735,171 | 5,686.10 | 23.415 ms | 27.495 ms | 55.894 ms | 2.033x |
| prune1 | 1.000000 | 1.004928 | 288,191,108 | 5,000.02 | 17.112 ms | 21.186 ms | 32.036 ms | 1.512x |

The full surface has `29,917,201` document postings, `64,329` query postings,
and exact integer top-100 boundaries for all 648 queries. Quantization retains
`99.74%` of float Recall and slightly improves NDCG, MAP, and MRR. The engine
gate therefore passes without a hidden approximate scorer.

Pruning the highest-DF 1% removes about 28.6% of document postings and lowers
p95 by 23%, but loses `0.023315` absolute BMP Recall and about `0.0289` NDCG.
The universal head is expensive, yet it carries useful retrieval signal. It
cannot be treated as dead capacity or removed after training.

## Interpretation

M1911 produces three durable conclusions:

1. **Training scale and a retrieval-native backbone matter.** A paper-shaped
   qrels-free SAE closes most of the quality gap that remained in M1910.
2. **Latent Terms is native-index compatible.** Exact BMP is faster than
   exhaustive sparse scoring and preserves selected-seed quality.
3. **The remaining bottleneck is representation cost, not engine correctness.**
   Quality depends on a dense high-frequency latent head and roughly 519
   document postings, about 2.9x the OpenSearch control nnz.

The result passes M1911D's evidence-based scale-up condition by a wide margin:
Recall exceeds `0.62` and improves M1910 by more than `0.04`. A directly
sampled, license-clean 100M-token continuation is therefore scientifically
justified. It is not automatically the next run: the consolidated reset first
compares this trainable latent route with the official Granite 30M Sparse and
OpenSearch vocabulary-sparse controls.

M1911 is not yet a product default. The five-seed Recall gate narrowly fails,
the model has only one official dataset, the derivative corpus has an
unresolved license declaration, and its native index is materially denser
than the mature vocabulary-sparse control.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1911-nomic-latent-terms-reproduction-contract.md`
- Activation builder: `scripts/prepare_m1911_nomic_activation_cache.py`
- SAE trainer: `scripts/train_m1911_nomic_latent_terms_sae.py`
- FiQA encoder: `scripts/prepare_m1911_latent_terms_fiqa.py`
- Five-seed summary: `scripts/summarize_m1911_latent_terms_fiqa.py`
- Native wrapper: `scripts/run_m1911_selected_seed_bmp_spark.sh`
- ClearML tasks: `eb32d4bd20444454ab3676cf86301e13`,
  `392917c5a21d48ba9724dbbd6f38a3b8`, and
  `6a800ac716b34c19a6d12ab01de22802`
- Remote training summary: `m1911b-nomic-topk-sae-v1/summary.json`
- Remote FiQA summary: `m1911c-nomic-fiqa-summary-v1/summary.json`
- Remote BMP summaries: `m1910-full-fiqa-b16-u32-v1/summary.json` and
  `m1910-prune1-fiqa-b16-u32-v1/summary.json`
