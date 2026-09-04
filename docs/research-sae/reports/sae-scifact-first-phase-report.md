# SAE Latent Sparse Retrieval SciFact First-Phase Report

Date: 2026-05-07

## Objective

This run moves the SAE-over-dense-embeddings prototype from synthetic and
self-retrieval checks to a small real-qrels benchmark. The question is still
research-level:

> Can TopK SAE latent activations over Snowflake dense embeddings provide a
> useful sparse retrieval signal beside BM25 and dense vector retrieval?

The goal is not to beat the production hybrid stack yet. The goal is to
measure recall, complementarity, and SQL feasibility before considering any
native index work.

## Dataset

Source: BEIR SciFact test split.

Sample used in this run:

- documents: `400`
- queries: `40`
- qrels: positive test labels for the selected queries
- dense model: `Snowflake/snowflake-arctic-embed-m-v2.0`
- query encoding: `query: ` prefix
- embedding shape: first `256` dimensions, then L2-normalized
- random seed: `31`

The sample intentionally includes every relevant document for the selected
queries, then fills the remaining document budget by seeded random sampling.
This keeps the run small enough for fast iteration while preserving meaningful
qrels.

## Commands

Prepare BEIR SciFact data:

```bash
python3 scripts/research_sae_prepare_beir.py \
  --dataset scifact \
  --cache-dir /tmp/beir \
  --output-dir /tmp/ii42_sae_scifact/data \
  --max-docs 400 \
  --max-queries 40 \
  --batch-size 16 \
  --seed 31
```

Run the SAE sweep:

```bash
python3 scripts/research_sae_sweep.py \
  --documents /tmp/ii42_sae_scifact/data/documents.jsonl \
  --queries /tmp/ii42_sae_scifact/data/queries.jsonl \
  --output-dir /tmp/ii42_sae_scifact/sweep \
  --latent-dims 128,256 \
  --active-dims 8,16 \
  --score-modes idf_dot,normalized_idf_dot \
  --epochs 10 \
  --batch-size 128 \
  --top-k 100 \
  --seed 31
```

Validate exact SQL sidecar parity for the best `idf_dot` run:

```bash
python3 scripts/research_sae_sql_sidecar.py \
  --dsn 'host=/tmp port=55432 dbname=postgres' \
  --schema sae_research_scifact \
  --run-id scifact-latent256-active16 \
  --doc-latents /tmp/ii42_sae_scifact/sweep/latent256_active16/doc_latents.jsonl \
  --query-latents /tmp/ii42_sae_scifact/sweep/latent256_active16/query_latents.jsonl \
  --qrels /tmp/ii42_sae_scifact/data/qrels.jsonl \
  --output-dir /tmp/ii42_sae_scifact/sql_sidecar \
  --top-k 100 \
  --reset
```

## Baselines

| source | Recall@20 | Recall@100 | MRR@20 | MRR@100 |
| --- | ---: | ---: | ---: | ---: |
| dense vector | `0.9750` | `1.0000` | `0.8942` | `0.8945` |
| simple BM25 | `0.9450` | `0.9750` | `0.8983` | `0.8993` |

On this small SciFact slice, both dense and lexical baselines are very strong.
That makes this a useful sanity benchmark: a weak SAE run cannot hide behind an
easy synthetic qrels setup.

## SAE Sweep

| run | score | Recall@20 | Recall@100 | MRR@20 | dense overlap@20 | BM25 overlap@20 | unique dims |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| latent128_active8 | idf_dot | `0.5475` | `0.8375` | `0.2048` | `0.1963` | `0.1425` | `53` |
| latent128_active8 | normalized_idf_dot | `0.5150` | `0.8950` | `0.2217` | `0.1825` | `0.1387` | `53` |
| latent128_active16 | idf_dot | `0.7450` | `0.9500` | `0.4248` | `0.2887` | `0.1788` | `74` |
| latent128_active16 | normalized_idf_dot | `0.7700` | `0.9625` | `0.5139` | `0.2812` | `0.1975` | `74` |
| latent256_active8 | idf_dot | `0.5525` | `0.8900` | `0.3479` | `0.2375` | `0.1525` | `82` |
| latent256_active8 | normalized_idf_dot | `0.6225` | `0.9025` | `0.3817` | `0.2275` | `0.1612` | `82` |
| latent256_active16 | idf_dot | `0.8350` | `0.9075` | `0.4210` | `0.3525` | `0.2225` | `117` |
| latent256_active16 | normalized_idf_dot | `0.8100` | `0.9200` | `0.4890` | `0.3250` | `0.2213` | `117` |

## SQL Sidecar Result

The SQL sidecar loaded the best `latent256_active16` run as exact sparse
postings:

- documents: `400`
- postings: `6400`
- dimensions with postings: `117`
- SQL sparse latent Recall@20: `0.8350`
- SQL sparse latent Recall@100: `0.9075`
- SQL sparse latent MRR@20: `0.4210`

The SQL result matches the offline `idf_dot` score exactly. This validates the
sidecar query shape:

```sql
WITH query_dims AS (
    SELECT *
    FROM unnest(
        $1::integer[],
        $2::real[]
    ) AS q(dim_id, query_weight)
),
candidate_scores AS (
    SELECT
        p.document_id,
        SUM(q.query_weight * p.weight * s.idf)::real AS score
    FROM query_dims AS q
    JOIN sparse_latent_postings AS p
      ON p.run_id = $3
     AND p.dim_id = q.dim_id
    JOIN sparse_latent_stats AS s
      ON s.run_id = p.run_id
     AND s.dim_id = p.dim_id
    GROUP BY p.document_id
)
SELECT document_id, score
FROM candidate_scores
ORDER BY score DESC, document_id
LIMIT $4;
```

One implementation fix came out of this check: PostgreSQL databases created as
`SQL_ASCII` can cause psycopg to return text values as `bytes`. The sidecar
script now normalizes fetched document ids before metric evaluation.

## Interpretation

The current TopK SAE signal is not production-ready as a standalone retrieval
source. Dense vector and BM25 are still substantially stronger on Recall@20.

The run is still useful because it shows three concrete things:

- Increasing active dimensions from `8` to `16` has a large positive effect.
- SAE rankings are not duplicates of dense or BM25 rankings; top-20 overlap is
  low to moderate.
- Exact SQL sparse scoring has the same semantics as the offline Python scorer.

The low overlap means SAE may still be valuable as a third candidate source for
reranker input, even before it is strong enough to replace any existing source.
The next stage should therefore measure hybrid contribution, not only isolated
SAE recall.

## Next Experiments

1. Run a larger SciFact sweep with `latent_dims=512,1024` and
   `active_dims=16,32,64`.
2. Increase the training corpus size before judging SAE quality. A `400`
   document training set is too small for stable latent concept learning.
3. Add hybrid fusion evaluation: dense only, BM25 only, dense+BM25,
   dense+SAE, BM25+SAE, and dense+BM25+SAE.
4. Track unique relevant hits contributed by SAE at candidate budgets
   `20`, `50`, and `100`.
5. After SciFact, repeat on SciDocs and an arxiv slice with non-self qrels or
   curated query judgments.

## Decision

Continue SAE-over-dense-embeddings research, but do not start native C index
work yet. The right next implementation step is hybrid contribution analysis
over real qrels.
