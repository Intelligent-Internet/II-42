# ii42 M160A C6-Style Full-Surface Plan

Date: 2026-06-03

## Summary

This run moves the successful C6 training idea onto the M160A Stage-A
representation.

Decision:

- start from M160A Stage-A, not from B8;
- preserve M160/PPLX `1024`-dimensional embeddings and `16384` SAE features;
- use strict preflight before and after row building;
- keep B8 BM25 false-positive handling as an auxiliary data idea, not as the
  checkpoint to continue from.

Run:

```text
ii42-m160a-c6-style-fullsurface-v1
```

Runner:

```bash
scripts/run_ii42_m160a_c6_style_fullsurface_spark.sh
```

## Why Not Continue From B8

B8 fixed the row-surface plumbing and produced nonzero BM25-score rows, but the
full-corpus result did not promote:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3182 | 0.2972 | 0.2257 | 0.1501 |
| B8 BM25+SAE score fusion | 0.3090 | 0.2654 | 0.2001 | 0.1335 |

B8 slightly improved admission over B7 but harmed ranking. Continuing from
that checkpoint risks preserving the wrong calibration.

## C6-Style Transfer

C6 worked because it changed the supervised row surface:

- official non-test qrels;
- dense-hit / SAE-miss rows;
- score-low rows;
- BM25+dense rank target;
- lower self-teacher weight.

For M160A, the same idea must be rebuilt under the corrected row contract:

- no 768-dimensional Snowflake rows;
- no official test qrels in training;
- no dense-only rows in BM25-aware Stage B;
- no silent `source_top_k=100` truncation when the run config says `500`.

## v1 Data Surface

The runner reuses validated B6 exact current rows for small/medium qrels:

```text
/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b6-currentdeep-v1-candidate-rows
/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b6-currentdeep-v1-eval-candidate-rows
```

It then adds streaming BM25+dense rows for larger non-test corpora:

| Role | Dataset | Split | Query cap |
| --- | --- | --- | ---: |
| Train | `msmarco` | `train` | 3000 |
| Train | `hotpotqa` | `train` | 3000 |
| Train | `fever` | `train` | 3000 |
| Train | `dbpedia-entity` | `dev` | 3000 |
| Validation | `msmarco` | `dev` | 500 |
| Validation | `hotpotqa` | `dev` | 500 |
| Validation | `fever` | `dev` | 500 |

Documents are not sampled; the caps only limit supervised query rows.

## Training Defaults

| Parameter | Value |
| --- | ---: |
| Candidate K | 192 |
| Source top K | 500 |
| Dense top K | 500 |
| BM25 top K | 500 |
| Streaming builder device | `cuda` |
| Streaming doc/query batch | `1024 / 32` |
| Feature K | 96 |
| Steps | 9000 |
| Fusion mode | `residual` |
| Initial SAE scale | 1.0 |
| Initial BM25 scale | 0.30 |

Row weights:

| Category | Weight |
| --- | ---: |
| BM25+dense hit | 0.45 |
| Dense hit / miss repair | 4.00 |
| BM25 false positive | 2.00 |
| Score low | 2.50 |
| Not retrieved | 1.50 |

## Acceptance Gate

The run is useful only if it improves the full-corpus continuity gate, not just
local candidate rows.

Minimum gate:

- BM25+SAE must beat B8 on Recall@100, MRR@20, NDCG@10, and MAP@100.
- It should move back toward M130/M150 C6 rather than only matching B-series
  local validation.
- If local validation improves but full-corpus eval does not, the next blocker
  is not another loss-weight tweak; it is official large-corpus row-surface
  depth and final scoring calibration.

## Execution Status

Started on Spark:

```text
session: ii42_m160a_c6_style_fullsurface_v1
log: /home/huoju/leask/logs/ii42_m160a_c6_style_fullsurface_v1.log
run: /home/huoju/leask/runs/ii42-m160a-c6-style-fullsurface-v1
```

Initial preflight passed:

- checkpoint shape: M160/PPLX `1024` input dimensions and `16384` SAE features;
- sampled document/query embeddings are `1024`-dimensional;
- train specs do not use official `test` qrels;
- post-merge preflight will require nonzero BM25 scores.

The first attempt used CPU streaming builder for stability, but was stopped
before rows were produced because `msmarco_train_max3000` would make dense
ranking too slow. The active attempt uses CUDA streaming builder with reduced
`doc/query` batches (`1024 / 32`) to keep the run practical while controlling
memory risk.
