# SAE M106 Stage-B Supervision Scale Results Report

Date: 2026-05-22

## Decision

M106 confirms the user's hypothesis in a controlled way: Stage-B ranking
calibration benefits more from additional human qrel supervision under the same
runtime representation than from simply increasing qrel loss weight or adding
larger hard-negative candidate pools on the same 886-query surface.

The promoted direction is not SQL productization yet. The next model step
should scale the same clean path from the M81 train-2000 probe to the full M81
train surface, then re-check compact runtime configs.

## Experiments

### M106A: Qrel-Hard Weight Probe

Run:
`/home/huoju/leask/runs/m106-qrel-hard-v1`

This keeps the M105 four candidate configs and increases qrel/pairwise pressure:

- `qrel_weight=1.25`
- `teacher_weight=0.35`
- `pairwise_weight=0.80`
- `pairwise_negatives=128`
- `regularization_weight=0.020`

Result: no useful improvement over M105. Eval average NDCG delta vs fixed
remains about `+0.0043`, slightly below M105's `+0.0044`.

### M106B: Expanded Hard-Negative Surface

Run:
`/home/huoju/leask/runs/m106-expanded-hardneg-v1`

This adds large candidate configs:

- `d8_p64_bm25200`
- `d16_p64_bm25200`
- `d32_p32_bm25200`

Result: larger configs get higher recall/MAP because they open more documents,
but the compact `d8_p16_bm25100` result is effectively unchanged. This means
extra hard-negative exposure on the same 886-query surface is not enough.

### M106C: M81 Train-2000 Qrel Scale Probe

Materialization run:
`/home/huoju/leask/runs/m106-m81-train2000-m96-materialized`

Stage-B run:
`/home/huoju/leask/runs/m106-m81-train2000-stageb-v1`

This re-materializes 2,000 M81 train candidate rows with the current
`m96_k512` representation, keeping the same runtime feature family as M105.
It does not mix in old `shared_sae_8192/12288/16384` artifacts.

## Data Comparison

| Run | Docs | Queries | Runtime Rows | Train Rows | Validation Rows | Holdout Rows |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M105 | 30,059 | 886 | 3,544 | 2,468 | 532 | 544 |
| M106C | 55,118 | 2,000 | 8,000 | 5,588 | 1,196 | 1,216 |

## Average Metrics

| Run | Split | Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| M105 | eval | fixed BM25+SAE | 0.4425 | 0.5226 | 0.4297 | 0.2956 |
| M105 | eval | Stage-B ranker | 0.4447 | 0.5327 | 0.4340 | 0.3003 |
| M106C | eval | fixed BM25+SAE | 0.4439 | 0.5451 | 0.4505 | 0.3221 |
| M106C | eval | Stage-B ranker | 0.4457 | 0.5550 | 0.4575 | 0.3265 |

## Delta vs Fixed BM25+SAE

| Run | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| M105 | eval | +0.0022 | +0.0100 | +0.0044 | +0.0047 |
| M106A | eval | +0.0022 | +0.0100 | +0.0043 | +0.0047 |
| M106C | validation | +0.0018 | +0.0104 | +0.0077 | +0.0046 |
| M106C | holdout | +0.0019 | +0.0094 | +0.0064 | +0.0042 |
| M106C | eval | +0.0018 | +0.0099 | +0.0071 | +0.0044 |
| M106C | all | +0.0026 | +0.0099 | +0.0074 | +0.0070 |

## Compact Config Check

The compact runtime candidate remains stable:

| Run | Config | Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean Candidates |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| M105 | `d8_p16_bm25100` | eval | 0.4367 | 0.5317 | 0.4334 | 0.2986 | 204.3 |
| M106C | `d8_p16_bm25100` | eval | 0.4369 | 0.5521 | 0.4556 | 0.3240 | 202.0 |

The absolute metrics are not directly comparable across M105 and M106C because
M106C uses a different M81 train-derived surface. The important comparable
signal is the delta against fixed BM25+SAE on the same surface. M106C improves
eval NDCG delta from M105's `+0.0044` to `+0.0071`.

## Interpretation

M106 separates three possible explanations:

1. More qrel weight alone does not help.
2. More hard negatives on the same small query set does not help compact
   runtime ranking.
3. More human-labeled queries under the current M96/k512 representation does
   help ranking calibration, especially NDCG.

This means the Stage-B ranker is still supervision-limited. The model shape is
not the immediate blocker; the training surface is.

## Next Step

M107 should scale the clean M106C route:

1. Materialize the full M81 train split with `m96_k512`, not only 2,000 rows.
2. Train the postings Stage-B ranker on the full train surface.
3. Evaluate both on the new full-train holdout and on the original M105/M97
   eval surface to check cross-surface generalization.
4. Only if M107 improves the original M105/M97 eval surface should we resume
   SQL-facing runtime work.
