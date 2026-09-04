# SPLADE Baseline Report

Date: 2026-05-12

## Purpose

This pass tests SPLADE before SQL or native integration. The goal is to answer
whether a learned sparse text encoder should become the next active source in
the generic sparse-impact engine.

SPLADE is evaluated as another weighted sparse source:

```text
source_id = splade
source_dim_id = tokenizer vocabulary id
weight = learned sparse lexical/expansion impact
```

## Model And Configuration

- Model: `naver/splade_v2_distil`
- Encoder implementation: `sentence_transformers.SparseEncoder`
- Document active dimensions: `128`
- Query active dimensions: `64`
- Datasets: `scifact`, `scidocs`, `nfcorpus`, `arguana`, `fiqa`
- Output:

```text
results/sae/phase5/splade-baseline/
```

The selected model is public and lightweight enough for a first local baseline.
It is not the strongest possible SPLADE family member. Stronger or gated
checkpoints can be evaluated later, but this run is sufficient to decide
whether the current public v2-distil path should enter SQL now.

## Normalized Matrix

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 0.6025 | 0.7035 | 0.5904 | 0.5031 | 0.4134 | 5.4692 |
| `bm25_sae` | 0.7045 | 0.7947 | 0.6835 | 0.6036 | 0.5052 | 6.6048 |
| `splade` | 0.6580 | 0.7519 | 0.6324 | 0.5559 | 0.4615 | 5.9563 |
| `bm25_splade` | 0.6620 | 0.7445 | 0.6402 | 0.5570 | 0.4593 | 6.0907 |
| `bm25_sae_splade` | 0.7122 | 0.7926 | 0.6827 | 0.6087 | 0.5094 | 6.7640 |

## Fixed-Saturation Matrix

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 0.6025 | 0.7035 | 0.5904 | 0.5031 | 0.4134 | 5.5364 |
| `bm25_sae` | 0.6971 | 0.7934 | 0.6742 | 0.5952 | 0.4987 | 5.6762 |
| `splade` | 0.6580 | 0.7519 | 0.6324 | 0.5559 | 0.4615 | 6.3750 |
| `bm25_splade` | 0.6545 | 0.7431 | 0.6399 | 0.5539 | 0.4572 | 7.9785 |
| `bm25_sae_splade` | 0.7030 | 0.7899 | 0.6814 | 0.6032 | 0.5043 | 7.6224 |

## Interpretation

SPLADE v2-distil is useful as a learned sparse baseline, but it does not beat
the current BM25+SAE path on the five-dataset mean.

The best normalized result for recall remains `bm25_sae`:

```text
BM25+SAE Recall@100 = 0.7947
BM25+SAE+SPLADE Recall@100 = 0.7926
```

The tri-source path improves NDCG and MAP slightly:

```text
BM25+SAE NDCG@10 = 0.6036
BM25+SAE+SPLADE NDCG@10 = 0.6087
BM25+SAE MAP@100 = 0.5052
BM25+SAE+SPLADE MAP@100 = 0.5094
```

That improvement is too small to justify SQL integration for this SPLADE
checkpoint. It is better treated as a reference baseline for now.

## Why SPLADE Lost To SAE In This Run

This result should not be read as "SPLADE is worse than SAE" in general. It
means this specific setup did not beat the current SAE path:

```text
naver/splade_v2_distil
doc_active_dims = 128
query_active_dims = 64
splade_weight = 1.0
five sampled BEIR-style datasets
current normalized / fixed-saturation fusion formulas
```

There are several plausible reasons:

1. SAE has a corpus-adapted advantage. The SAE path is built over the current
   Snowflake dense embeddings, so it inherits the dense semantic geometry of
   this corpus slice. SPLADE v2-distil is a general checkpoint and was not
   adapted to these sampled corpora.
2. Snowflake is already strong on semantic retrieval. SAE-over-dense can
   preserve some of that semantic recall in sparse form. SPLADE is still
   anchored in lexical vocabulary ids and expansion terms, which is valuable
   but may underperform on larger semantic-span datasets such as `fiqa` and
   `scidocs`.
3. The document-side SPLADE cap is probably too tight. Most datasets reached
   roughly `128` active dimensions per document, meaning the cap is truncating
   output rather than preserving the model's natural sparsity. SPLADE often
   needs more document-side expansion terms to show its full recall behavior.
4. SPLADE fusion was not tuned. `BM25+SAE` uses a previously validated
   `sae_weight = 2.0`, while this SPLADE pass used `splade_weight = 1.0`.
   Some datasets show `BM25+SPLADE` worse than pure SPLADE, which is usually a
   calibration/fusion warning rather than proof that the SPLADE signal is bad.
5. `naver/splade_v2_distil` is a practical public baseline, not necessarily
   the strongest SPLADE-family checkpoint. Stronger checkpoints or larger
   budgets may produce a different result.

The useful interpretation is therefore:

```text
SPLADE remains an important learned-sparse reference direction, but this
particular public v2-distil baseline does not justify SQL/native integration
ahead of BM25+SAE fixed-saturation work.
```

If this direction is reopened, the next fair comparison should sweep:

- document active dimensions: `128`, `256`, `512`;
- query active dimensions: `64`, `128`;
- `splade_weight`;
- stronger SPLADE checkpoints;
- overlap analysis showing whether SPLADE retrieves relevant documents missed
  by SAE and dense.

## Operational Notes

Initial SPLADE encoding for the five-dataset matrix took roughly:

```text
document encoding: 199 seconds, excluding the earlier cached SciFact smoke
query encoding: 5.8 seconds, excluding the earlier cached SciFact smoke
```

Query encoding is outside the reported `Mean ms` ranking cost. In production,
document encodings can be precomputed, but query encoding is an online cost.

## Decision

Do not move `naver/splade_v2_distil` into SQL sidecar or native payload as the
next implementation step.

Keep SPLADE support at the offline baseline layer. Reopen only if one of these
happens:

- a stronger SPLADE checkpoint is available without operational friction;
- larger active-dimension budgets materially improve quality;
- real arxiv/pubmed/commons qrels show a workload where SPLADE covers misses
  that SAE and dense do not cover.
