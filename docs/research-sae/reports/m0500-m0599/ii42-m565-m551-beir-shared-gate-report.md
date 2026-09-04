# M565 M551 BEIR Shared-Root Gate

M565 tests whether the promoted BM25-free M551 configuration still carries a
positive signal on a wider BEIR-style shared root.  It deliberately does not
change the M551 training logic.  The only new code is a shared-root adapter that
validates and symlinks an existing BEIR materialization into the task-root shape
expected by M551.

## Data Surface

- Source root:
  `/home/huoju/leask/data/psql_bm25s_sae_beir15_shared`
- Generated shared root:
  `/home/huoju/leask/runs/ii42-m565-m551-beir-shared-root-v1/_shared/tasks`
- Manifest:
  `/home/huoju/leask/runs/ii42-m565-m551-beir-shared-root-v1/_shared/m565_beir_shared_root_manifest.json`
- Local manifest copy:
  `outputs/m565_beir_shared_root_manifest.json`

The validated BEIR8 gate contains 31,600 documents, 699 queries, and 699
covered qrel queries.  All eight datasets use 768-dimensional embeddings.

| Dataset | Docs | Queries | Qrel Queries | Qrel Pairs |
| --- | ---: | ---: | ---: | ---: |
| arguana | 2000 | 100 | 100 | 100 |
| nfcorpus | 2063 | 100 | 100 | 3818 |
| fiqa | 2000 | 100 | 100 | 267 |
| scidocs | 2000 | 100 | 100 | 492 |
| scifact | 2000 | 100 | 100 | 116 |
| trec-covid | 17537 | 50 | 50 | 24673 |
| webis-touche2020 | 2000 | 49 | 49 | 932 |
| cqadupstack | 2000 | 100 | 100 | 787 |

Important caveat: this is a broader stability gate, not an apples-to-apples
replacement for the M551 broad10 root.  The M551 promoted broad10 evidence used
the MTEB shared root with 1024-dimensional rows.  This BEIR shared root uses
768-dimensional rows, so M565 should be interpreted as an additional robustness
surface.

## Configuration

All three runs used the promoted M551 settings:

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
- `MIN_ACTIVE_RECALL=0.995`

Run outputs:

- `runs/m565_m551_beir8_seed551/m551_m565_m551_resid025_beir8_seed551.json`
- `runs/m565_m551_beir8_seed552/m551_m565_m551_resid025_beir8_seed552.json`
- `runs/m565_m551_beir8_seed553/m551_m565_m551_resid025_beir8_seed553.json`

## Three-Seed Macro Delta

Deltas are relative to `dense_topk128_sparse` on the same seed and task root.

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.00117 | +0.00126 | -0.00090 | -0.00068 | -0.00069 |
| 552 | +0.00519 | +0.00587 | -0.00019 | +0.00641 | +0.00062 |
| 553 | +0.00325 | +0.00449 | -0.00053 | +0.00429 | +0.00112 |

| Metric | Mean Delta | Std | Min | Max | Positive Seeds |
| --- | ---: | ---: | ---: | ---: | ---: |
| NDCG@10 | +0.003203 | 0.001641 | +0.001170 | +0.005190 | 3/3 |
| MAP@100 | +0.003873 | 0.001932 | +0.001260 | +0.005870 | 3/3 |
| Recall@100 | -0.000540 | 0.000290 | -0.000900 | -0.000190 | 0/3 |
| MRR@20 | +0.003340 | 0.002971 | -0.000680 | +0.006410 | 2/3 |
| Overlap@100 | +0.000350 | 0.000763 | -0.000690 | +0.001120 | 2/3 |

Mean absolute macro:

| Metric | dense_topk128_sparse | M551 Residual |
| --- | ---: | ---: |
| NDCG@10 | 0.665140 | 0.668343 |
| MAP@100 | 0.507327 | 0.511200 |
| Recall@100 | 0.742433 | 0.741893 |
| MRR@20 | 0.759210 | 0.762550 |
| Overlap@100 | 0.645740 | 0.646090 |

## Task Stability

Mean deltas over seeds:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | +0.004403 | +0.005627 | +0.000000 | +0.005637 | +0.000333 |
| cqadupstack | +0.001750 | +0.001693 | +0.000370 | +0.001017 | +0.000750 |
| fiqa | -0.000430 | +0.000443 | -0.001387 | +0.002233 | +0.000250 |
| nfcorpus | +0.000530 | -0.000040 | -0.000283 | -0.003840 | +0.001750 |
| scidocs | +0.001253 | +0.000770 | -0.001667 | +0.000477 | -0.000500 |
| scifact | +0.017900 | +0.022037 | +0.000000 | +0.021197 | +0.002417 |
| trec-covid | +0.000787 | -0.000297 | +0.000460 | +0.000000 | -0.003000 |
| webis-touche2020 | -0.000523 | +0.000750 | -0.001853 | +0.000000 | +0.000833 |

Seed-task counts:

| Metric | Positive | Zero | Negative | Worst |
| --- | ---: | ---: | ---: | --- |
| NDCG@10 | 15 | 1 | 8 | trec-covid seed553 -0.006950 |
| MAP@100 | 17 | 1 | 6 | fiqa seed551 -0.004410 |
| Recall@100 | 4 | 11 | 9 | scidocs seed551 -0.005000 |
| MRR@20 | 10 | 8 | 6 | nfcorpus seed551 -0.012190 |
| Overlap@100 | 12 | 3 | 9 | trec-covid seed551 -0.004500 |

## Interpretation

M565 is a weak-positive broader-gate result for the M551 route:

- NDCG and MAP are positive on all three seeds.
- MRR is positive on two of three seeds.
- Dense overlap is slightly positive on average.
- Recall is consistently but very slightly negative.

This means the promoted M551 shape still has value outside the original MTEB
root, but it is not yet a final usable version.  The current loss improves
ranking distribution more reliably than it preserves recall.  That is a
training-objective issue, not evidence that the route is dead.

The next useful step is not another DREAM-attention micro-sweep.  It is either:

1. build the 1024-dimensional `m150-beir-full-pplx` qrels adapter so the broader
   gate is closer to the current PPLX/MTEB surface; or
2. add a recall-constrained M551 successor where promotion requires NDCG/MAP
   improvement without negative Recall@100.

Given the M565 result, the route should continue, but the next gate must make
recall preservation an explicit constraint.
