# II-42 M412 Trainable Student Dense Distillation Report

Date: 2026-06-27

## Question

M412 tests whether partially fine-tuning a compact text encoder improves dense
teacher imitation beyond the M411 frozen-head route:

```text
raw text -> trainable MiniLM last layers + dense head -> pplx teacher dense
```

The loss uses only materialized dense vectors.  Qrels are evaluation-only.

## Runs

Remote host: `spark-1`

Artifacts:

- `/home/huoju/leask/runs/ii42-m412-trainable-student-dense-distill-v1/fiqa_minilm_last2_16k_e2_seed414/m412_fiqa_minilm_last2_16k_e2_seed414.json`
- `/home/huoju/leask/runs/ii42-m412-trainable-student-dense-distill-v1/fiqa_minilm_last2_32k_e8_seed415/m412_fiqa_minilm_last2_32k_e8_seed415.json`
- `/home/huoju/leask/runs/ii42-m412-trainable-student-dense-distill-v1/fiqa_minilm_last2_allrows_e16_seed416/m412_fiqa_minilm_last2_allrows_e16_seed416.json`

## Performance Matrix

| Variant | Train rows | Epochs | Dense NDCG@10 | Teacher NDCG@10 | Student posting NDCG@10 | Student + BM25 NDCG@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `last2_16k_e2_seed414` | 16,384 | 2 | 0.28878 | 0.51869 | 0.21990 | 0.34161 |
| `last2_32k_e8_seed415` | 32,768 | 8 | 0.32106 | 0.51539 | 0.23902 | 0.35821 |
| `last2_allrows_e16_seed416` | 58,286 | 16 | 0.36999 | 0.53283 | 0.28049 | 0.39699 |
| M411 best frozen baseline: `MiniLM MLP2048 allrows` | 58,286 | 24 | 0.35688 | 0.50726 | 0.28727 | 0.39264 |

## Representation Matrix

| Variant | Doc dense cos | Query dense cos | Doc active Jaccard | Query active Jaccard | Doc sketch cos | Query sketch cos |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `last2_16k_e2_seed414` | 0.70626 | 0.67939 | 0.34205 | 0.33694 | 0.11691 | 0.76587 |
| `last2_32k_e8_seed415` | 0.78485 | 0.77329 | 0.39616 | 0.40124 | 0.24750 | 0.84260 |
| `last2_allrows_e16_seed416` | 0.81965 | 0.79452 | 0.42643 | 0.42056 | 0.30675 | 0.86079 |
| M411 best frozen baseline: `MiniLM MLP2048 allrows` | 0.80684 | 0.78012 | 0.41441 | 0.41079 | 0.29686 | 0.84617 |

## Interpretation

M412 now shows a small positive dense-distillation signal, but not a
breakthrough.

1. More training helps: `e8` improves dense cosine and NDCG over `e2`.
2. The fairer `allrows_e16` run beats M411's frozen MiniLM MLP2048 all-row
   baseline on dense NDCG@10 (`0.36999` vs `0.35688`) and dense cosine.
3. The gain is still small relative to the teacher (`0.53283`) and does not
   close the dense gap.
4. Posting-only remains roughly at the frozen baseline, so the posting layer is
   still bottlenecked by dense imitation quality.
5. Blindly adding ranking/BM25 loss here would hide a weak dense student.

## Next

The next useful direction is not FiQA-specific micro-tuning.  Use M412's
positive slope to justify a broader dense-distillation stage:

- generate teacher dense targets for more corpora/texts;
- train with dense-only objective first;
- keep posting/ranking losses disabled until dense cosine and dense retrieval
  move substantially closer to teacher;
- add checkpointing and epoch-level evaluation before launching longer jobs.
