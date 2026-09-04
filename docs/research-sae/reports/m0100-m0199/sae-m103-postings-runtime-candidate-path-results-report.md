# SAE M103 Postings Runtime Candidate Path Report

Date: 2026-05-22

## Decision

M103 moves beyond the M102 candidate-surface approximation. It runs
postings-driven candidate generation over the full M97/M81 eval document set:
`30059` documents, `886` queries, `2.80M` BM25 postings, and `15.39M`
`m96_k512` SAE postings.

The result is useful but not promotable as-is. The exported M101 scorer runs
over full-corpus generated candidate pools, but it does not transfer cleanly
from the M98 candidate-surface distribution. On this postings-generated
distribution, fixed `BM25+SAE w2` beats the M101 runtime ranker across the
matrix.

This changes the immediate next step: do not push M101 directly into the
read-only PostgreSQL payload path. First retrain or recalibrate Stage-B on
postings-generated candidate rows.

## Inputs

- Corpus root:
  `/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval`
- Latent run: `m96_k512`
- Exported scorer:
  `/home/huoju/leask/runs/m101-batched-stage-b-ranker-v0/m101_stage_b_ranker_export.json`
- Spark output:
  `/home/huoju/leask/runs/m103-postings-runtime-candidate-path-v0`
- Local artifact mirror:
  `results/sae/m103-postings-runtime-candidate-path`

## Data

| Field | Value |
| --- | ---: |
| Documents | `30059` |
| Queries | `886` |
| Query contexts | `886` |
| Doc latents | `30059` |
| Query latents | `886` |
| BM25 terms | `76361` |
| BM25 postings | `2800748` |
| SAE dims | `7184` |
| SAE postings | `15390208` |
| Load plus eval seconds | `642.93` |

## Baseline

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `full_bm25` | 0.4063 | 0.4513 | 0.3434 | 0.2348 |

The full BM25 baseline is lexical-only over the same `30059` documents. It is
useful as a sanity check: generated hybrid candidate rows must beat this on
ranking quality without exploding candidate cost.

## Best Current Points

| Config | Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean Cand | SAE Gen Postings | Query ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `d8_p16_bm25100` | `fixed_bm25_sae_w2` | 0.4231 | 0.5138 | 0.4081 | 0.2840 | 204.3 | 128.0 | 12.122 |
| `d8_p16_bm25100` | `m101_runtime_ranker` | 0.4154 | 0.5134 | 0.3973 | 0.2747 | 204.3 | 128.0 | 12.122 |
| `d16_p32_bm25100` | `fixed_bm25_sae_w2` | 0.4405 | 0.5157 | 0.4096 | 0.2882 | 562.6 | 511.9 | 20.224 |
| `d16_p32_bm25100` | `m101_runtime_ranker` | 0.4319 | 0.4813 | 0.3764 | 0.2600 | 562.6 | 511.9 | 20.224 |
| `d32_p64_bm25100` | `fixed_bm25_sae_w2` | 0.4635 | 0.5168 | 0.4111 | 0.2930 | 1930.7 | 2047.4 | 57.404 |
| `d32_p64_bm25100` | `m101_runtime_ranker` | 0.4361 | 0.4733 | 0.3675 | 0.2549 | 1930.7 | 2047.4 | 57.404 |

## Interpretation

The good news:

- Postings-driven candidate generation works over the full `30059` document
  corpus.
- Even the compact `d8_p16_bm25100` point beats full BM25 on NDCG@10 and
  MAP@100 with only about `204` candidate docs and `128` SAE generation
  postings per query.
- Increasing SAE candidate breadth improves Recall@100, but candidate count
  grows quickly.

The blocker:

- The M101 residual scorer was trained on the M98/M81 candidate surface where
  the candidate pool had mean `118.7` docs and a very different score/rank
  distribution.
- In M103, the generated pools range from about `141` to `1931` candidates.
  Rank/margin features are therefore not distribution-compatible with the M101
  training rows.
- The fixed score `bm25 + 2 * sae` is currently more robust on the new
  postings-generated distribution.

## Decision

M103 promotes the postings-driven simulator as the correct evaluation harness
for the next Stage-B step. It does not promote the M101 exported ranker for
read-only SQL execution.

Next work should be M104:

1. Generate postings-driven training rows with the M103 candidate generator.
2. Train a Stage-B residual ranker on those rows, not on the old M98 candidate
   surface.
3. Keep `fixed_bm25_sae_w2` as the baseline to beat, not just M101/M102.
4. Add candidate-count/posting-cost regularization so the model does not only
   win by preferring larger candidate pools.
5. Re-run M103's full-corpus cost/quality matrix with the newly trained scorer.

