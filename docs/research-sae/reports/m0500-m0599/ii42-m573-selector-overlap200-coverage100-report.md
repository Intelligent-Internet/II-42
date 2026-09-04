# M573 Selector Overlap200 + Coverage100 Gate

M572 proved that the selector-split guarded source works mechanically, but
`overlap@100 >= 0` plus `coverage@100 >= 0` did not fix seed551 Recall@100.
M573 tests a stricter, still qrels-free selector rule:

- accept learned source only if selector `overlap_at_200` delta is at least
  `+0.002`;
- and selector `candidate_coverage_at_100` delta is at least `+0.002`.

The rule was chosen after inspecting M572 seed551, so seed551 is not sufficient
evidence.  The next evidence must come from seed552 and seed553.

## Seed551 Backtest From M572 Stats

The M572 seed551 run already exported multi-cutoff selector stats.  Applying
the M573 rule to those stats would accept only:

- `scifact`
- `trec-covid`
- `webis-touche2020`

Backtested deltas versus `dense_topk128_sparse`:

| Rule | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M570 overlap@100 >= 0 | +0.005706 | +0.003467 | -0.000636 | +0.010326 | +0.001170 |
| M573 overlap@200 >= .002 + coverage@100 >= .002 | +0.005241 | +0.002823 | +0.001279 | +0.010224 | +0.001667 |

This is the first selector rule that keeps all five macro deltas positive on
the selector-split seed551 surface.

## Formal Validation

Completed on `spark-1`:

- `runs/m573_selector_beir7_seed552/`
- `runs/m573_selector_beir7_seed553/`

Formal deltas versus `dense_topk128_sparse`:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 552 | learned | +0.008590 | +0.004930 | -0.001540 | +0.007620 | +0.001520 |
| 552 | guarded | +0.003320 | +0.002960 | +0.000000 | +0.001130 | +0.000350 |
| 553 | learned | +0.001240 | +0.005150 | +0.009370 | -0.001370 | -0.000660 |
| 553 | guarded | +0.002970 | +0.005100 | +0.007880 | +0.003190 | -0.000270 |
| 552-553 mean | guarded | +0.003145 | +0.004030 | +0.003940 | +0.002160 | +0.000040 |

Guarded accepted tasks:

| Seed | Accepted learned source |
| --- | --- |
| 552 | `scifact`, `webis-touche2020` |
| 553 | `fiqa`, `scifact`, `webis-touche2020` |

## Interpretation

M573 is the first independent-selector rule in this line that is conservative
enough to avoid the Recall regression seen in M570/M572 while keeping positive
macro NDCG, MAP, Recall, and MRR over the two validation seeds.

The signal is still bounded.  The guarded two-seed mean overlap gain is only
`+0.000040`, and seed553 overlap is slightly negative.  This means the DREAM-lite
candidate-competition idea is useful as a training and selection signal, but
the current selector gate is not yet a final promotion rule.

Next stage should keep the same qrels-free selector split and test a stronger
selector utility target, not jump directly to BM25 fusion:

1. Keep M573 as the current safe baseline.
2. Add selector-side distribution fit, such as candidate-set KL or top-rank
   teacher mass, as a third gate next to overlap@200 and coverage@100.
3. Promote only if seed551/552/553 all keep nonnegative Recall and improve
   at least NDCG/MAP/MRR without relying on dataset labels.
