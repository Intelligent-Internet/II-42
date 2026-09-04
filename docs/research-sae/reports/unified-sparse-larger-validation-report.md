# Unified Sparse Larger Validation Report

Date: 2026-05-10

## Objective

Validate the unified `BM25 + SAE` sparse impact index on a larger and harder
dataset than the initial SciFact 400-document slice.

This run uses BEIR SciDocs with Snowflake embeddings and the same SAE
configuration:

```text
embedding model: Snowflake/snowflake-arctic-embed-m-v2.0
input dimensions: 256
SAE latent dimensions: 8192
SAE active dimensions: 32
SAE score mode: normalized_idf_dot
training device: mps
```

## Dataset

```text
dataset: BEIR SciDocs
documents: 2000
queries: 100
seed: 37
```

The data was prepared with:

```bash
python3 scripts/research_sae_prepare_beir.py \
  --dataset scidocs \
  --cache-dir /tmp/beir \
  --output-dir /tmp/ii42_sae_scidocs_2k/data \
  --max-docs 2000 \
  --max-queries 100 \
  --batch-size 16 \
  --seed 37
```

## SAE Training

```bash
python3 scripts/research_sae_sweep.py \
  --documents /tmp/ii42_sae_scidocs_2k/data/documents.jsonl \
  --queries /tmp/ii42_sae_scidocs_2k/data/queries.jsonl \
  --output-dir /tmp/ii42_sae_scidocs_2k/sweep_8192_active32 \
  --latent-dims 8192 \
  --active-dims 32 \
  --score-modes normalized_idf_dot \
  --epochs 20 \
  --batch-size 256 \
  --top-k 100 \
  --seed 37 \
  --device mps
```

## Single-Source Metrics

| source | Recall@20 | Recall@100 | MRR@20 | MRR@100 |
| --- | ---: | ---: | ---: | ---: |
| dense vector | `0.5195` | `0.6740` | `0.6544` | `0.6560` |
| BM25 | `0.4315` | `0.5720` | `0.5455` | `0.5474` |
| SAE `8192/32` | `0.4440` | `0.6400` | `0.5664` | `0.5697` |

SciDocs is a better stress test than the first SciFact slice. Dense vector
retrieval is clearly stronger than BM25, and SAE sits between BM25 and dense.
That means this corpus is useful for judging whether SAE can recover semantic
recall that BM25 misses.

SAE overlap:

```text
SAE vs dense top20 overlap: 0.4750
SAE vs BM25 top20 overlap: 0.3005
```

The low BM25 overlap is useful: SAE is not merely duplicating lexical ranking.

## Unified Sparse Index Size

| source | dimensions | postings |
| --- | ---: | ---: |
| BM25 | `19471` | `207188` |
| SAE | `1461` | `64000` |
| total | `20932` | `271188` |

The SAE side adds exactly:

```text
documents * active_dims = 2000 * 32 = 64000 postings
```

## Unified BM25 + SAE Weight Sweep

| SAE weight | Recall@20 | Recall@100 | MRR@20 | MRR@100 |
| ---: | ---: | ---: | ---: | ---: |
| `0.10` | `0.4510` | `0.5905` | `0.5553` | `0.5569` |
| `0.25` | `0.4610` | `0.6050` | `0.5838` | `0.5857` |
| `0.50` | `0.4710` | `0.6190` | `0.6101` | `0.6120` |
| `0.75` | `0.4875` | `0.6450` | `0.6276` | `0.6289` |
| `1.00` | `0.4885` | `0.6470` | `0.6280` | `0.6293` |
| `1.25` | `0.4905` | `0.6510` | `0.6212` | `0.6223` |
| `1.50` | `0.4945` | `0.6510` | `0.6234` | `0.6246` |
| `2.00` | `0.4920` | `0.6490` | `0.6207` | `0.6229` |

Best MRR balance:

```text
bm25_weight = 1.0
sae_weight = 1.0
```

Best Recall@20 in this sweep:

```text
bm25_weight = 1.0
sae_weight = 1.5
```

For the current prototype, `sae_weight=1.0` remains the best default because it
has the strongest MRR and nearly the same recall as higher SAE weights.

## SQL Sidecar Parity

The unified SQL sidecar loaded the same single postings table and matched the
Python scorer exactly:

```text
queries: 100
ranking diffs: 0
Recall@20: 0.4885
Recall@100: 0.6470
MRR@20: 0.6280
MRR@100: 0.6293
```

This confirms that the unified sparse query shape can be expressed as one SQL
request over a single postings table.

## Dense Gap Analysis

The gap analysis script is:

```text
scripts/research_sae_gap_analysis.py
```

It compares dense, BM25, SAE, and unified `BM25 + SAE` query by query. On the
SciDocs 2k slice:

