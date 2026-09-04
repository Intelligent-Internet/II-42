# II-42 M408 Staged Text-to-Posting Report

Date: 2026-06-27

## Question

The final target is `text -> posting`, but the pipeline must be split into
diagnostic stages:

```text
dense embedding -> dense-derived posting -> raw text -> posting
```

M408 tests the first stage only.  If this stage cannot reproduce deterministic
dense-derived postings, then raw-text training should not start.

## Run

Remote host: `spark-2`

Execution mode: `CUDA_VISIBLE_DEVICES=` with low CPU/IO priority.

Artifacts:

- `/home/huoju/leask/runs/ii42-m408-staged-text-to-posting-v1/fiqa_gate_seed408/m408_fiqa_gate_seed408.json`
- `/home/huoju/leask/runs/ii42-m408-staged-text-to-posting-v1/fiqa_gate_seed408/m408_fiqa_gate_seed408.md`
- `/home/huoju/leask/runs/ii42-m408-staged-text-to-posting-v1/fiqa_linear_deep_seed408/m408_fiqa_linear_deep_seed408.json`
- `/home/huoju/leask/runs/ii42-m408-staged-text-to-posting-v1/fiqa_linear_deep_seed408/m408_fiqa_linear_deep_seed408.md`

Variants:

- `exact_postprocess`: no learned weights except exact known rotation and
  deterministic post-process.
- `linear_fit`: dense-input linear coordinate head trained only with
  representation imitation, not qrels and not retrieval ranking.

## Retrieval Matrix

FiQA heldout query split, seed `408`.

| Source | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Touch | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.50298 | 0.81522 | 0.60798 | 0.44503 | 1.00000 | 1.00000 |
| `m408_teacher_structural_tail` | 0.39527 | 0.57014 | 0.50226 | 0.33777 | 0.08002 | 0.54324 |
| `m408_exact_postprocess` | 0.39527 | 0.57014 | 0.50226 | 0.33777 | 0.08002 | 0.54324 |
| `m408_linear_fit` | 0.40631 | 0.57828 | 0.52889 | 0.34785 | 0.08002 | 0.54185 |
| `m408_teacher_structural_tail_bm25_a010` | 0.49877 | 0.79415 | 0.59825 | 0.43669 | 0.14520 | 0.79756 |
| `m408_exact_postprocess_bm25_a010` | 0.49877 | 0.79415 | 0.59825 | 0.43669 | 0.14520 | 0.79756 |
| `m408_linear_fit_bm25_a010` | 0.50547 | 0.78888 | 0.60460 | 0.43992 | 0.14517 | 0.78160 |
| `m408_linear_fit_deep` | 0.39427 | - | - | - | - | - |
| `m408_linear_fit_deep_bm25_a010` | 0.49899 | - | - | - | - | - |

## Representation Diagnostics

| Variant | Doc Coord Cos | Doc Active Jaccard | Doc Sketch Cos | Query Coord Cos | Query Active Jaccard | Query Sketch Cos |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_postprocess` | 0.99927 | 1.00000 | 0.99927 | 1.00000 | 1.00000 | 1.00000 |
| `linear_fit` | 0.96898 | 0.76985 | 0.81051 | 0.96909 | 0.77925 | 0.98951 |
| `linear_fit_deep` | 0.99925 | 0.99734 | 0.99751 | 0.99998 | 0.99679 | 0.99998 |

## Findings

1. The staged formulation is correct.  `exact_postprocess` matches
   `m408_teacher_structural_tail` exactly in retrieval metrics and active
   support.
2. Dense-to-posting is learnable.  A small CPU-only linear regression probe
   reaches about `0.969` coordinate cosine after only `8192` rows and `4`
   epochs.
3. The remaining failure mode in shallow training is active support and
   document tail sketch, not global coordinate reconstruction.  The shallow
   `linear_fit` has good coord cosine but only `0.77` active Jaccard and `0.81`
   doc sketch cosine.
4. Deeper qrels-free representation imitation solves the Stage-A problem:
   `32768` rows and `16` epochs reach about `0.997` active Jaccard and `0.997`
   document sketch cosine.
5. A learned dense-to-posting approximation can perturb ranking in useful ways:
   `linear_fit_bm25_a010` slightly exceeds exact dense on this FiQA split
   (`0.50547` vs `0.50298`).  This is not yet a promotion claim; it is a signal
   that representation imitation can be a valid first-stage training target.
6. Once `linear_fit` becomes nearly exact, retrieval returns to the teacher
   neighborhood (`0.39427` vs teacher `0.39527`, hybrid `0.49899` vs teacher
   hybrid `0.49877`).  That is expected and confirms the training target is
   actually "be the dense-derived posting teacher."

## Decision

Continue M408, but keep it staged:

1. Treat Stage A as solved for dense-input linear imitation.
2. Start Stage B: raw text to dense-derived posting.
3. Do not reintroduce ranking loss until the raw-text representation gate
   passes.

The next immediate run should identify the dense teacher model/tokenizer used
to create the materialized embeddings, then train a raw-text student against
the M408 posting targets.

## Checks

- Local `py_compile` passed for
  `scripts/research_sae_m408_staged_text_to_posting.py`.
- Local `git diff --check --no-index` passed for M408 script and plan.
- Remote `py_compile` passed on `spark-2`.
- No residual `ii42_m408_*` tmux session or M408 Python process remained after
  the gate run.
