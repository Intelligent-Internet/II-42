# II-42 M394/M395 MTEB Robustness Report

## Summary

M394/M395 tests whether the M393 MTEB result is robust or just a lucky
single-seed/tuned-alpha result.

The test keeps the same materialized 10-task MTEB English retrieval surface and
the same qrels-free retrieval route:

- dense-coordinate admission: `pca_doc`, active dims `128`, prefix `256`
- dense rerank: active coordinate dot plus joint-PCA tail sketch
- tail sketch: `256` dims, int8 document sketch
- candidate budgets: dense coordinate `0.08`, BM25 `0.08`
- fusion: global z-score blend, no per-dataset policy
- scoring: MTEB-style graded qrels, main score `ndcg_at_10`
- seeds: `393`, `1393`, `2393`

No qrels are used for projection, admission, or fusion-policy construction.
The sweep reads qrels only for final evaluation.

## Artifacts

- Eval script: `scripts/research_sae_m393_mteb_tail_bm25_eval.py`
- Runner: `scripts/run_m394_m395_mteb_full_spark.sh`
- Aggregator: `scripts/research_sae_m394_m395_mteb_aggregate.py`
- Remote run root:
  `/home/huoju/leask/runs/mteb-m394-m395-robustness-v1`
- Remote aggregate JSON:
  `/home/huoju/leask/runs/mteb-m394-m395-robustness-v1/_report/m394_m395_mteb_aggregate.json`
- Remote aggregate report:
  `/home/huoju/leask/runs/mteb-m394-m395-robustness-v1/_report/m394_m395_mteb_aggregate.md`
- Remote log:
  `/home/huoju/leask/logs/mteb_m394_m395_full.log`

## Source Robustness

| Source | Mean NDCG@10 | Std | Min | Max | Seeds |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m394_bm25_tail_union_zblend_a015` | 0.58786 | 0.00058 | 0.58731 | 0.58867 | 3 |
| `m395_gate_rescue` | 0.58510 | 0.00026 | 0.58479 | 0.58543 | 3 |
| `m394_bm25_tail_union_zblend_a010` | 0.58399 | 0.00043 | 0.58339 | 0.58436 | 3 |
| `m395_gate_overlap_margin` | 0.58224 | 0.00014 | 0.58212 | 0.58243 | 3 |
| `m394_bm25_tail_union_zblend_a008` | 0.58177 | 0.00032 | 0.58132 | 0.58200 | 3 |
| `m394_bm25_tail_union_zblend_a005` | 0.57801 | 0.00026 | 0.57764 | 0.57822 | 3 |
| `m394_bm25_tail_union_zblend_a003` | 0.57427 | 0.00042 | 0.57367 | 0.57461 | 3 |
| `m394_bm25_tail_union_zblend_a000` | 0.56730 | 0.00069 | 0.56635 | 0.56798 | 3 |
| `m393_runtime_tail_256` | 0.46034 | 0.00039 | 0.46004 | 0.46089 | 3 |
| `m393_bm25` | 0.40515 | 0.00000 | 0.40515 | 0.40515 | 3 |

## Baseline Comparison

All rows are on the same materialized 10-task MTEB retrieval surface.

| Source | Main NDCG@10 | Delta vs M394 winner |
| --- | ---: | ---: |
| `m394_bm25_tail_union_zblend_a015` | 0.58786 | 0.00000 |
| `simple_c6/bm25_dense_score_fusion` | 0.58275 | +0.00511 |
| `simple_c6/dense` | 0.57263 | +0.01523 |
| `m310_surface/lin_latent1_bm250p75_sae1` | 0.52695 | +0.06091 |
| `simple_c6/bm25_sae_score_fusion` | 0.52603 | +0.06183 |
| `simple_c6/sae` | 0.48474 | +0.10312 |
| `simple_c6/bm25` | 0.39282 | +0.19504 |

## Per-Task Winner Mean

| Task | Mean NDCG@10 | Std | Min | Max |
| --- | ---: | ---: | ---: | ---: |
| `ArguAna` | 0.43742 | 0.00016 | 0.43727 | 0.43764 |
| `CQADupstackGamingRetrieval` | 0.62371 | 0.00018 | 0.62350 | 0.62394 |
| `CQADupstackUnixRetrieval` | 0.47248 | 0.00078 | 0.47148 | 0.47337 |
| `ClimateFEVERHardNegatives` | 0.38807 | 0.00126 | 0.38709 | 0.38985 |
| `FEVERHardNegatives` | 0.91828 | 0.00049 | 0.91764 | 0.91882 |
| `FiQA2018` | 0.51907 | 0.00105 | 0.51789 | 0.52044 |
| `HotpotQAHardNegatives` | 0.76755 | 0.00112 | 0.76621 | 0.76895 |
| `SCIDOCS` | 0.22070 | 0.00046 | 0.22029 | 0.22135 |
| `TRECCOVID` | 0.81672 | 0.00462 | 0.81180 | 0.82291 |
| `Touche2020Retrieval.v3` | 0.71463 | 0.00083 | 0.71372 | 0.71573 |

## Verdict

M394 turns the M393 narrow single-seed win into a stable result. The best tested
global policy is fixed BM25 blend alpha `0.15`, with mean NDCG@10 `0.58786`
and seed std `0.00058`.

This is `+0.00511` above the same-surface `simple_c6/bm25_dense_score_fusion`
baseline and `+0.01523` above dense alone. That is a materially stronger margin
than M393's original `+0.00064` single-seed result.

M395's qrels-free gate policies are useful diagnostics but not promotions:
`m395_gate_rescue` reaches `0.58510`, still below fixed `alpha=0.15`.

## M396 Alpha Frontier

M396 extends the global alpha range above `0.15`, again with the same three
projection seeds and no dataset-specific tuning.

Remote artifacts:

- Remote run root:
  `/home/huoju/leask/runs/mteb-m396-alpha-frontier-v1`
- Remote aggregate JSON:
  `/home/huoju/leask/runs/mteb-m396-alpha-frontier-v1/_report/m396_alpha_frontier_aggregate.json`
- Remote aggregate report:
  `/home/huoju/leask/runs/mteb-m396-alpha-frontier-v1/_report/m396_alpha_frontier_aggregate.md`
- Remote log:
  `/home/huoju/leask/logs/mteb_m396_alpha_frontier.log`

| Source | Mean NDCG@10 | Std | Min | Max | Seeds |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m394_bm25_tail_union_zblend_a018` | 0.58804 | 0.00099 | 0.58720 | 0.58943 | 3 |
| `m394_bm25_tail_union_zblend_a015` | 0.58791 | 0.00065 | 0.58731 | 0.58882 | 3 |
| `m394_bm25_tail_union_zblend_a020` | 0.58786 | 0.00071 | 0.58704 | 0.58878 | 3 |
| `m394_bm25_tail_union_zblend_a025` | 0.58546 | 0.00025 | 0.58511 | 0.58564 | 3 |
| `m395_gate_rescue` | 0.58511 | 0.00027 | 0.58479 | 0.58544 | 3 |
| `m395_gate_overlap_margin` | 0.58219 | 0.00019 | 0.58197 | 0.58243 | 3 |
| `m394_bm25_tail_union_zblend_a030` | 0.58126 | 0.00023 | 0.58096 | 0.58152 | 3 |

