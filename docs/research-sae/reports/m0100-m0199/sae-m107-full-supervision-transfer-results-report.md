# SAE M107 Full-Supervision Transfer Results Report

Date: 2026-05-23

## Decision

M107 scales the clean M106C route from 2,000 M81 train queries to the full M81
train split and confirms that the larger-supervision postings Stage-B ranker
is useful. It improves its own full-train validation/holdout/eval surfaces and
also transfers back to the original M105/M97 postings-generated eval surface
without retraining.

The result is positive but not a final SQL-facing ranker decision. On the M97
transfer surface, M107 improves fixed BM25+SAE, but its ranking delta is smaller
than M105's original in-surface ranker delta. This means more supervision helps
generalization, but we should not replace the compact M105 contract until a
mixed-surface training/selection pass combines M105's M97-local ranking strength
with M107's broader M81 supervision.

## Runs

Materialization:
`/home/huoju/leask/runs/m107-m81-fulltrain-m96-materialized`

Stage-B training:
`/home/huoju/leask/runs/m107-m81-fulltrain-stageb-v1`

M97 transfer evaluation:
`/home/huoju/leask/runs/m107-transfer-to-m97-v1`

## Data

| Surface | Documents | Queries | Runtime Rows | Candidate Configs |
| --- | ---: | ---: | ---: | --- |
| M105/M97 eval surface | 30,059 | 886 | 3,544 | `d8_p16`, `d8_p32`, `d16_p16`, `d16_p32` |
| M106 train-2000 surface | 55,118 | 2,000 | 8,000 | `d8_p16`, `d8_p32`, `d16_p16`, `d16_p32` |
| M107 full M81 train surface | 71,417 | 3,920 | 15,680 | `d8_p16`, `d8_p32`, `d16_p16`, `d16_p32` |

All M107 candidate configs use BM25 top-100 and the current `m96_k512`
representation. The recommended compact runtime config remains
`d8_p16_bm25100`.

## Full M81 Train Surface

M107 trains the same M105 multiconfig postings Stage-B ranker over the full M81
train surface. Best epoch is `240`; elapsed training time is `922.38s` on Spark
CUDA.

Average eval metrics on the M107 full-train materialized surface:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Fixed BM25+SAE | 0.4104 | 0.4748 | 0.3844 | 0.2778 |
| M107 Stage-B ranker | 0.4133 | 0.4862 | 0.3911 | 0.2819 |
| Delta | +0.0029 | +0.0114 | +0.0067 | +0.0041 |

Average holdout metrics on the same surface:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Fixed BM25+SAE | 0.4171 | 0.4897 | 0.4004 | 0.2870 |
| M107 Stage-B ranker | 0.4202 | 0.5019 | 0.4077 | 0.2913 |
| Delta | +0.0031 | +0.0123 | +0.0072 | +0.0044 |

## Transfer Back To M105/M97

The exported M107 ranker was evaluated on the original M105/M97
postings-generated surface without retraining.

Average eval metrics:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Fixed BM25+SAE | 0.4425 | 0.5226 | 0.4297 | 0.2956 |
| M107 transfer ranker | 0.4464 | 0.5285 | 0.4319 | 0.2976 |
| Delta | +0.0040 | +0.0058 | +0.0022 | +0.0020 |

Average holdout metrics:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Fixed BM25+SAE | 0.4277 | 0.5342 | 0.4314 | 0.2905 |
| M107 transfer ranker | 0.4313 | 0.5397 | 0.4341 | 0.2939 |
| Delta | +0.0036 | +0.0054 | +0.0027 | +0.0034 |

## Comparison To M105

On the same M105/M97 eval surface:

| Run | Recall@100 Delta | MRR@20 Delta | NDCG@10 Delta | MAP@100 Delta |
| --- | ---: | ---: | ---: | ---: |
| M105 in-surface ranker | +0.0022 | +0.0100 | +0.0044 | +0.0047 |
| M107 transfer ranker | +0.0040 | +0.0058 | +0.0022 | +0.0020 |

M107 transfers safely and improves Recall more than M105, but it gives back
roughly half of M105's MRR/NDCG/MAP improvement on the M97 surface. The likely
reason is distribution selection rather than model capacity: M107 is selected
against the larger M81 full-train surface, while M105 is directly tuned and
validated on the M97 query/candidate distribution.

## Interpretation

M107 confirms three points:

1. The Stage-B ranker benefits from more clean human-qrel supervision.
2. The gain is not purely overfit to the larger surface; the export still
   improves the original M97 surface without retraining.
3. The current model-selection target is still too single-surface. A larger
   training surface improves generalization, but local M97 ranking quality is
   better preserved by M105.

## Next Step

M108 should train/select over a mixed objective:

1. Keep M107 full M81 train rows as the main supervision source.
2. Add M105/M97-style rows as an explicit validation/selection surface.
3. Select checkpoints by a two-surface score: full-M81 eval must remain
   positive, and M97 transfer NDCG/MAP must recover most of M105's delta.
4. Keep `d8_p16_bm25100` as the compact runtime target unless a larger config
   gives a clear quality/cost Pareto improvement.

Only after M108 should we resume SQL-facing runtime contract work.
