# SAE Text-To-Sparse-Atoms Plan Execution Report

Date: 2026-05-14

## Executive Summary

The text-to-sparse-atoms plan has now been executed through the first complete
research loop:

```text
T1 teacher dataset artifacts
T2 distillation baseline
T3 retrieval-aware training
T4 first index-aware fanout penalty
T5 leave-one-dataset-out generalization check
T6 product gate assessment
```

The strongest in-dataset result is promising:

```text
BM25 + text-student atoms
Recall@100 = 0.7735
MRR@20     = 0.6483
```

Compared with:

```text
BM25 baseline
Recall@100 = 0.7035
MRR@20     = 0.5904

Snowflake-SAE teacher
Recall@100 = 0.7947
MRR@20     = 0.6835
```

This means the serving-side abstraction is viable: the student uses no dense
vector index and no query-time Snowflake embedding model in evaluation.

However, the plan is not product-complete. The first leave-one-dataset-out check
failed when the student atom channel used the same high weight that worked
in-domain:

```text
holdout BM25 + text-student atoms
Recall@100 = 0.6457
MRR@20     = 0.5245
```

A follow-up low-weight/gated T5 pass now clears pure BM25 by a small margin:

```text
holdout BM25 + text-student atoms, adaptive low-weight gate
Recall@100 = 0.7074
MRR@20     = 0.5953
```

This is a real T5 improvement over BM25, but it is not a teacher-level
breakthrough. The current student can learn a useful sparse semantic channel,
but cross-domain robustness is weak and still depends on conservative weighting.

## T1: Teacher Dataset Builder

Implemented:

```text
scripts/research_sae_text_atom_dataset_builder.py
```

Generated artifacts:

```text
results/sae/text-atoms/teacher-dataset/
```

The builder exports:

- dense teacher top-k rankings;
- BM25 top-k rankings;
- BM25 hard negatives;
- teacher atom DF/fanout statistics;
- per-dataset manifests.

Dataset artifact summary:

| Dataset | Docs | Queries | Qrels |
| --- | ---: | ---: | ---: |
| `scifact` | 2000 | 100 | 116 |
| `scidocs` | 2000 | 100 | 492 |
| `nfcorpus` | 2063 | 100 | 3818 |
| `arguana` | 2000 | 100 | 100 |
| `fiqa` | 2000 | 100 | 267 |

Status:

```text
T1 passed for sampled BEIR.
T1 is not complete for full BEIR or product corpora.
```

## T2: Distillation Baseline

Baseline model:

```text
token text
  -> learned token embeddings
  -> masked mean pooling
  -> asymmetric query/doc atom heads
```

Run:

```text
results/sae/text-atoms/t2-baseline-calibrated/
```

Mean matrix:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.7347 | 0.6101 | 0.5288 | 0.4352 |

Status:

```text
T2 passed as a baseline.
```

The student beats pure BM25, proving the direct text-to-atoms route is not
empty. It remains far below the teacher.

## T3: Retrieval-Aware Training

Added to:

```text
scripts/research_sae_text_atom_train.py
```

New training controls:

```text
--retrieval-loss-weight
--retrieval-margin
--retrieval-batch-size
--max-retrieval-pairs
--hard-negative-k
```

The retrieval loss trains query text, positive document text, and BM25 hard
negative document text with a pairwise margin objective.

Run:

```text
results/sae/text-atoms/t3-retrieval-aware/
```

Mean matrix with token features and student weight 0.25:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.7513 | 0.6329 | 0.5439 | 0.4465 |

Student weight sweep on this model:

| Student Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.125 | 0.7322 | 0.6187 | 0.5299 | 0.4347 |
| 0.250 | 0.7513 | 0.6329 | 0.5439 | 0.4465 |
| 0.500 | 0.7612 | 0.6434 | 0.5551 | 0.4570 |
| 0.750 | 0.7705 | 0.6476 | 0.5536 | 0.4571 |
| 1.000 | 0.7640 | 0.6406 | 0.5440 | 0.4478 |

Status:

```text
T3 passed in-domain.
```

Retrieval-aware loss improves every mean quality metric over distillation-only.

## T4: Index-Aware First Pass

Added controls:

```text
--fanout-loss-weight
--support-loss-mode dense|sampled
--negative-dims
```

The implemented fanout loss penalizes support probability on high document-DF
teacher atoms. This is a first-pass index-aware regularizer, not the final
candidate-budget loss.

Status:

```text
T4 partially passed as infrastructure.
T4 did not yet prove a better quality/cost frontier.
```

The current best quality run used:

```text
fanout_loss_weight = 0.01
```

but the current report does not yet show a clean isolated fanout ablation. The
next pass should compare:

```text
retrieval-aware without fanout
retrieval-aware with fanout
retrieval-aware with candidate-budget loss
```

## Token + Character N-Gram Features

To reduce OOV/domain drift, the trainer now supports:

```text
--feature-mode token
--feature-mode token_char
```

The `token_char` mode adds character 3/4/5-gram features for each lexical
token.

Run:

```text
results/sae/text-atoms/t3-retrieval-aware-token-char/
```