```text
dense unique relevant hits over unified@20: 40
unified unique relevant hits over dense@20: 25
dense unique relevant hits over BM25@20: 64
unified unique relevant hits over BM25@20: 34
SAE unique relevant hits over BM25@20: 46
```

This means dense is still stronger, but not one-directionally dominant.
Unified sparse retrieval finds relevant documents that dense misses, and SAE
is clearly adding non-lexical recall beyond BM25.

Dense-only examples tend to be semantic-neighbor cases rather than exact term
matches. In the top dense-over-unified examples, the mean query/document
lexical overlap was only `0.323`. Representative examples include:

- `3D ActionSLAM: wearable person tracking in multi-floor environments`
  matching `SignalSLAM: Simultaneous localization and mapping with mixed WiFi,
  Bluetooth, LTE and magnetic signals`.
- `Calcium hydroxylapatite for jawline rejuvenation` matching filler-related
  vascular occlusion treatment literature with low direct token overlap.
- `DeepMem: Learning Graph Neural Network Models for Fast and Robust Memory
  Forensic Analysis` matching broader graph/structured-data representation
  papers.

These are exactly the cases where SAE-over-dense needs better semantic
coverage or a larger active-dimension budget.

## Active Dimension Scaling

The first larger run used `active_dims=32`. To test whether SAE quality is
capacity-limited, the same SciDocs data was rerun with `active_dims=64` and
`active_dims=128`.

Single-source SAE:

| active dims | SAE postings | unique dims | Recall@20 | Recall@100 | MRR@20 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `32` | `64000` | `1461` | `0.4440` | `0.6400` | `0.5664` |
| `64` | `128000` | `2401` | `0.4790` | `0.6425` | `0.6242` |
| `128` | `256000` | `3634` | `0.4990` | `0.6570` | `0.6443` |

Unified `BM25 + SAE` best observed configurations:

| active dims | SAE weight | total postings | Recall@20 | Recall@100 | MRR@20 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `32` | `1.0` | `271188` | `0.4885` | `0.6470` | `0.6280` |
| `64` | `1.5` | `335188` | `0.5035` | `0.6775` | `0.6654` |
| `128` | `1.5` | `463188` | `0.5280` | `0.6750` | `0.6629` |

For comparison, dense vector retrieval on the same sample:

```text
Recall@20: 0.5195
Recall@100: 0.6740
MRR@20: 0.6544
```

The `active_dims=128, sae_weight=1.5` unified sparse run exceeded dense on
`Recall@20`, `Recall@100`, and `MRR@20` in this SciDocs slice. SQL sidecar
parity also held for this configuration with `0` ranking diffs over `100`
queries.

The cost is clear: SAE postings double with each active-dimension step. The
`128` profile is promising for quality, while `64` may be a better first
systems target because it nearly matches or exceeds dense with half the SAE
postings of `128`.

## Interpretation

The larger validation is now stronger than the first pass suggested:

- `BM25 + SAE` improves BM25 substantially on SciDocs.
- `BM25 + SAE` can match or exceed dense on this SciDocs slice when the SAE
  active-dimension budget is increased.
- SAE adds semantically useful signal because it has low overlap with BM25.
- `sae_weight=1.0` is a stable conservative default, but `1.5` is better for
  the higher-density SAE profiles tested here.

The conclusion is still not "drop vector now." The conclusion is:

> The single sparse engine is promising enough to move from research scripts to
> a more realistic SQL/database prototype. The next research work should focus
> on field-aware BM25, active-dimension cost/quality tradeoffs, and larger
> arxiv-style evaluation before native C index work.

## Next Step

The most valuable next experiments were:

1. Add field-aware lexical dimensions:

   ```text
   bm25:title:<term>
   bm25:abstract:<term>
   sae:<latent_id>
   ```

2. Test `active_dims=64` as the likely systems profile and `active_dims=128`
   as the quality profile.
3. Rerun on arxiv with realistic semantic queries.
4. Evaluate larger training data and `input_dim=768` before deciding whether
   the 256-dimensional truncation is limiting SAE quality.

## Follow-up Experiments

The next experiment batch tested those directions with the same SciDocs 2k
slice unless stated otherwise. The important change is that these results are
now measured against real BEIR qrels, not self-retrieval labels.

### Field-Aware BM25 Dimensions

The prototype now supports lexical dimensions shaped as:

```text
bm25:title:<term>
bm25:body:<term>
sae:<latent_id>
```

The implementation is in:

```text
scripts/research_sae_unified_sparse_eval.py
scripts/research_sae_unified_sql_sidecar.py
scripts/research_sae_gap_analysis.py
scripts/research_sae_weight_sweep.py
```

Field-aware BM25 is mechanically valid and SQL sidecar parity holds, but it did
not improve this SciDocs slice. The best title/body sweep observed:

