# II-42 M405 Overlap-Anchored Query Correction Report

Date: 2026-06-27

## Goal

M405 follows the M403-A structural-space query correction route, not the M404
shared dense-space adapter route.  Document postings remain deterministic and
frozen.  The learned component only corrects query structural coordinates.

The new loss adds two preservation terms:

- dense-pool anchor: corrected scores on exact dense top-k candidates should
  stay close to deterministic structural scores;
- query anchor: corrected query coordinates and tail sketches should stay close
  to deterministic query coordinates.

Training is qrels-free.  Qrels are held out for evaluation only.

## Runs

Remote host: `spark-2`

Remote working directory:
`/home/huoju/leask/runs/mteb-m393-tail-bm25-v1`

Remote output root:
`/home/huoju/leask/runs/ii42-m405-overlap-anchored-query-correction-v1`

All runs were CPU-only and low priority.  The four-task run used two BLAS
threads because another CUDA training job was active on spark-2.

## FiQA Seed404, 1 Epoch

Run:
`/home/huoju/leask/runs/ii42-m405-overlap-anchored-query-correction-v1/fiqa_seed404_e1/m405_fiqa_seed404_e1.json`

| Route | NDCG@10 | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense | 0.51790 | 1.00000 | 0.84661 | 0.60205 | 0.45420 |
| teacher structural | 0.42180 | 0.54985 | 0.60346 | 0.52587 | 0.35875 |
| M405 overlap-anchored query | 0.42489 | 0.54654 | 0.60292 | 0.53259 | 0.35984 |
| teacher structural + BM25 a0.10 | 0.51750 | 0.79926 | 0.82970 | 0.61054 | 0.45105 |
| M405 + BM25 a0.10 | 0.51940 | 0.79216 | 0.82790 | 0.61115 | 0.45063 |

This is the best local signal from the new line:

- learned-only beats deterministic structural by `+0.00309`;
- Dense O@100 stays close to teacher and improves over M403-A seed404
  (`0.54654` vs M403-A `0.53802`);
- static BM25 blend slightly beats exact dense and teacher+BM25 on NDCG@10.

## FiQA Seed404, 2 Epochs

Run:
`/home/huoju/leask/runs/ii42-m405-overlap-anchored-query-correction-v1/fiqa_seed404_e2/m405_fiqa_seed404_e2.json`

| Route | NDCG@10 | Dense O@100 | Recall@100 |
| --- | ---: | ---: | ---: |
| teacher structural | 0.42180 | 0.54985 | 0.60346 |
| M405 overlap-anchored query | 0.41788 | 0.54534 | 0.59703 |
| teacher structural + BM25 a0.10 | 0.51750 | 0.79926 | 0.82970 |
| M405 + BM25 a0.10 | 0.51110 | 0.79034 | 0.82327 |

Deeper training is negative.  M405 should use 1 epoch or early stopping, not a
longer blind distillation loop.

## Four-Task Canary

Run:
`/home/huoju/leask/runs/ii42-m405-overlap-anchored-query-correction-v1/multitask_4x64_e1/m405_multitask_4x64_e1.json`

Config:

- tasks: `FiQA2018,ArguAna,SCIDOCS,TRECCOVID`
- epochs: `1`
- max train groups per task: `64`

### Macro

| Source | NDCG@10 |
| --- | ---: |
| exact dense | 0.49596 |
| teacher structural | 0.42365 |
| M405 overlap-anchored query | 0.42392 |
| teacher structural + BM25 a0.10 | 0.51375 |
| M405 + BM25 a0.10 | 0.51127 |

### Per Task

| Task | Source | NDCG@10 | Dense O@100 | Recall@100 |
| --- | --- | ---: | ---: | ---: |
| ArguAna | teacher structural | 0.43818 | 0.94394 | 1.00000 |
| ArguAna | M405 query | 0.43891 | 0.93466 | 1.00000 |
| FiQA2018 | teacher structural | 0.44370 | 0.55985 | 0.62453 |
| FiQA2018 | M405 query | 0.44396 | 0.55932 | 0.62602 |
| SCIDOCS | teacher structural | 0.22644 | 0.83528 | 0.48163 |
| SCIDOCS | M405 query | 0.22557 | 0.83160 | 0.48883 |
| TRECCOVID | teacher structural | 0.58627 | 0.18520 | 0.06322 |
| TRECCOVID | M405 query | 0.58726 | 0.18440 | 0.06316 |

The four-task result is weak but directionally useful:

- learned-only macro is slightly above teacher (`+0.00027`);
- Dense O@100 is almost preserved;
- BM25 blend is below deterministic structural + BM25, so this is not yet a
  production replacement for the static hybrid route.

## Conclusion

M405 is the most promising new branch from the M404/M405 push, but it is not a
finished breakthrough.

Validated points:

- The useful learning should happen in structural query space, not shared dense
  space.
- One epoch plus overlap anchors can preserve dense overlap better than M403-A.
- Blindly deeper training is harmful.

Current best level:

- FiQA seed404 learned-only: `0.42489` vs teacher `0.42180`;
- FiQA seed404 hybrid: `0.51940` vs dense `0.51790` and teacher+BM25 `0.51750`;
- four-task learned-only macro: `0.42392` vs teacher `0.42365`;
- four-task hybrid macro still below teacher+BM25.

## Next Step

Do not scale M404.  Do not deepen M405 without early stopping.

The next version should keep the M405 structural-space shape and add a
qrels-free selection gate:

- train a small predictor for whether a corrected structural candidate survives
  deterministic structural + static BM25 top-k;
- keep static BM25 alpha outside the model;
- reject updates that reduce Dense O@100 beyond a small threshold;
- use one epoch or validation-free early stopping based on dense-anchor loss.

This tests whether learning can improve candidate survival while preserving the
strong static hybrid baseline.
