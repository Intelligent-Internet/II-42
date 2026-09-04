# II-42 M409 Raw Text Posting Distillation Report

Date: 2026-06-27

## Question

M409 tests the first raw-text stage:

```text
raw text -> dense-derived posting coordinates
```

The target is the M408 deterministic dense-derived posting surface.  Training
uses no qrels, no BM25-aware loss, and no ranking loss.

## Run

Remote host: `spark-2`

Execution mode: CPU-only, low CPU/IO priority.

Artifact:

- `/home/huoju/leask/runs/ii42-m409-raw-text-posting-distill-v1/fiqa_small_seed409/m409_fiqa_small_seed409.json`
- `/home/huoju/leask/runs/ii42-m409-raw-text-posting-distill-v1/fiqa_small_seed409/m409_fiqa_small_seed409.md`

Model:

- stable hashed word, bigram, and character trigram features;
- `EmbeddingBag` mean pooling;
- small coordinate head;
- deterministic M408 active-coordinate and tail-sketch post-process.

## Retrieval Matrix

FiQA heldout query split, seed `409`.

| Source | NDCG@10 |
| --- | ---: |
| `exact_dense_teacher` | 0.53739 |
| `m409_teacher_structural_tail_bm25_zblend_a010` | 0.53457 |
| `m409_teacher_structural_tail` | 0.42915 |
| `m409_raw_text_fit_bm25_zblend_a010` | 0.01724 |
| `m409_raw_text_fit` | 0.00770 |

## Representation Diagnostics

| Metric | Value |
| --- | ---: |
| Doc coord cosine | 0.08386 |
| Doc active Jaccard | 0.08307 |
| Doc sketch cosine | 0.00540 |
| Query coord cosine | 0.06134 |
| Query active Jaccard | 0.07693 |
| Query sketch cosine | 0.11957 |

Training loss did decrease:

| Trace | Values |
| --- | --- |
| coord | `[0.17990185, 0.05852596, 0.02369095, 0.01122515]` |
| active | `[0.02320709, 0.00792899, 0.00354221, 0.00195076]` |
| inactive | `[0.15720085, 0.05060008, 0.02014884, 0.00854671]` |
| loss | `[0.26548971, 0.08703319, 0.03581255, 0.01744526]` |

## Findings

1. The simple lexical hash encoder fits the sampled training rows but does not
   generalize to the dense-derived semantic geometry.
2. Retrieval is effectively collapsed.  BM25 blending cannot rescue this
   model because the predicted posting signal is nearly noise.
3. The failure is a text-encoder capacity / semantic pretraining failure, not a
   proof that the M408 posting target is wrong.
4. Scaling this exact hash model is not justified.  The next stage must use a
   real semantic text encoder path.

## Decision

Stop the M409 hash/lexical encoder line.

Proceed to M410:

```text
raw text -> cached pplx dense encoder -> deterministic M408 posting
```

M410 is an encoder-parity gate.  It must verify that re-encoding raw text with
the cached dense encoder reproduces the materialized dense embeddings closely
enough before training a smaller student.
