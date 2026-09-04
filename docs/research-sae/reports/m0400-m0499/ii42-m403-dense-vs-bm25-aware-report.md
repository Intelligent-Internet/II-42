# II-42 M403 Dense-Faithful vs BM25-Aware Report

Date: 2026-06-27

## Goal

M403 continues both paths after M402:

- Branch A: dense-faithful query-side correction.  BM25 is not allowed in the
  pass condition.
- Branch B: BM25-aware hybrid fusion.  This is evaluated against deterministic
  structural + BM25, not as a dense-faithful encoder.

Training remains qrels-free.  It uses exact dense score groups and BM25 scores
as inputs only for the hybrid branch.

## Run Surfaces

Remote host: `spark-2`

Remote working directory:
`/home/huoju/leask/runs/mteb-m393-tail-bm25-v1`

Remote output root:
`/home/huoju/leask/runs/ii42-m403-dense-vs-bm25-aware-v1`

Local code:
`scripts/research_sae_m403_dense_vs_bm25_aware.py`

Local plan:
`docs/research-sae/reports/m0400-m0499/ii42-m403-dense-vs-bm25-aware-plan.md`

Execution was CPU-only and low priority:

```bash
CUDA_VISIBLE_DEVICES= OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 \
OPENBLAS_NUM_THREADS=4 nice -n 10 ionice -c2 -n7 ...
```

## FiQA Canary Matrix

Task: `FiQA2018`

Primary metric: `NDCG@10`.

### Seed 403, 8-Group Sanity, 1 Epoch

| Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense | 0.51623 | 0.83449 | 0.60803 | 0.45142 | 1.00000 |
| teacher structural | 0.42571 | 0.60228 | 0.53270 | 0.36219 | 0.54296 |
| A dense query correction | 0.42923 | 0.60676 | 0.53478 | 0.36695 | 0.54293 |
| B hybrid branch learned-only | 0.42800 | 0.60202 | 0.53175 | 0.36486 | 0.54256 |
| teacher structural + BM25 a0.10 | 0.51440 | 0.80766 | 0.60426 | 0.44891 | 0.80052 |
| A dense query correction + BM25 a0.18 | 0.51473 | 0.79730 | 0.60506 | 0.44975 | 0.74596 |
| B learned BM25 fusion | 0.51294 | 0.80267 | 0.60428 | 0.44901 | 0.79985 |

### Seed 403, Full Queries, 1 Epoch

| Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense | 0.51623 | 0.83449 | 0.60803 | 0.45142 | 1.00000 |
| teacher structural | 0.42571 | 0.60228 | 0.53270 | 0.36219 | 0.54296 |
| A dense query correction | 0.43238 | 0.60311 | 0.53653 | 0.37142 | 0.53336 |
| B hybrid branch learned-only | 0.43041 | 0.60741 | 0.53513 | 0.36643 | 0.53481 |
| teacher structural + BM25 a0.10 | 0.51440 | 0.80766 | 0.60426 | 0.44891 | 0.80052 |
| A dense query correction + BM25 a0.18 | 0.51260 | 0.79814 | 0.60577 | 0.44746 | 0.73444 |
| B learned BM25 fusion | 0.50578 | 0.80359 | 0.59469 | 0.44025 | 0.79559 |

### Seed 403, Full Queries, 2 Epochs

| Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense | 0.51623 | 0.83449 | 0.60803 | 0.45142 | 1.00000 |
| teacher structural | 0.42571 | 0.60228 | 0.53270 | 0.36219 | 0.54296 |
| A dense query correction | 0.42544 | 0.59429 | 0.53699 | 0.36329 | 0.52247 |
| B hybrid branch learned-only | 0.43704 | 0.61473 | 0.54369 | 0.37375 | 0.53102 |
| teacher structural + BM25 a0.10 | 0.51440 | 0.80766 | 0.60426 | 0.44891 | 0.80052 |
| A dense query correction + BM25 a0.10 | 0.51281 | 0.80436 | 0.60510 | 0.44625 | 0.76265 |
| B learned BM25 fusion | 0.50166 | 0.80661 | 0.59176 | 0.43963 | 0.78627 |

### Seed 404, Full Queries, 1 Epoch

| Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact dense | 0.51790 | 0.84661 | 0.60205 | 0.45420 | 1.00000 |
| teacher structural | 0.42180 | 0.60346 | 0.52587 | 0.35875 | 0.54985 |
| A dense query correction | 0.42422 | 0.59086 | 0.53752 | 0.36304 | 0.53802 |
| B hybrid branch learned-only | 0.43004 | 0.61616 | 0.52711 | 0.36638 | 0.53861 |
| teacher structural + BM25 a0.10 | 0.51750 | 0.82970 | 0.61054 | 0.45105 | 0.79926 |
| A dense query correction + BM25 a0.10 | 0.51594 | 0.82147 | 0.60456 | 0.44730 | 0.77346 |
| B learned BM25 fusion | 0.51198 | 0.82787 | 0.59651 | 0.44102 | 0.79241 |

Raw outputs:

- sanity:
  `/home/huoju/leask/runs/ii42-m403-dense-vs-bm25-aware-v1/sanity/m403_sanity.json`
- full seed403 e1:
  `/home/huoju/leask/runs/ii42-m403-dense-vs-bm25-aware-v1/full_e1/m403_full_e1.json`
- full seed403 e2:
  `/home/huoju/leask/runs/ii42-m403-dense-vs-bm25-aware-v1/full_e2/m403_full_e2.json`
- full seed404 e1:
  `/home/huoju/leask/runs/ii42-m403-dense-vs-bm25-aware-v1/seed404_e1/m403_seed404_e1.json`

## Interpretation

Branch A produced the first repeatable learned-only NDCG improvement in this
line:

- seed403 full e1: `0.43238` vs teacher `0.42571`;
- seed404 full e1: `0.42422` vs teacher `0.42180`.

This is a real positive signal, but it is not yet a full dense-faithfulness
success.  Dense O@100 drops in the full runs:

- seed403 teacher: `0.54296`, A e1: `0.53336`;
- seed404 teacher: `0.54985`, A e1: `0.53802`.

The model is improving top-rank ordering while slightly hurting dense top-100
overlap.  That means it is learning a useful query correction, but not yet a
strict dense-preserving posting encoder.

Branch B did not validate the learned BM25 fusion objective:

- learned BM25 fusion is below deterministic structural + static BM25 in every
  full run;
- the static BM25 blends remain very strong;
- the hybrid branch learned-only sometimes improves structural-only, but its
  fusion head does not convert that into better hybrid ranking.

## Conclusion

M403-A should continue.  It is the first branch in M401-M403 that repeatedly
beats deterministic structural tail without BM25, even though the gain is small
and Dense O@100 regresses.

M403-B should not continue in its current fusion form.  The learned alpha head
is not better than a static BM25 blend.  If BM25-aware training continues, it
needs a different target: candidate survival/admission under static lexical
blend, not learned scalar fusion.

## Recommended Next Step

M404 should focus on Branch A:

- keep query-side correction;
- add an explicit Dense O@100 preservation term;
- add a stop rule that rejects NDCG gains that reduce overlap too much;
- keep training to one epoch or use early stopping, because seed403 e2 degrades
  the dense-query branch;
- evaluate on a small BEIR subset only after FiQA overlap no longer regresses.

For the hybrid line, the next useful test is not another alpha head.  The better
experiment is a BM25-aware admission label:

- freeze static BM25 blend alpha;
- train a model to predict whether structural candidates survive the fused
  top-k;
- compare against deterministic structural + static BM25 as the baseline.