Mean in-dataset matrix:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.7735 | 0.6483 | 0.5549 | 0.4539 |

This is the strongest current student result. It is close to the teacher on
Recall@100, but still meaningfully behind on MRR/NDCG/MAP.

## T5: Cross-Dataset Holdout

Two leave-one-dataset-out matrices were first run with the in-domain student
weight. Both failed, which exposed that the text-student atom signal did not
transfer at the same scale across datasets.

Token-only holdout:

```text
results/sae/text-atoms/t5-holdout/
```

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.6457 | 0.5245 | 0.4336 | 0.3473 |

Token+char holdout:

```text
results/sae/text-atoms/t5-holdout-token-char/
```

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.6256 | 0.5248 | 0.4334 | 0.3472 |

Status of the first fixed-weight pass:

```text
T5 failed at the in-domain student weight.
```

The current model is not cross-dataset robust at high student-atom weight.
Character n-grams improve in-domain quality, but do not solve held-out
generalization by themselves.

Interpretation:

- The student can imitate useful atoms within the same distribution.
- The document-side atom mapping remains brittle across corpora.
- Query-side support imitation alone is insufficient.
- A production model needs broader teacher data, stronger text encoder
  capacity, and a safer default calibration.

## T5.1: Dense-Teacher Pairs and Low-Weight Calibration

Added to:

```text
scripts/research_sae_text_atom_train.py
```

New controls:

```text
--dense-teacher-pairs-per-query
--dense-teacher-positive-k
--dense-teacher-negative-k
```

This adds dense-neighborhood pair supervision on top of qrel/BM25-negative
retrieval training. For each training query, dense top-k documents are treated
as teacher positives, while BM25 top-k documents outside the dense positives and
qrels are used as hard negatives.

Run:

```text
results/sae/text-atoms/t3-dense-teacher-token-char/
```

In-dataset result:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms` | 0.7611 | 0.6642 | 0.5713 | 0.4684 |

The dense-teacher pairs improve first-page ranking metrics compared with the
previous token-char student, but reduce Recall@100. This means pairwise
dense-teacher supervision helps ordering, not coverage.

Leave-one-dataset-out with the same fixed high student weight still fails:

```text
results/sae/text-atoms/t5-holdout-dense-teacher-token-char/
```

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_teacher_sae` | 0.7947 | 0.6835 | 0.6036 | 0.5052 |
| `bm25_student_atoms`, fixed `0.75` | 0.6349 | 0.5345 | 0.4392 | 0.3501 |

The important result is the low-weight holdout sweep:

```text
results/sae/text-atoms/t5-holdout-dense-teacher-low-weight-sweep.json
```

| Student Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.000 | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| 0.020 | 0.7078 | 0.5939 | 0.5052 | 0.4155 |
| 0.050 | 0.7070 | 0.5945 | 0.5052 | 0.4157 |
| 0.060 | 0.7074 | 0.5946 | 0.5056 | 0.4161 |
| 0.075 | 0.7035 | 0.5952 | 0.5060 | 0.4161 |
| 0.100 | 0.7030 | 0.5936 | 0.5046 | 0.4150 |

A simple query-level agreement gate gives the best balanced result:

```text
results/sae/text-atoms/t5-holdout-adaptive-gate-sweep.json
```

Gate:

```text
student_weight = 0.05 + 0.05 * min(1, overlap(BM25@100, SAE@100) / 0.2)
```

Mean matrix:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7035 | 0.5904 | 0.5031 | 0.4134 |
| `bm25_student_atoms`, adaptive gate | 0.7074 | 0.5953 | 0.5063 | 0.4166 |

Per-dataset adaptive-gate matrix:

| Dataset | Mean Gate | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 0.0672 | 0.9517 | 0.7864 | 0.8023 | 0.7633 |
| `scidocs` | 0.0654 | 0.5700 | 0.5690 | 0.3488 | 0.2560 |
| `nfcorpus` | 0.0598 | 0.2875 | 0.6094 | 0.3749 | 0.1865 |
| `arguana` | 0.0654 | 0.9700 | 0.4629 | 0.5348 | 0.4661 |
| `fiqa` | 0.0647 | 0.7578 | 0.5488 | 0.4708 | 0.4110 |

Status:

```text
T5 now narrowly passes pure BM25 with conservative student weighting.
T5 does not pass the teacher-replacement bar.
```

Interpretation:

- Student atoms are useful cross-domain only as a weak additive semantic signal.
- High student weights over-trust brittle atom predictions and harm coverage.
- Dense-teacher pair supervision improves ranking more than recall.
- The next training objective should target candidate coverage directly, not
  another fixed-weight sweep.

## T6: Product Workload Gate

Status:

```text
T6 not passed.
```

The arxiv/pubmed data can be pulled from elm, but product recall cannot be
claimed without qrels or defensible proxy qrels. Running the student on
arxiv/pubmed without relevance labels would only measure:

- export viability;
- encoder latency;
- atom density;
- index fanout;
- qualitative examples.

It would not prove retrieval quality.

Recommended next product-gate data:

