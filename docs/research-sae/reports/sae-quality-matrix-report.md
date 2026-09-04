# SAE Quality Matrix Report

Date: 2026-05-11

## Objective

Use the main branch BEIR quality-matrix metric set to evaluate the current
most promising SAE path:

```text
base embedding: Snowflake/snowflake-arctic-embed-m-v2.0
embedding dimensions: 768
SAE latent dimensions: 8192
SAE active dimensions: 64
SAE score mode: normalized_idf_dot
BM25 weight: 1.0
SAE weight: 2.0
```

The production PG18 quality matrix is a full-corpus BM25 engine comparison.
This SAE run is intentionally a sampled neural-retrieval research matrix
because dense embedding generation and SAE training are not feasible across
the full 15-dataset BEIR corpus in one local pass.

## Scope

```text
datasets: scifact, scidocs, nfcorpus, arguana, fiqa
sample target: 2000 docs, 100 queries
qrels handling: include all positive documents for selected queries
top_k: 100
device: mps
```

The sampled corpus is not directly comparable to the official full-corpus
PG18 BM25 table. The comparison that matters in this report is within each
sampled dataset: `dense`, `BM25`, `SAE`, and `BM25+SAE` all see the same
documents and qrels.

## Command

```bash
python3 scripts/benchmark_sae_quality_matrix.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --datasets-dir /Volumes/Betty/Tmp/ii42_dataset_cache/beir_official \
  --work-dir /tmp/ii42_sae_quality_matrix \
  --output /tmp/ii42_sae_quality_matrix/sae_quality_matrix.json \
  --embedding-dim 768 \
  --latent-dims 8192 \
  --active-dims 64 \
  --sae-weight 2.0 \
  --max-docs 2000 \
  --max-queries 100 \
  --epochs 20 \
  --device mps
```

The script writes:

```text
/tmp/ii42_sae_quality_matrix/sae_quality_matrix.json
/tmp/ii42_sae_quality_matrix/sae_quality_matrix.md
/tmp/ii42_sae_quality_matrix/sae_weight_sweep_summary.json
```

## Fixed-Weight Result

This table uses the fixed configuration from the current best SciDocs run:
`bm25_weight=1.0`, `sae_weight=2.0`.

| Dataset | Dense R@100 | BM25 R@100 | SAE R@100 | BM25+SAE R@100 | Dense MRR@20 | BM25+SAE MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `1.0000` | `0.9700` | `1.0000` | `1.0000` | `0.5710` | `0.5126` |
| `fiqa` | `0.9211` | `0.7578` | `0.9126` | `0.9131` | `0.7726` | `0.7472` |
| `nfcorpus` | `0.3635` | `0.2660` | `0.3590` | `0.3763` | `0.7058` | `0.6850` |
| `scidocs` | `0.6920` | `0.5720` | `0.7025` | `0.7040` | `0.7120` | `0.6624` |
| `scifact` | `0.9800` | `0.9517` | `0.9500` | `0.9800` | `0.7965` | `0.8102` |

Mean over the five sampled datasets:

| Source | Mean R@100 | Mean NDCG@10 |
| --- | ---: | ---: |
| `BM25` | `0.7035` | `0.5031` |
| `SAE` | `0.7848` | `0.5674` |
| `dense` | `0.7913` | `0.6190` |
| `BM25+SAE` | `0.7947` | `0.6036` |

## Weight Sweep

The fixed `sae_weight=2.0` is not uniformly optimal. A quick cached sweep over
`0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 2.5, 3.0` produced:

| Dataset | Best R@100 Weight | Best R@100 | Best MRR@20 Weight | Best MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | `0.5` | `1.0000` | `2.0` | `0.5126` |
| `fiqa` | `1.5` | `0.9192` | `2.0` | `0.7472` |
| `nfcorpus` | `1.25` | `0.3785` | `1.0` | `0.6897` |
| `scidocs` | `3.0` | `0.7125` | `2.0` | `0.6624` |
| `scifact` | `0.5` | `0.9800` | `0.75` | `0.8401` |

## Interpretation

The route remains promising, especially as a recall-oriented single sparse
engine candidate:

- `BM25+SAE` beats BM25 on every sampled dataset.
- `BM25+SAE` matches or beats dense on Recall@100 in `arguana`, `nfcorpus`,
  `scidocs`, and `scifact`.
- `BM25+SAE` trails dense on first-page ranking for `arguana`, `fiqa`,
  `nfcorpus`, and `scidocs`, even when recall is competitive.
- `scifact` is the strongest ranking win: BM25+SAE improves MRR@20 over dense,
  and lower SAE weight improves it further.

The main weakness is ranking calibration. The same global SAE weight does not
fit every dataset. That points toward source gating or query-adaptive weighting
as a real next step, not just a nice-to-have.

## Decision

Continue the `Snowflake 768 + SAE 8192/64` path as the main systems profile.
It is strong enough to justify a database-side sparse prototype, but not strong
enough to remove dense vector retrieval yet. The next quality milestone should
be a larger matrix with either:

1. per-dataset tuned weights, to test upper-bound quality;
2. a learned or heuristic query gate, to test whether one deployable policy can
   approximate the upper bound;
3. a real arxiv/pubmed semantic query set, because BEIR samples are still only
   proxies for the production AI-search workload.
