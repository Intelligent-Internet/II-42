# SAE M54 Stage-A Distillation Results Report

## Summary

M54 A/B smoke tested whether Stage-A teacher-shape distillation should start
from an existing text-to-atoms checkpoint or from scratch.

Result:

```text
A: init checkpoint + M54 distillation = positive teacher-shape movement
B: scratch + M54 distillation         = not usable after 1 epoch
```

This supports the two-step Stage-A design:

```text
M52 broad teacher-imitation pretraining
-> M54 low-LR teacher-shape distillation
-> later Stage-B supervised ranking
```

M54 should not be trained from scratch as the default. From-scratch training
may still be a long warmup control, but it is not the efficient or stable
route for the current Stage-A objective.

## Run

Spark run:

```text
host = huoju@100.123.2.95
output = results/sae/m54/stage-a-distill-ab-smoke-spark
datasets = scifact, nfcorpus
epochs = 1
doc_active = 128
query_active = 96
teacher = shared_sae_8192_64
```

Arm A:

```text
init checkpoint = m22-token-lse-tokenchar-h256-asym96-budget8-five
learning_rate = 4e-4
```

Arm B:

```text
init checkpoint = none
learning_rate = 2e-3
```

## Stage-A Teacher Shape

Teacher support recall deltas versus the baseline checkpoint:

| Arm | Dataset | Split | Teacher Recall Delta | Jaccard Delta |
| --- | --- | --- | ---: | ---: |
| A init | nfcorpus | docs | +0.010603 | +0.003885 |
| A init | nfcorpus | queries | +0.002812 | +0.001240 |
| A init | scifact | docs | +0.009219 | +0.003345 |
| A init | scifact | queries | +0.005781 | +0.002541 |
| B scratch | nfcorpus | docs | -0.124053 | -0.043406 |
| B scratch | nfcorpus | queries | -0.112656 | -0.047680 |
| B scratch | scifact | docs | -0.106437 | -0.037021 |
| B scratch | scifact | queries | -0.097187 | -0.040703 |

Arm A improves all four Stage-A support metrics. Arm B is effectively
untrained for teacher support after one epoch.

## Atom Shape

Baseline unique student SAE dimensions:

| Dataset | Doc dims | Query dims |
| --- | ---: | ---: |
| nfcorpus | 6,766 | 3,797 |
| scifact | 7,087 | 3,029 |

Arm A:

| Dataset | Doc dims | Query dims |
| --- | ---: | ---: |
| nfcorpus | 7,321 | 3,940 |
| scifact | 7,567 | 3,194 |

Arm B:

| Dataset | Doc dims | Query dims |
| --- | ---: | ---: |
| nfcorpus | 657 | 614 |
| scifact | 664 | 301 |

Arm A expands toward the teacher's broader atom vocabulary. Arm B collapses to
a tiny set of high-value dimensions, which is not the desired teacher semantic
shape.

## Ranking Sanity

Ranking is not the M54 promotion metric, but it is still a collapse sanity
check.

Mean metrics:

| Path | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| baseline best-ish student | 0.653229 | 0.778242 | 0.652990 | 0.531360 |
| Arm A best-ish student | 0.657258 | 0.771468 | 0.646227 | 0.526076 |
| Arm B best-ish student | 0.604828 | 0.683379 | 0.568723 | 0.451181 |

Arm A has small ranking drift while improving teacher shape. Arm B is a clear
ranking collapse.

## M52 Context

The Spark M52E no-Wikipedia e6 fidelity result also completed:

| Run | Docs Recall | Queries Recall | Docs Jaccard | Queries Jaccard |
| --- | ---: | ---: | ---: | ---: |
| M52F wiki e6 | 0.274606 | 0.260834 | 0.101655 | 0.117710 |
| M52E no-wiki e6 | 0.283956 | 0.261925 | 0.105572 | 0.118312 |

No-Wikipedia e6 is better on both docs and queries, especially docs. This
supports using the best Stage-A checkpoint as the M54 starting point rather
than starting M54 from scratch.