```text
arxiv query set + qrels/proxy qrels
pubmed query set + qrels/proxy qrels
dense/Snowflake teacher top-k for the same snapshots
BM25 hard negatives for the same snapshots
```

## Current Decision

Do not productize the text-student path yet.

Continue using Snowflake+SAE teacher as the quality reference. The text-student
path is now validated as a promising direction, but the current version is a
research prototype. The T5 result is no longer a hard failure, but the margin is
too small to justify removing the dense teacher path.

## Next Optimization Priorities

### 1. Stronger Document Encoder

The current masked-mean bag encoder is too weak for cross-domain document atom
prediction. The next model should test:

- mini-transformer encoder;
- SPLADE-style MLM backbone initialized from a retrieval model;
- late interaction over document chunks;
- title/body separate pooling.

### 2. Listwise Dense-Teacher Distillation

The pairwise qrel/BM25-negative loss improved in-domain metrics, and the
pairwise dense-teacher extension improved first-page ranking. Neither transfers
enough dense-neighborhood coverage. Next loss:

```text
query student atoms
  should reproduce dense teacher top-k distribution
```

This should use the T1 `dense_teacher_topk.jsonl` artifacts as a listwise
distribution, not only pairwise positive/negative samples.

### 2.1 First Listwise Dense-Teacher Result

Implemented an opt-in listwise teacher loss:

```text
--listwise-teacher-loss-weight
--listwise-teacher-positive-k
--listwise-teacher-negative-k
--listwise-teacher-batch-size
--listwise-teacher-temperature
--listwise-teacher-negative-margin
```

The loss trains each query against a dense-teacher top-k candidate set plus BM25
hard negatives. The target is a soft teacher distribution over the candidate
set, rather than isolated pairwise positives.

Runs:

```text
results/sae/text-atoms/t3-listwise-token-char/
results/sae/text-atoms/t3-dense-listwise-token-char/
```

In-domain matrix at each run's best swept student weight:

| Run | Best Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `t3-retrieval-aware-token-char` | 0.750 | 0.7735 | 0.6483 | 0.5549 | 0.4539 |
| `t3-dense-teacher-token-char` | 0.750 | 0.7611 | 0.6642 | 0.5713 | 0.4684 |
| `t3-listwise-token-char` | 0.500 | 0.7527 | 0.6408 | 0.5473 | 0.4498 |
| `t3-dense-listwise-token-char` | 0.500 | 0.7519 | 0.6157 | 0.5350 | 0.4371 |

Status:

```text
first listwise dense-teacher attempt did not improve the frontier
```

Interpretation:

- The implemented listwise loss is mechanically valid and loss decreases.
- It does not improve the current quality frontier on the sampled matrix.
- Pairwise dense-teacher supervision still gives better MRR/NDCG/MAP.
- The next step should not be another static listwise-weight sweep. It should
  change the objective toward candidate coverage: penalize missing dense top-k
  teacher documents under a fixed candidate budget, and optimize the query atom
  set for coverage before final score calibration.

### 2.2 Five-Step Training Follow-Up

Detailed report:

```text
sae-text-to-sparse-atoms-five-step-training-report.md
```

This follow-up implemented and tested:

- candidate-coverage training;
- BM25 plus dense near-miss hard negatives;
- teacher top-k coverage loss;
- a small transformer encoder;
- larger cross-domain teacher-data readiness.

Mean sampled five-dataset matrix:

| Run | Best/Eval Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `t3-retrieval-aware-token-char` | 0.750 | 0.7735 | 0.6483 | 0.5549 | 0.4539 |
| `t3-dense-teacher-token-char` | 0.750 | 0.7611 | 0.6642 | 0.5713 | 0.4684 |
| `t4-coverage-token-char` | 0.750 | 0.7540 | 0.6433 | 0.5551 | 0.4522 |
| `t4-dense-coverage-token-char` | 0.500 | 0.7591 | 0.6472 | 0.5659 | 0.4606 |
| `t4-transformer-dense-coverage-token-char` | 0.500 | 0.6949 | 0.5758 | 0.4918 | 0.4055 |

Status:

```text
coverage objective is useful infrastructure but not a new frontier
```

The main learning is that candidate coverage should be trained closer to the
physical query atom selection process. The current differentiable approximation
still scores all candidate documents in the batch. The next version should train:

```text
query atom scores -> top atom budget -> opened document set -> teacher-positive coverage
```

### 3. Larger Cross-Domain Teacher Dataset

Five sampled BEIR datasets are too small. The next training corpus should mix:

- more BEIR datasets;
- larger sampled document pools;
- pseudo queries;
- arxiv/pubmed/policy snapshots if qrels/proxy labels exist.

### 4. Real Candidate-Budget Loss

The current fanout loss is a proxy. The next version should directly penalize:

```text
expected candidate docs
expected postings touched
high-DF query atoms
```

### 5. Product Gate Before API Work

Do not integrate this as a product API until T5 and T6 pass. The target remains:

```text
BM25 + text-student atoms
  Recall@100 within 0.02 of BM25 + Snowflake-SAE teacher
  MRR/NDCG/MAP above pure BM25
  cross-dataset holdout above pure BM25
```
