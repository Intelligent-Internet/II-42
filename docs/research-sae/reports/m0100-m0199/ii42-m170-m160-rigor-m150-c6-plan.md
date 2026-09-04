# ii42 M170: M160 Data Rigor + M150 Training Route

Date: 2026-06-05

## Summary

M170 is a clean reset after the M160/M150 review.

The corrected principle is:

- use M160's strict data and dimensional contract;
- use M150's staged training route and loss ideas;
- do not skip the broad Stage-A representation pass.

The earlier Stage-B-only M170 attempt was stopped. It used the M160A checkpoint
and BEIR supervised rows, but it did not rerun the broad corpus stage with
Wikipedia/arXiv/PubMed. That is not the intended M170 route.

Correct first run:

```text
ii42-m170a-broad-m150loss-v1
```

Correct first runner:

```bash
scripts/run_ii42_m170a_broad_m150loss_spark.sh
```

## Non-Negotiable Contract

M170 must fail fast instead of silently falling back to a convenient surface.

Hard requirements:

- model family: `perplexity-ai/pplx-embed-v1-0.6B`;
- dense embedding dimension: `1024`;
- SAE latent features: `16384`;
- M170A starts from scratch, not from M150/M160/B12;
- no 768-dimensional Snowflake rows;
- no official `test` qrels in training rows;
- no official BEIR test query text in Stage-A query-style rows;
- BEIR corpus documents may be used as unsupervised representation data;
- Wikipedia, arXiv, and PubMed neutral sources must be present;
- M170B/C can start only after M170A has a valid checkpoint and manifest;
- promotion is based on official full-corpus evaluation, not local candidate
  metrics.

## Why This Reset Exists

M150 was strong because it found the right supervised target: dense-miss,
score-low, and BM25+dense admission/ranking repair. But its data execution was
not clean enough to productize directly.

M160 fixed the broad-data execution gap: BEIR + Wikipedia/arXiv/PubMed were
actually consumed with PPLX `1024`-dimensional embeddings. But later M160 B/C
did not fully reproduce M150's sharp correction target.

M170 therefore keeps M160's data rigor and replays the M150 route:

1. broad Stage A representation and coverage;
2. broad Stage B BM25-aware ranking;
3. narrow C6-style official dense-miss correction;
4. official full-corpus gate.

## Stage Layout

### M170A: Broad Stage A

M170A is the required first stage.

Data:

- full official BEIR corpus documents;
- BEIR query-style rows with official `qrels/test.tsv` query ids removed;
- neutral Wikipedia documents and query-style rows;
- neutral arXiv documents and query-style rows;
- neutral PubMed documents and query-style rows;
- optional external BEIR non-heldout/proxy rows from the existing M160 split
  root, if present.

Training method:

- M160 broad manifest builder;
- M150-style Stage-A coverage auxiliary;
- PPLX `1024` input;
- SAE `16384` features;
- active `k=96`;
- no final ranking/product claim from this stage alone.

Runner:

```bash
scripts/run_ii42_m170a_broad_m150loss_spark.sh
```

Expected manifest checks:

| Source group | Minimum |
| --- | ---: |
| BEIR corpus docs | full materialized corpus |
| Wikipedia docs | `>= 190k` |
| arXiv docs | `>= 190k` |
| PubMed docs | `>= 190k` |
| Neutral query-style rows | `>= 45k` per neutral source |

The M170A manifest writes filtered BEIR query files under:

```text
/home/huoju/leask/runs/m170a-broad-m150loss-manifest/beir_non_test_queries
```

### M170B: Broad C6-Style Stage B

M170B starts from:

```text
/home/huoju/leask/runs/ii42-m170a-broad-m150loss-v1/bm25sae_stagea_best.pt
```

It rebuilds a strict BM25/dense/SAE/qrels surface from full-corpus BEIR
documents with M150/C6-style supervision:

- qrel positives;
- BM25 top-k;
- dense top-k;
- SAE top-k from the M170A checkpoint;
- BM25+dense teacher ranks;
- BM25+SAE source ranks;
- dense-hit / SAE-miss rows;
- SAE-positive lost-by-fusion rows;
- BM25-positive lost-by-fusion rows;
- high-BM25 false positives;
- score-low / rank-regression rows.

Runner:

