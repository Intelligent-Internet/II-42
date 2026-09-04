# II-42 M398 Deep Posting Loss Report

## Summary

M398 tested the user's hypothesis that the new direct posting encoder needs a
more search-shaped loss and deeper training before judging quality.

The result is mixed but useful:

- Deep MLP posting heads failed.
- Tied query/document dual heads failed again.
- Strong rank/listwise loss with hard active-coordinate training failed.
- A linear shared posting head with mild listwise/pairwise dense-order loss and
  longer training improved the BM25-free surface.

Best run:

- run: `fiqa_linearsoft_e24`
- host: `spark-2`
- source: `m398_shared_deep_deep_posting_loss`
- NDCG@10: `0.42356`
- touch: `0.08002`
- Recall@100: `0.70489`
- MRR@20: `0.50004`
- MAP@100: `0.36237`

This beats:

- M397B shared linear raw regression: `0.41340`
- M397 smoke shared linear raw regression: `0.41272`
- deterministic PCA coordinate posting: `0.39620`

The improvement is real on this held-out FiQA split, but it does not yet improve
posthoc BM25 fusion. PCA + BM25 remains stronger at `0.47491`.

## Runs

All runs used `FiQA2018`, held-out query split, no qrels for training, and
qrels only for final evaluation.

| Run | Head | Main loss shape | Epochs | Best BM25-free NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa_linearsoft_e24` | linear shared | raw score + mild listwise/pairwise | 24 | 0.42356 | 0.08002 | 0.51938 | 0.70489 | 0.50004 | 0.36237 |
| `fiqa_linearsoft_e48` | linear shared | raw score + mild listwise/pairwise | 48 | 0.41798 | 0.08002 | 0.50231 | 0.68510 | 0.48860 | 0.35965 |
| `fiqa_linearsoft_e12` | linear shared | raw score + mild listwise/pairwise | 12 | 0.41635 | 0.08002 | 0.52907 | 0.69427 | 0.48667 | 0.35354 |
| `fiqa_rankloss_e12` | deep shared/tied | strong listwise/pairwise + hard active | 12 | 0.36878 | 0.08002 | 0.49537 | 0.69444 | 0.43590 | 0.31423 |
| `fiqa_softloss_e12` | deep shared/tied | softer listwise/pairwise | 12 | 0.37795 | 0.08002 | 0.46799 | 0.68300 | 0.43596 | 0.31608 |
| `fiqa_rawpoint_e12` | deep shared/tied | raw score only | 12 | 0.35140 | 0.08002 | 0.46318 | 0.65228 | 0.42277 | 0.29785 |

## Best Matrix

Run: `fiqa_linearsoft_e24`

| Source | NDCG@10 | Touch | Dense O@100 | Recall@100 | MRR@20 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_teacher` | 0.50335 | 1.00000 | 1.00000 | 0.81999 | 0.57717 | 0.44694 |
| `m398_pca_doc_coord_bm25_zblend_a010` | 0.47491 | 0.14499 | 0.73636 | 0.79642 | 0.54914 | 0.41812 |
| `m398_pca_doc_coord_bm25_zblend_a018` | 0.47132 | 0.14499 | 0.70130 | 0.78466 | 0.54996 | 0.41292 |
| `m398_shared_deep_deep_posting_loss_bm25_zblend_a018` | 0.46781 | 0.14219 | 0.55787 | 0.75815 | 0.53620 | 0.40776 |
| `m398_shared_deep_deep_posting_loss_bm25_zblend_a010` | 0.46231 | 0.14219 | 0.56216 | 0.75683 | 0.52906 | 0.40232 |
| `m398_shared_deep_deep_posting_loss` | 0.42356 | 0.08002 | 0.51938 | 0.70489 | 0.50004 | 0.36237 |
| `m398_pca_doc_coord` | 0.39620 | 0.08002 | 0.51920 | 0.60573 | 0.49216 | 0.34308 |

## Interpretation

The new loss helps only when it preserves the known-good linear shared geometry.
The winning configuration is not a deeper encoder; it is a linear shared
projection with a search-shaped objective.

The reason is visible in the failed runs:

- Deep heads learn a local group ranking but damage global posting geometry.
- Strong listwise/pairwise pressure increases Recall@100 in some cases but
  hurts top-rank calibration.
- Tied dual heads remain unstable; even a small query adapter is too much for
  this stage.
- e48 overtrains relative to e24: NDCG@10 drops from `0.42356` to `0.41798`.

The best run improves BM25-free quality but not BM25 fusion. That means the
posting encoder itself is better, while the posthoc fusion/calibration surface
still needs separate work.

## Decision

Continue from M398, but narrow the search:

- keep shared linear geometry;
- keep mild listwise/pairwise dense-order loss;
- do not continue deep MLP heads for now;
- do not continue tied dual heads for now;
- stop training around the e24 regime unless a validation signal is added;
- treat BM25-aware training as a separate M399 surface.

## M399 Direction

The next model should separate admission and ranking:

- admission objective: preserve dense top-k coverage under fixed posting touch;
- ranking objective: calibrate scores only inside admitted candidates;
- fusion objective: learn or calibrate BM25 posthoc blending without using qrels;
- validation: track BM25-free NDCG, Dense O@100, and posthoc BM25 NDCG as three
  separate gates.

This is likely better than forcing one posting dot product to solve admission,
ranking, and BM25 calibration at the same time.