M396 closes the high-alpha question. The best tested global policy is now
`alpha=0.18`, but the win over `alpha=0.15` is small: `+0.00013` macro
NDCG@10. The important result is not that `0.18` is magic; it is that the
frontier is around `0.15-0.20` and degrades clearly by `0.25-0.30`.

Relative to same-surface baselines, the M396 winner is:

| Source | Main NDCG@10 | Delta vs M396 winner |
| --- | ---: | ---: |
| `m394_bm25_tail_union_zblend_a018` | 0.58804 | 0.00000 |
| `simple_c6/bm25_dense_score_fusion` | 0.58275 | +0.00529 |
| `simple_c6/dense` | 0.57263 | +0.01541 |
| `m310_surface/lin_latent1_bm250p75_sae1` | 0.52695 | +0.06109 |

## Final Verdict

The current best version from this research line is the M396 global alpha
frontier winner:

- source: `m394_bm25_tail_union_zblend_a018`
- mean MTEB NDCG@10: `0.58804`
- std over three projection seeds: `0.00099`
- margin over same-surface dense+BM25: `+0.00529`
- margin over same-surface dense: `+0.01541`

This is a real but modest improvement over dense+BM25 on the current 10-task
MTEB surface. It is not a dataset-specific policy and it does not use qrels to
construct the retrieval route, but the exact alpha was selected by this eval
sweep, so it should be treated as a research winner rather than a locked product
constant until confirmed on another held-out eval surface.

The most useful next step is not another alpha sweep. The route has reached a
clear local frontier: `0.15-0.20` is the useful band, and higher alpha hurts.
Further progress should come from improving the dense-tail posting route itself
or from a qrels-free policy that predicts when to use this band, not from
per-dataset tuning.
