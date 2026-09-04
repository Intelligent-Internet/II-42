# ii42 M1118 Shared5 Profile Export Plan

## Objective

Scale the M1110-M1117 scoring-geometry replay from the original three-dataset
export surface to a slightly broader shared5 smoke surface.

This is not a revival of the semantic-neighbor branch. Semantic pair mining is
disabled. The run exists only to produce candidate-score exports that can be
used to replay:

- M1110 fixed profile grid;
- M1114 boundary trace audit;
- M1115 rank-geometry selector;
- M1117 fixed base/alternate blend.

## Surface

Remote host: `spark-1`

Run dir:

`/home/huoju/leask/runs/ii42-m1118-shared5-profile-export-v1`

Dataset root:

`/home/huoju/leask/data/psql_bm25s_sae_beir15_shared`

Datasets:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`

The remote root currently exposes roughly 2k docs and 100 qrels queries per
dataset, so this is a smoke-size shared5 surface, not full shared15.

## Runner

```bash
RUN=/home/huoju/leask/runs/ii42-m1118-shared5-profile-export-v1 \
DATASETS_ROOT=/home/huoju/leask/data/psql_bm25s_sae_beir15_shared \
DATASETS="nfcorpus scifact fiqa arguana scidocs" \
SEED=1050 \
OUTPUT_NAME=m1118_shared5_profile_export_s1050.json \
SEMANTIC_HARD_NEGATIVES_PER_QUERY=0 \
SEMANTIC_MAX_PAIRS_PER_DATASET=0 \
RANKING_EXPORT_DIR=/home/huoju/leask/runs/ii42-m1118-shared5-profile-export-v1/rankings \
RANKING_EXPORT_K=1000 \
bash scripts/run_m1100_semantic_neighbor_posting_spark.sh
```

## Acceptance

Proceed only if the export completes and the replay shows one of:

- M1117-style fixed blend improves Recall/MRR/NDCG/MAP without local row
  floor damage; or
- M1115-style rank-geometry selector improves all macro metrics and survives a
  leave-dataset-out check on the shared5 surface.

If neither holds, stop this scoring-profile route and return to atom/posting
score construction rather than further selector tuning.
