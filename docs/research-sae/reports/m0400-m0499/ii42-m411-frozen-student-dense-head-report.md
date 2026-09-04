# II-42 M411 Frozen Student Dense-Head Report

Date: 2026-06-27

## Question

M411 tests whether a frozen off-the-shelf text encoder already contains enough
semantic information to imitate the materialized `pplx` dense teacher through a
learned dense head:

```text
raw text -> frozen student encoder -> learned dense head -> M408 posting
```

The training loss uses only materialized dense vectors.  Qrels are used only
for evaluation.

## Runs

Remote host: `spark-1`

Artifacts:

- `/home/huoju/leask/runs/ii42-m411-frozen-student-dense-head-v1/fiqa_minilm_linear_seed411/m411_fiqa_minilm_linear_seed411.json`
- `/home/huoju/leask/runs/ii42-m411-frozen-student-dense-head-v1/fiqa_minilm_mlp2048_allrows_seed412/m411_fiqa_minilm_mlp2048_allrows_seed412.json`
- `/home/huoju/leask/runs/ii42-m411-frozen-student-dense-head-v1/fiqa_bgebase_mlp2048_allrows_seed413/m411_fiqa_bgebase_mlp2048_allrows_seed413.json`

## Performance Matrix

FiQA heldout-query canary.

| Variant | Student model | Head | Dense NDCG@10 | Teacher NDCG@10 | Student posting NDCG@10 | Student + BM25 NDCG@10 |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `seed411` | `all-MiniLM-L6-v2` | linear, 32k rows | 0.31766 | 0.51784 | 0.24087 | 0.36134 |
| `seed412` | `all-MiniLM-L6-v2` | MLP2048, all rows | 0.35688 | 0.50726 | 0.28727 | 0.39264 |
| `seed413` | `bge-base-en-v1.5` | MLP2048, all rows | 0.33816 | 0.49935 | 0.24566 | 0.37238 |

## Representation Matrix

| Variant | Doc dense cos | Query dense cos | Doc active Jaccard | Query active Jaccard | Doc sketch cos | Query sketch cos |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `MiniLM linear` | 0.69988 | 0.66241 | 0.33957 | 0.33190 | 0.13602 | 0.74589 |
| `MiniLM MLP2048` | 0.80684 | 0.78012 | 0.41441 | 0.41079 | 0.29686 | 0.84617 |
| `BGE-base MLP2048` | 0.80269 | 0.77010 | 0.41119 | 0.40266 | 0.26814 | 0.84070 |

## Interpretation

The frozen-head route is not enough.

1. A nonlinear head and full-row supervision help, but only move dense cosine
   to about `0.78-0.81`.
2. BGE-base does not beat MiniLM MLP on this teacher target, so simply swapping
   to a stronger frozen encoder is not the missing piece.
3. The remaining gap appears before posting compression.  Student dense
   retrieval is already far below teacher dense retrieval, so posting/BM25
   tuning would hide the wrong failure.
4. M410 proved the target is reachable when the same dense encoder is used.
   M411 shows the problem is student encoder adaptation, not deterministic
   posting.

## Next

Proceed to M412:

```text
raw text -> trainable student encoder + dense head -> teacher dense
```

The first M412 gate should fine-tune MiniLM end-to-end on dense imitation with
qrels held out for evaluation only.  Promotion requires dense parity movement
before adding posting or ranking losses.