## Decision

Promote Arm A as the M54 route:

```text
Stage-A checkpoint -> low-LR M54 distillation
```

Do not promote Arm B as a practical route:

```text
scratch + M54 loss
```

If scratch remains interesting, it should be tested only with a longer
teacher-imitation warmup or a pure M52 objective first. It is not the next
productive path for M54.

## Scaled M52E Run

The scaled Spark run used the best M52 Stage-A checkpoint rather than the
older M22 checkpoint:

```text
start checkpoint = M52E no-wiki e6
corpus = m52e-balanced-no-wiki-750k
teacher = shared_sae_8192_64
doc_active = 128
query_active = 96
primary run = e2-clean, lr=4e-4
automatic continuation = e4-cont, lr=2e-4
```

The unattended monitor promoted `e2-clean` to `e4-cont` only after `e2-clean`
preserved query fidelity and improved document fidelity. Both checkpoints
improve the Stage-A teacher shape versus M52E:

| Run | Docs Recall | Queries Recall | Docs Jaccard | Queries Jaccard |
| --- | ---: | ---: | ---: | ---: |
| M52E no-wiki e6 | 0.283956 | 0.261925 | 0.105572 | 0.118312 |
| M54 e2-clean | 0.310884 | 0.283413 | 0.116925 | 0.129321 |
| M54 e4-cont | 0.313332 | 0.286103 | 0.117972 | 0.130719 |

This confirms that the M54 objective is useful for Stage-A semantic-shape
distillation. It also shows diminishing returns: `e4-cont` improves fidelity
only modestly over `e2-clean`.

## Full15 Ranking Sanity

The full15 sanity pass compares M52E, M54 e2, and M54 e4 with the same
`doc128/query96` export and student SAE weight grid. This is not the M54
promotion metric, but it catches downstream ranking regressions before Stage B.

Best student rows:

| Run | Best Recall Source | Recall@100 | Best MRR Source | MRR@20 | Best NDCG Source | NDCG@10 | Best MAP Source | MAP@100 |
| --- | --- | ---: | --- | ---: | --- | ---: | --- | ---: |
| M52E baseline | `bm25_student_atoms_w0p5` | 0.796188 | `bm25_student_atoms_w0p5` | 0.791270 | `bm25_student_atoms_w0p5` | 0.677484 | `bm25_student_atoms_w0p5` | 0.640870 |
| M54 e2-clean | `bm25_student_atoms` | 0.798566 | `bm25_student_atoms_w0p25` | 0.789807 | `bm25_student_atoms_w0p25` | 0.678558 | `bm25_student_atoms_w0p25` | 0.638212 |
| M54 e4-cont | `bm25_student_atoms_w1` | 0.798042 | `bm25_student_atoms_w0p5` | 0.789262 | `bm25_student_atoms_w0p25` | 0.677530 | `bm25_student_atoms_w0p5` | 0.637938 |

Reference rows from the same eval:

| Path | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.783775 | 0.786255 | 0.667518 | 0.625401 |
| BM25 + Snowflake-SAE teacher | 0.844716 | 0.839950 | 0.749006 | 0.724736 |

Interpretation:

- `e2-clean` is the best balanced M54 checkpoint: it improves Recall@100 and
  NDCG@10 over M52E while keeping MRR/MAP regression small.
- `e4-cont` is the best pure Stage-A fidelity checkpoint, but it does not
  improve downstream ranking sanity versus `e2-clean`.
- Further Stage-A-only continuation should stop here. The next improvement
  needs Stage-B supervision or a loss that preserves the improved teacher shape
  while explicitly protecting ranking quality.

## Next Step

Use `M54 e2-clean` as the balanced checkpoint for Stage-B experiments and keep
`M54 e4-cont` as a semantic-fidelity control. Do not continue to `e6` with the
same Stage-A loss unless a new ranking-preservation term is added.

```text
balanced checkpoint = M54 e2-clean
semantic control = M54 e4-cont
next objective = Stage-B supervised ranking with teacher-shape anchoring
```
