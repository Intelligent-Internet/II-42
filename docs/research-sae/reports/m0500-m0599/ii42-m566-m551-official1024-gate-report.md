# M566 M551 Official-1024 Gate

M566 moves the broader M551 gate from the older 768-dimensional BEIR shared
root to the official 1024-dimensional root:

`/home/huoju/leask/runs/ii42-m310b-official-roots-v1`

This root already contains `documents.jsonl`, `queries.jsonl`, and
`quality_qrels.json`, so the M565 adapter can validate and symlink it without
changing the M551 training logic.

## Adapter Surface

- Source root:
  `/home/huoju/leask/runs/ii42-m310b-official-roots-v1`
- Generated shared root:
  `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`
- Manifest:
  `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/m566_official1024_root_manifest.json`
- Local manifest copy:
  `outputs/m566_official1024_root/m566_official1024_root_manifest.json`

The adapter validated eight datasets, all with 1024-dimensional embeddings.

| Dataset | Docs | Queries | Qrel Queries | Qrel Pairs |
| --- | ---: | ---: | ---: | ---: |
| arguana | 8674 | 1401 | 1401 | 1401 |
| nfcorpus | 3633 | 323 | 323 | 12334 |
| fiqa | 57638 | 648 | 648 | 1706 |
| scidocs | 25657 | 1000 | 1000 | 4928 |
| scifact | 5183 | 300 | 300 | 339 |
| trec-covid | 171331 | 50 | 50 | 24673 |
| webis-touche2020 | 382545 | 49 | 49 | 932 |
| cqadupstack | 457199 | 13145 | 13145 | 23703 |

For the first run, `cqadupstack` was intentionally excluded because it dominates
query count.  The first gate is BEIR7; `cqadupstack` should be added only after
the BEIR7 seed surface is stable.

## Three-Seed Result

Run outputs:

- `runs/m566_m551_official1024_beir7_seed551/m551_m566_m551_resid025_official1024_beir7_seed551.json`
- `runs/m566_m551_official1024_beir7_seed552/m551_m566_m551_resid025_official1024_beir7_seed552.json`
- `runs/m566_m551_official1024_beir7_seed553/m551_m566_m551_resid025_official1024_beir7_seed553.json`

Configuration is the promoted M551 setting:

- `ACTIVE_DIMS=128`
- `TEACHER_POOL_K=128`
- `TEACHER_TEMPERATURE=0.025`
- `STUDENT_TEMPERATURE=0.050`
- `RANDOM_NEGATIVES=128`
- `MAX_TRAIN_GROUPS=512`
- `EPOCHS=4`
- `HIDDEN_DIMS=384`
- `RESIDUAL_SCALE=0.025`
- `SUPPORT_WEIGHT=0.5`
- `SCORE_WEIGHT=0.05`

Mean absolute macro:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense_topk128_sparse | 0.43309 | 0.253517 | 0.547637 | 0.532273 | 0.544840 |
| M551 residual | 0.43530 | 0.256517 | 0.551427 | 0.530740 | 0.545993 |

Deltas are relative to `dense_topk128_sparse` on the same seed and task root.

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.00579 | +0.00425 | +0.00560 | +0.00334 | +0.00240 |
| 552 | +0.00331 | +0.00175 | -0.00019 | -0.00196 | +0.00188 |
| 553 | -0.00247 | +0.00300 | +0.00596 | -0.00598 | -0.00082 |

| Metric | Mean Delta | Std | Min | Max | Positive Seeds |
| --- | ---: | ---: | ---: | ---: | ---: |
| NDCG@10 | +0.002210 | 0.003461 | -0.002470 | +0.005790 | 2/3 |
| MAP@100 | +0.003000 | 0.001021 | +0.001750 | +0.004250 | 3/3 |
| Recall@100 | +0.003790 | 0.002818 | -0.000190 | +0.005960 | 2/3 |
| MRR@20 | -0.001533 | 0.003817 | -0.005980 | +0.003340 | 1/3 |
| Overlap@100 | +0.001153 | 0.001411 | -0.000820 | +0.002400 | 2/3 |

Mean per-task deltas over seeds:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | +0.002737 | +0.002123 | +0.001193 | +0.002370 | -0.000810 |
| fiqa | +0.004230 | +0.005320 | +0.003727 | +0.000220 | +0.002397 |
| nfcorpus | +0.000747 | -0.001183 | +0.000997 | -0.001300 | -0.000257 |
| scidocs | -0.001817 | -0.000670 | +0.001567 | -0.007170 | +0.001407 |
| scifact | +0.015773 | +0.015833 | +0.012500 | +0.015513 | +0.005473 |
| trec-covid | +0.001393 | +0.000700 | +0.000947 | -0.010553 | +0.000833 |
| webis-touche2020 | -0.007617 | -0.001143 | +0.005617 | -0.009810 | -0.001000 |

Seed-task counts:

| Metric | Positive | Zero | Negative | Worst |
| --- | ---: | ---: | ---: | --- |
| NDCG@10 | 13 | 0 | 8 | webis-touche2020 seed553 -0.027510 |
| MAP@100 | 14 | 0 | 7 | webis-touche2020 seed553 -0.006150 |
| Recall@100 | 17 | 0 | 4 | nfcorpus seed552 -0.007010 |
| MRR@20 | 9 | 0 | 12 | webis-touche2020 seed553 -0.036950 |
| Overlap@100 | 14 | 0 | 7 | webis-touche2020 seed553 -0.008000 |

## Current Interpretation

M566 is a stronger and more relevant signal than M565 because it uses the
1024-dimensional official root.  It is still not a final version.

What improved:

- MAP@100 is positive on all three seeds.
- Recall@100 is positive on average and positive on two of three seeds.
- NDCG@10 is positive on average and positive on two of three seeds.
- Overlap@100 is slightly positive on average.

What is still weak:

- MRR@20 is negative on average.
- `webis-touche2020` is unstable and is the worst task for NDCG, MAP, MRR, and
  overlap.
- `trec-covid` is recall-positive but MRR-negative.

The route is therefore alive and worth continuing, but promotion requires the
next successor to add task-stability and early-rank constraints.  The best next
step is M567: keep the M551 promoted baseline, but add a conservative selection
gate that accepts a trained residual only when it preserves Recall@100 and does
not materially damage MRR@20 on validation.  `cqadupstack` should be added after
that because it has 13,145 queries and would otherwise dominate the runtime
before the instability is addressed.

Do not spend more effort on the DREAM-attention branch until this
M551-family recall/MRR stability question is resolved.
