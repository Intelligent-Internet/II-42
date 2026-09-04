# M515 Query-Adaptive Route LoRA Report

M515 follows the M514 stop rule: stop changing the loss and test whether a
qrels-free query-time fanout controller can improve the p64/p96/p128 tradeoff.

The policy starts from p64 and raises an individual query to p96 or p128 when
its p64 touched-doc ratio is low.  It uses no BM25, no qrels, no dataset name,
and no dense score at query time.

## Run

| Run | Host | Task | Route docs | Adaptive presets | Output |
| --- | --- | --- | ---: | --- | --- |
| `fiqa_query_adaptive_route_lora` | `spark-1` | `FiQA2018` | `4096/57638` | `t045_055,t050_060,t055_065` | `outputs/m515/fiqa_query_adaptive_route_lora/m515_query_adaptive_route_lora.json` |

This is a route-subset gate, not a full-corpus BEIR score.

## Matrix

### FiQA Gate

| Source | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch | Prefix histogram |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `fixed_p64` | 0.46533 | 0.84621 | 0.57371 | 0.41271 | 0.82234 | 0.51523 | `{64: 64}` |
| `fixed_p96` | 0.46636 | 0.87786 | 0.57180 | 0.41645 | 0.89047 | 0.60552 | `{96: 64}` |
| `fixed_p128` | 0.46747 | 0.88620 | 0.57180 | 0.41864 | 0.92828 | 0.66954 | `{128: 64}` |
| `adaptive_t045_055` | 0.46642 | 0.87839 | 0.57272 | 0.41725 | 0.87187 | 0.58944 | `{64: 18, 96: 35, 128: 11}` |
| `adaptive_t050_060` | 0.46642 | 0.87526 | 0.57272 | 0.41670 | 0.90000 | 0.62891 | `{64: 7, 96: 22, 128: 35}` |
| `adaptive_t055_065` | 0.46747 | 0.88620 | 0.57180 | 0.41882 | 0.91578 | 0.64872 | `{64: 2, 96: 16, 128: 46}` |

### Broad4 Gate

Broad4 uses `ArguAna,FiQA2018,SCIDOCS,TRECCOVID` with the same controller.

| Source | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `fixed_p64` | 0.45897 | 0.63757 | 0.51738 | 0.23635 | 0.80189 | 0.52480 |
| `fixed_p96` | 0.46325 | 0.64856 | 0.51601 | 0.23956 | 0.85612 | 0.61422 |
| `fixed_p128` | 0.47076 | 0.65391 | 0.51779 | 0.24431 | 0.88653 | 0.67408 |
| `adaptive_t045_055` | 0.46691 | 0.64326 | 0.51516 | 0.24002 | 0.84657 | 0.56928 |
| `adaptive_t050_060` | 0.46663 | 0.64385 | 0.51452 | 0.23994 | 0.85540 | 0.58438 |
| `adaptive_t055_065` | 0.46664 | 0.64541 | 0.51387 | 0.24027 | 0.86470 | 0.60311 |

## Interpretation

M515 is more promising than M514 on the single-task FiQA gate, but the Broad4
check is not a promotion.

- `adaptive_t055_065` matches fixed p128 on NDCG@10 and Recall@100 while
  reducing Touch from 0.66954 to 0.64872.
- Candidate recall drops from 0.92828 to 0.91578, so this is not a full
  promotion yet.
- `adaptive_t050_060` is a lower-touch middle point: Candidate R@100 0.90000
  at Touch 0.62891, but it loses Recall@100 versus p128.
- On Broad4, fixed p128 remains the best quality point: NDCG@10 0.47076 and
  Recall@100 0.65391.
- Broad4 `adaptive_t055_065` reduces Touch from 0.67408 to 0.60311, but drops
  NDCG@10 to 0.46664 and Candidate R@100 to 0.86470.

The adaptive route creates a useful single-task Pareto point, but simple
fanout-count thresholds do not generalize strongly enough across tasks.  This
points to a more structured query-time controller instead of more training-loss
tweaks or hand-tuned thresholds.

## Next Step

M516 should keep the baseline route model fixed and test a richer qrels-free,
teacher-supervised adaptive policy:

1. score query hardness from p64 candidate count, query coordinate entropy, and
   top-coordinate load;
2. choose p64/p96/p128 with a small monotonic rule or a lightweight learned
   controller trained only to mimic dense-teacher candidate coverage;
3. train/select the controller on train-query dense-teacher candidate coverage
   and evaluate on heldout qrels;
4. require Broad4 improvement before any full promotion.

Promotion target for the next gate:

- NDCG@10 and Recall@100 at least fixed p128;
- Candidate R@100 close to fixed p128, ideally above 0.925;
- Touch below fixed p128 by at least 3-5%.
