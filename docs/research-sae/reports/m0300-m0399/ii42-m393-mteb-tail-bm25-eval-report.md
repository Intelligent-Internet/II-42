# II-42 M393 MTEB Tail-Sketch + BM25 Eval Report

## Summary

M393 ports the current M392 best route to the materialized MTEB English
retrieval evaluation surface:

- dense-coordinate admission: `pca_doc`, active dims `128`, prefix `256`
- dense rerank: active coordinate dot plus joint-PCA tail sketch
- tail sketch: `256` dims, int8 document sketch
- candidate budgets: dense coordinate `0.08`, BM25 `0.08`
- fusion: fixed z-score blend, BM25 alpha `0.10`
- scoring: MTEB-style graded qrels, main score `ndcg_at_10`
- no qrels are used for projection, admission, or fusion-policy selection

Full 10-task run completed on `spark-2`.

## Artifacts

- Script: `scripts/research_sae_m393_mteb_tail_bm25_eval.py`
- Remote run root:
  `/home/huoju/leask/runs/mteb-m393-tail-bm25-v1/full10`
- Remote JSON:
  `/home/huoju/leask/runs/mteb-m393-tail-bm25-v1/full10/m393_mteb_tail_bm25.json`
- Remote report:
  `/home/huoju/leask/runs/mteb-m393-tail-bm25-v1/full10/m393_mteb_tail_bm25.md`
- Remote log:
  `/home/huoju/leask/logs/mteb_m393_tail_bm25_full10.log`

## Macro

| Source | Main NDCG@10 | Tasks |
| --- | ---: | ---: |
| `m393_bm25_tail_union_zblend_a010` | 0.58339 | 10 |
| `m393_runtime_tail_256` | 0.46009 | 10 |
| `m393_bm25` | 0.40515 | 10 |

## Baseline Comparison

All rows are on the same materialized 10-task MTEB retrieval surface.

| Source | Main NDCG@10 | Delta vs M393 |
| --- | ---: | ---: |
| `m393_bm25_tail_union_zblend_a010` | 0.58339 | 0.00000 |
| `simple_c6/bm25_dense_score_fusion` | 0.58275 | +0.00064 |
| `simple_c6/dense` | 0.57263 | +0.01076 |
| `m310_surface/lin_latent1_bm250p75_sae1` | 0.52695 | +0.05644 |
| `simple_c6/bm25_sae_score_fusion` | 0.52603 | +0.05736 |
| `simple_c6/sae` | 0.48474 | +0.09865 |
| `simple_c6/bm25` | 0.39282 | +0.19057 |

## Per-Task M393

| Task | NDCG@10 | MAP@100 | R@100 | MRR@10 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| `ArguAna` | 0.44148 | 0.30831 | 1.00000 | 0.30080 | 0.13447 |
| `CQADupstackGamingRetrieval` | 0.62352 | 0.57735 | 0.89335 | 0.60608 | 0.14138 |
| `CQADupstackUnixRetrieval` | 0.47055 | 0.42866 | 0.81740 | 0.45604 | 0.13986 |
| `ClimateFEVERHardNegatives` | 0.38702 | 0.30693 | 0.70495 | 0.50799 | 0.14140 |
| `FEVERHardNegatives` | 0.91648 | 0.88933 | 0.98587 | 0.92578 | 0.14844 |
| `FiQA2018` | 0.52048 | 0.46101 | 0.81191 | 0.60570 | 0.14520 |
| `HotpotQAHardNegatives` | 0.75998 | 0.68959 | 0.90800 | 0.89428 | 0.15056 |
| `SCIDOCS` | 0.22303 | 0.15787 | 0.49173 | 0.36880 | 0.13534 |
| `TRECCOVID` | 0.80373 | 0.13266 | 0.16288 | 0.92500 | 0.15294 |
| `Touche2020Retrieval.v3` | 0.68767 | 0.45183 | 0.69700 | 0.94898 | 0.15337 |

## Per-Task Delta vs Simple Dense+BM25

| Task | M393 | Dense+BM25 | Delta |
| --- | ---: | ---: | ---: |
| `ArguAna` | 0.44148 | 0.45439 | -0.01291 |
| `CQADupstackGamingRetrieval` | 0.62352 | 0.62204 | +0.00148 |
| `CQADupstackUnixRetrieval` | 0.47055 | 0.46138 | +0.00917 |
| `ClimateFEVERHardNegatives` | 0.38702 | 0.36637 | +0.02065 |
| `FEVERHardNegatives` | 0.91648 | 0.90026 | +0.01622 |
| `FiQA2018` | 0.52048 | 0.50827 | +0.01221 |
| `HotpotQAHardNegatives` | 0.75998 | 0.77359 | -0.01361 |
| `SCIDOCS` | 0.22303 | 0.21961 | +0.00342 |
| `TRECCOVID` | 0.80373 | 0.80692 | -0.00319 |
| `Touche2020Retrieval.v3` | 0.68767 | 0.71466 | -0.02699 |

## Verdict

M393 is a real external-surface validation of the M392 route. It is slightly
above the existing dense+BM25 score-fusion baseline on macro NDCG@10, while
touching only about `0.13-0.15` of documents per task for the unified source.

The win over dense+BM25 is narrow (`+0.00064`), so it should not be treated as
a robust promotion by itself. The more important result is that the
posting-shaped M392 route reaches dense+BM25 quality on the MTEB retrieval
surface and is far above the prior SAE/posting M310-style surfaces.

Next useful checks:

- repeat with two more projection seeds;
- sweep alpha around `0.05`, `0.10`, `0.15` on MTEB;
- test whether task-agnostic alpha can reduce the losses on `ArguAna`,
  `HotpotQAHardNegatives`, and `Touche2020Retrieval.v3` without hurting FEVER
  and CQADupstack.