| profile | title weight | SAE weight | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `8192/64` | `0.5` | `1.5` | `0.5055` | `0.6675` | `0.6621` |
| `8192/128` | `0.5` | `1.5` | `0.5190` | `0.6710` | `0.6614` |

That is below the plain BM25+SAE results from the previous section. The likely
reason is corpus-specific: SciDocs text already starts with title-like text,
and the qrels do not strongly reward title matches over abstract/body matches.
Field-aware dimensions remain useful for production paper tables, but they are
not proven as a universal quality win by this benchmark.

### 768-Dimensional Snowflake Input

The earlier runs truncated Snowflake embeddings to the first `256` dimensions.
The follow-up reran the same SciDocs 2k setup with `input_dim=768`.

Single-source results:

| source | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: |
| dense vector, `768` input | `0.5210` | `0.6880` | `0.6609` |
| SAE `8192/64`, `768` input | `0.5030` | `0.6925` | `0.6418` |
| SAE `8192/128`, `768` input | `0.4975` | `0.6580` | `0.6285` |

Unified BM25+SAE best observed results:

| profile | SAE weight | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `8192/64`, `768` input | `2.0` | `0.5195` | `0.7120` | `0.6824` |
| `8192/128`, `768` input | `1.25` | `0.5130` | `0.6675` | `0.6450` |

This is a stronger result than the 256-dimensional run for the systems profile:
`8192/64` with `sae_weight=2.0` beats the dense `768` baseline on MRR@20 and
Recall@100 while using a single sparse postings surface. The `8192/128` quality
profile regressed in this run, so higher active dimension is not automatically
better without adjusting training or regularization.

The SQL sidecar also matched the `8192/64`, `768` input result:

```text
Recall@20: 0.5195
Recall@100: 0.7120
MRR@20: 0.6824
MRR@100: 0.6844
```

### Query-Adaptive Source Weighting

The fixed global `sae_weight` is simple, but the per-query oracle shows there
is some headroom for learned or heuristic source gating. The new script:

```text
scripts/research_sae_query_weight_oracle.py
```

splits queries into train/test, selects the best fixed weight on the train
split, then compares it to a per-query oracle on the test split.

For `8192/64`, `768` input:

| method | test queries | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| best fixed weight from train, `sae_weight=2.0` | `30` | `0.5633` | `0.7267` | `0.7078` |
| per-query oracle over the same weight grid | `30` | `0.5850` | `0.7133` | `0.7194` |

The oracle improves Recall@20 and MRR@20, but the gap is modest. That makes
query-adaptive gating worth a later learned-ranking experiment, but not a
blocker for the first database-side sparse index design.

### SPLADE SparseEncoder Baseline

The first neural sparse baseline used SentenceTransformers `SparseEncoder`
with:

```text
model: naver/splade-cocondenser-ensembledistil
max_active_dims: 128
device: mps
```

On the same SciDocs 2k slice:

| source | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: |
| SPLADE SparseEncoder | `0.4340` | `0.6105` | `0.5354` |

That is below BM25+SAE and below dense vector retrieval in this setup. This is
not a final judgment on SPLADE; it is only a baseline with one readily available
model and aggressive `128` active-dimension truncation. BGE-M3 sparse scoring
was not run in this pass because the local environment does not currently have
`FlagEmbedding` installed, and installing that dependency should be handled as
a separate, explicit baseline environment step.

### Arxiv Sanity Slice

A 2k arxiv slice was copied from `elm` into `/tmp` for local chain validation:

```text
documents: 2000
embedding dimensions: 256
query labels: self-retrieval only
```

The self-retrieval sanity run returned perfect scores for dense, BM25, SAE, and
BM25+SAE. That proves the scripts can ingest the production arxiv field shape
and vectors, but it does not prove semantic retrieval quality. A real arxiv
evaluation still needs curated queries, click/judgment labels, or another
non-self qrels source.

## Updated Direction

The current best path is:

1. Treat `BM25 + SAE` as a unified sparse impact-index candidate, not merely as
   another Python-side fusion source.
2. Use `Snowflake/snowflake-arctic-embed-m-v2.0` full `768` input for the next
   serious SAE experiments; the 256-dimensional truncation leaves measurable
   quality on the table.
3. Use `8192/64` as the first systems profile. It has better quality/cost
   balance than `8192/128` in the latest run.
4. Keep field-aware BM25 support in the prototype, but do not assume it is a
   universal benchmark win.
5. Defer learned query gating until after the database-side postings layout is
   more stable. The oracle shows headroom, but not enough to complicate the
   first native design.
6. Do not drop dense vector retrieval yet. The result is now strong enough to
   prototype a single sparse engine, but it still needs larger datasets and
   real arxiv-style qrels before replacing vector search.
