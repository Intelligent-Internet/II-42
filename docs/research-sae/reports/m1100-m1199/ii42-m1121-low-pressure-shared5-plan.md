# ii42 M1121 Low-Pressure Shared5 Plan

## Objective

Test whether the M1118/M1120 train-heldout mismatch comes from excessive atom
rank pressure.

M1118 showed that:

- train split monotonically preferred higher additive alpha;
- heldout top-rank quality peaked near alpha `0.25`;
- alpha `0.5` had better Recall but worse MRR/NDCG/MAP;
- alpha `0.75` was train-selected but failed heldout.

This is an overfitting/pressure signal. M1121 reduces the utility-training
pressure while keeping the same shared5 surface and replay harness.

## Run

Remote host: `spark-1`

Run dir:

`/home/huoju/leask/runs/ii42-m1121-shared5-lowpressure-export-v1`

Datasets:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`

Key parameters:

- `RANK_EPOCHS=4`
- `LEARNING_RATE=0.0007`
- `SEMANTIC_HARD_NEGATIVES_PER_QUERY=0`
- `SEMANTIC_MAX_PAIRS_PER_DATASET=0`

## Acceptance

After export, run the same replay suite as M1118:

- fixed profile replay;
- fixed alpha summary;
- fine alpha grid.

Promote the route only if:

1. heldout improves over lexical on all four metrics;
2. selected alpha does not show the same high-alpha train overfit;
3. heldout improves over M1118 `additive_atom_0.5` or provides a better
   Recall/top-rank tradeoff than M1118 alpha `0.25`;
4. per-dataset damage is limited.

If lower pressure only reduces all atom gains or still overfits alpha, stop
this pressure-swap line and redesign the atom utility objective.
