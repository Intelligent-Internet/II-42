# Snowflake SAE MPS Prototype Report

Date: 2026-05-07

## Objective

Train a first larger SAE-over-Snowflake prototype on the local Mac MPS backend
and validate the downstream retrieval path:

1. SAE latent sparse retrieval with `normalized_idf_dot`.
2. Exact PostgreSQL SQL-sidecar parity.
3. Dense/BM25/SAE hybrid contribution.

This is still a small SciFact run, not a production-quality model. The goal is
to validate the prototype shape and find a reasonable next default.

## Environment

- PyTorch: `2.9.1`
- MPS available: `true`
- MPS built: `true`
- Dense model used by the prepared data:
  `Snowflake/snowflake-arctic-embed-m-v2.0`
- Dense input shape used by the prototype: first `256` dimensions,
  L2-normalized

## Training Configuration

```text
dataset: BEIR SciFact sample
documents: 400
queries: 40
latent_dims: 8192
active_dims: 32
score_mode: normalized_idf_dot
epochs: 20
batch_size: 128
seed: 31
device: mps
```

The resulting model artifact was `16 MB` for this configuration. The document
latent postings are deterministic top-32 activations:

```text
posting_count = documents * active_dims = 400 * 32 = 12800
```

## Command

```bash
python3 scripts/research_sae_sweep.py \
  --documents /tmp/ii42_sae_scifact/data/documents.jsonl \
  --queries /tmp/ii42_sae_scifact/data/queries.jsonl \
  --output-dir /tmp/ii42_sae_scifact_mps_8192 \
  --latent-dims 8192 \
  --active-dims 32 \
  --score-modes normalized_idf_dot \
  --epochs 20 \
  --batch-size 128 \
  --top-k 100 \
  --seed 31 \
  --device mps
```

## Single-Source Retrieval

| source | Recall@20 | Recall@100 | MRR@20 | MRR@100 |
| --- | ---: | ---: | ---: | ---: |
| dense vector | `0.9750` | `1.0000` | `0.8942` | `0.8945` |
| BM25 | `0.9450` | `0.9750` | `0.8983` | `0.8993` |
| SAE `8192/32` | `0.9250` | `0.9750` | `0.7954` | `0.7972` |

Compared with the earlier `256/16` prototype, the larger SAE is substantially
better:

```text
old SAE 256/16 normalized_idf_dot Recall@20: 0.8100
new SAE 8192/32 normalized_idf_dot Recall@20: 0.9250
```

The larger latent space also activates many more distinct dimensions:

```text
unique latent dimensions with postings: 1362
mean active dimensions per document: 32
```

## SQL Sidecar Parity

The SQL sidecar now supports the same score modes as the offline sparse
retriever. For `normalized_idf_dot`, document weights are normalized by the
stored latent norm before insertion into the sidecar postings table.

SQL sidecar result:

```text
documents: 400
postings: 12800
dimensions: 1362
Recall@20: 0.9250
Recall@100: 0.9750
MRR@20: 0.7954
MRR@100: 0.7972
```

This matches the offline SAE result exactly.

## Hybrid Contribution

The prototype evaluator normalizes each source by its per-query max score and
then applies source weights.

Equal-weight result:

| combo | Recall@20 | Recall@100 | MRR@20 |
| --- | ---: | ---: | ---: |
| dense + BM25 | `0.9750` | `1.0000` | `0.9225` |
| BM25 + SAE | `0.9750` | `1.0000` | `0.9297` |
| dense + BM25 + SAE | `0.9750` | `1.0000` | `0.9175` |

This is a useful signal: on this small slice, `BM25 + SAE` can match the
recall of `dense + BM25` and slightly improve MRR. However, equal-weight
three-way fusion is worse than `dense + BM25`, so SAE should not be added at
full weight by default.

SAE weight sweep for `dense + BM25 + SAE`:

| SAE weight | Recall@20 | Recall@100 | MRR@20 |
| ---: | ---: | ---: | ---: |
| `0.10` | `0.9750` | `1.0000` | `0.9238` |
| `0.25` | `0.9750` | `1.0000` | `0.9225` |
| `0.50` | `0.9750` | `1.0000` | `0.9217` |
| `0.75` | `0.9750` | `1.0000` | `0.9217` |
| `1.00` | `0.9750` | `1.0000` | `0.9175` |
| `1.25` | `0.9750` | `1.0000` | `0.9154` |

For the next prototype default, use:

```text
dense_weight = 1.0
bm25_weight = 1.0
sae_weight = 0.10
```

This keeps SAE as a small semantic sparse assist instead of letting it dominate
the already strong dense/BM25 pair.

## Interpretation

The larger SAE confirms that capacity matters. `latent_dims=8192` and
`active_dims=32` are much more promising than the first tiny sweep. The
vectorless path is also worth testing further because `BM25 + SAE` matched
`dense + BM25` recall on this sample.

Do not conclude that dense vector retrieval can be removed yet. This sample is
small and SciFact is lexically strong. The next validation must use a larger
corpus and harder semantic queries, then measure unique relevant hits at
candidate budgets `20`, `50`, and `100`.

## Next Step

Run the same `8192/32` configuration on a larger arxiv or SciDocs sample. If
`BM25 + SAE` continues to match dense recall, the next database prototype
should be a single unified sparse postings table with lexical and SAE latent
dimensions in one scorer.
