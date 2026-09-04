# SAE M110-M111 Full-Corpus Index Results Report

Date: 2026-05-23

## Decision

M110 and M111 separate candidate-set success from full-corpus index success.
The result is mixed but useful:

- M109 candidate-set training works and can match dense candidate ranking.
- Full-corpus sparse-index evaluation is much harder.
- SAE alone is still below BM25 on full corpus.
- BM25+SAE fusion improves BM25 substantially, but remains below dense.
- Full-corpus hard-negative refresh improves refreshed candidate-set metrics,
  but worsens actual full-corpus SAE retrieval.

Therefore, the next model step must not be another candidate-row fine-tune. The
next step should train against a full-corpus/posting-aware objective or add a
much stronger anti-fanout/background negative term. Candidate-bank metrics are
now diagnostic only; M110 full-corpus metrics are the promotion gate.

## M110 Full-Corpus Index Evaluation

Script:

```text
scripts/research_sae_m110_full_corpus_index_eval.py
```

Evaluation corpus:

```text
/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval
```

Data:

- Documents with embeddings: `25,863`
- Queries with embedding and positive relevance in the embedded doc set: `823`
- Retrieval depth: `top_k=100`

The script evaluates:

- Dense full-corpus dot-product retrieval.
- SAE full-corpus sparse inverted-index retrieval.
- Simple BM25 full-corpus inverted-index retrieval.
- BM25+SAE reciprocal-rank fusion.
- BM25+SAE per-query min-max score fusion.

## M110 Checkpoint Comparison

| Checkpoint | Method | Recall@10 | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Baseline | BM25 | 0.2442 | 0.3693 | 0.4463 | 0.3216 | 0.2002 |
| Baseline | Dense | 0.2869 | 0.4388 | 0.5044 | 0.3844 | 0.2584 |
| M109 4096/k64 | SAE | 0.2143 | 0.3550 | 0.4103 | 0.2859 | 0.1758 |
| M109 4096/k64 | BM25+SAE score | 0.2673 | 0.4146 | 0.4894 | 0.3610 | 0.2314 |
| M109 8192/k64 | SAE | 0.2297 | 0.3647 | 0.4103 | 0.2988 | 0.1862 |
| M109 8192/k64 | BM25+SAE score | 0.2718 | 0.4241 | 0.4907 | 0.3634 | 0.2344 |
| M109 rank-heavy | SAE | 0.2230 | 0.3362 | 0.4082 | 0.2829 | 0.1755 |
| M109 rank-heavy | BM25+SAE score | 0.2749 | 0.4156 | 0.4968 | 0.3580 | 0.2277 |

Interpretation:

- SAE alone is not yet a full-corpus replacement for BM25 or dense.
- BM25+SAE is consistently better than BM25, so SAE is adding useful semantic
  evidence.
- Dense still wins the full-corpus ranking matrix.
- M109's candidate-set win over dense does not directly transfer to full-corpus
  index retrieval.

## Fusion Sweep

For the M109 rank-heavy checkpoint, score-fusion `SAE weight` was swept while
BM25 weight stayed `1.0`.

| SAE Weight | Recall@10 | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.25 | 0.2596 | 0.4135 | 0.4722 | 0.3441 | 0.2198 |
| 0.50 | 0.2685 | 0.4159 | 0.4824 | 0.3552 | 0.2271 |
| 0.75 | 0.2710 | 0.4163 | 0.4958 | 0.3601 | 0.2312 |
| 1.00 | 0.2749 | 0.4156 | 0.4968 | 0.3580 | 0.2277 |
| 1.50 | 0.2642 | 0.4156 | 0.4684 | 0.3422 | 0.2183 |
| 2.00 | 0.2487 | 0.4153 | 0.4560 | 0.3283 | 0.2119 |

Fusion helps, but score calibration alone does not reach dense.

## M111 Hard-Negative Refresh

Script:

```text
scripts/research_sae_m111_full_corpus_hard_negative_refresh.py
```

M111 builds refreshed candidate rows from full-corpus sources:

- qrel positives,
- dense full-corpus top candidates,
- BM25 full-corpus top candidates,
- SAE full-corpus top candidates.

Generated roots:

```text
/home/huoju/leask/runs/m111-full-corpus-hard-negatives/m111_m107_train_refresh
/home/huoju/leask/runs/m111-full-corpus-hard-negatives/m111_m97_eval_refresh
```

Rows:

- M107 train refresh: `3,683`
- M97 eval refresh: `823`

Unlike the old candidate rows, the refreshed rows do not place positives at
rank zero by construction. The row-order diagnostic is much lower:

- M111 eval `row_order_leak_check hit@1`: `0.3135`
- Old M109 eval `row_order_leak_check hit@1`: `1.0000`

## M111 Candidate-Set Result

M111 candidate-set training on refreshed hard negatives produced a strong
candidate-set result:

| Method | hit@1 | hit@10 | MRR@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5407 | 0.7849 | 0.6235 | 0.6278 |
| Dense | 0.6112 | 0.8190 | 0.6835 | 0.6856 |
| M111 model | 0.6112 | 0.9004 | 0.7119 | 0.7149 |

This is a real improvement on the refreshed candidate surface.

## M111 Full-Corpus Regression

When the M111 checkpoint is exported back to full-corpus sparse-index
evaluation, it regresses:

| Method | Recall@10 | Recall@100 | MRR@10 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.2442 | 0.3693 | 0.4463 | 0.3216 | 0.2002 |
| Dense | 0.2869 | 0.4388 | 0.5044 | 0.3844 | 0.2584 |
| M111 SAE | 0.2060 | 0.3247 | 0.3462 | 0.2475 | 0.1539 |
| M111 BM25+SAE RRF | 0.2697 | 0.4132 | 0.4867 | 0.3532 | 0.2178 |
| M111 BM25+SAE score | 0.2675 | 0.4109 | 0.4656 | 0.3432 | 0.2160 |

This proves the candidate surface is still too narrow. M111 learns to solve
the refreshed rows, but it does not learn a better global sparse posting
geometry.

## Interpretation

The core blocker is not whether the model can optimize a candidate set. It can.
The blocker is whether the learned atom geometry suppresses the wrong
full-corpus postings while preserving semantic recall.

The failure mode is visible in diagnostics:

| Checkpoint | SAE Postings Touched / Query | SAE Accumulators / Query |
| --- | ---: | ---: |
| M109 8192/k64 | 60,370 | 23,052 |
| M111 refreshed | 78,156 | 25,456 |

M111 improves candidate rows but opens more full-corpus postings and retrieves
more wrong documents. That is the opposite of what a product index needs.

## Next Step

M112 should train with a full-corpus-aware negative objective, not just
candidate-row hard negatives:

1. Keep the M111 refreshed candidate rows because they are a cleaner diagnostic
   surface.
2. Add sampled background negatives from outside dense/BM25/SAE top candidates.
3. Add posting/fanout regularization based on atom document frequency.
4. Select checkpoints on M110 full-corpus metrics, not candidate-set metrics.
5. Keep BM25+SAE fusion as the product target, but do not tune fusion until the
   SAE full-corpus index stops regressing.

Promotion gate for the next checkpoint:

- SAE alone must at least beat BM25 on `Recall@100` or `NDCG@10`, or
- BM25+SAE must beat dense on at least one ranking metric without losing
  `Recall@100`, and
- postings touched / query must not increase versus M109 8192/k64.