```bash
scripts/run_ii42_m170_m160rigor_m150c6_spark.sh
```

This runner is now a Stage-B runner despite its historical filename.

### M170C: Official Dense-Miss Correction

M170C starts only after M170B improves or matches the current B12 continuity
gate. It applies the narrower M150 C6 correction:

- official non-test qrels only;
- dense-miss and score-low row oversampling;
- BM25+dense rank target;
- low BM25+SAE self-imitation weight;
- optional frozen document SAE if the remaining issue is query-side scoring.

### M170G: Official Gate

Every promoted checkpoint must run:

- official BEIR full-corpus per-dataset matrix;
- continuity matrix for comparability with M130/M150/M160;
- SAE-only, BM25, dense, BM25+dense, BM25+SAE score fusion, and RRF rows;
- physical matrix: SAE postings/query, accumulator entries/query, latency, and
  payload size where available;
- miss taxonomy: dense-hit/SAE-miss, candidate-hit/score-low, BM25 false
  positive, SAE positive lost by fusion.

## M170A Defaults

| Parameter | Value |
| --- | ---: |
| `epochs` | `3` |
| `batch_size` | `384` |
| `feature_k` | `96` |
| `n_features` | `16384` |
| `topk_iters` | `24` |
| `query_repeat` | `2` |
| `wikipedia_repeat` | `16` |
| `arxiv_repeat` | `16` |
| `pubmed_repeat` | `16` |
| coverage recall | `0.05` |
| coverage complement CE | `0.10` |
| coverage dense KL | `0.02` |

## M170B Defaults

Core surface:

| Parameter | Value |
| --- | ---: |
| `candidate_k` | `192` |
| `source_top_k` | `500` |
| `dense_top_k` | `500` |
| `bm25_top_k` | `500` |
| `sae_top_k` | `500` |
| `feature_k` | `96` |
| `doc_active_k` / `query_active_k` while building rows | `96 / 96` |
| `max_df_ratio` | `0.12` |
| `bm25_max_df_ratio` | `0.30` |
| fusion mode | `residual` |

Builder teacher and row weights:

| Signal | Weight |
| --- | ---: |
| dense teacher score | `0.80` |
| BM25+dense rank target | `0.75` |
| BM25 rank target | `0.05` |
| dense-hit / SAE-miss | `4.00` |
| BM25 false positive | `2.00` |
| score-low | `2.50` |
| rank regression | `4.00` |
| not retrieved | `1.50` |

Training loss:

| Loss | Weight |
| --- | ---: |
| recall | `1.35` |
| single-positive CE | `0.15` |
| multi-positive CE | `1.00` |
| teacher KL | `0.65` |
| complement | `0.75` |
| final hard pairwise | `0.80` |
| SAE hard pairwise | `0.80` |
| row-weighted admission | `1.00` |
| BM25 false positive | `0.50` |
| SAE lost preservation | `0.80` |
| reconstruction | `0.02` |
| k-budget | `0.00` |
| scale regularization | `0.03` |

## Acceptance Gates

M170A is healthy only if:

1. manifest summary proves BEIR + Wikipedia/arXiv/PubMed were consumed;
2. BEIR test query text was filtered from query-style rows;
3. candidate eval does not regress badly versus M160A Stage A;
4. source counters show nonzero neutral and BEIR exposure.

M170B/C is worth continuing only if it satisfies at least one gate:

1. it beats M160 B12 on the continuity full-corpus surface in at least three of
   four metrics without a severe fourth-metric regression;
2. it improves `NDCG@10`/top-rank quality while preserving the M160 B12
   recall/MAP advantage over dense;
3. it reproduces the M150 C6 direction under strict M170A/PPLX rows, proving the
   old result was not an artifact.

If local candidate metrics improve but full-corpus metrics do not, do not tune
weights blindly. Inspect:

- row category distribution;
- SAE-only transfer;
- BM25+SAE source rank regression;
- dense-hit/SAE-miss coverage;
- score-low rows where candidate admission is correct but ranking fails.

## Execution State

Local setup:

```text
commit before M170: 9f23a87 Record M160 frontier before M170 reset
tag: m160-frontier-before-m170
```

The Stage-B-only M170 attempt was stopped on 2026-06-05. Correct next command:

```bash
scripts/run_ii42_m170a_broad_m150loss_spark.sh
```
