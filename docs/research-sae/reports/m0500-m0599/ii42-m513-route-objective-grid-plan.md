# M513 Route Objective Grid Plan

M512 proved that route-aware fine-tuning can materially reduce posting fanout,
but candidate recall became the bottleneck.  M513 keeps the same no-BM25/no-qrels
training boundary and sweeps route objective weights.

## Goal

Recover candidate recall while preserving the M512 fanout reduction.

Promotion target on the FiQA route-subset gate:

- p96 or p128 NDCG@10 near 0.467;
- Recall@100 near or above 0.87;
- candidate recall at least 0.95;
- Touch clearly below 0.80.

## Script

`scripts/research_sae_m513_route_objective_grid.py`

The script trains the base PPLX-LoRA model once, saves the trainable adapter/head
state, then restores that state for each route-objective preset.

This avoids retraining the base LoRA model for every objective setting.

## Presets

| Preset | Pos K | Neg K | Route Epochs | Neg Weight | Anchor |
| --- | ---: | ---: | ---: | ---: | ---: |
| `baseline` | 8 | 24 | 1 | 0.20 | 0.25 |
| `preserve16` | 16 | 24 | 1 | 0.10 | 0.50 |
| `preserve16_neg48_e2` | 16 | 48 | 2 | 0.10 | 0.50 |

## Run Command

```bash
cd /home/huoju/leask/dev/ii42-m327-work
PYTHONPATH=scripts /home/huoju/SideProj/next-ELF-torch/.venv/bin/python \
    scripts/research_sae_m513_route_objective_grid.py \
    --tasks FiQA2018 \
    --output-root /home/huoju/leask/runs/ii42-m513-route-objective-grid-fiqa \
    --json-name m513_fiqa_route_objective_grid.json \
    --report-name m513_fiqa_route_objective_grid.md \
    --grid-presets baseline,preserve16,preserve16_neg48_e2 \
    --route-doc-limit 4096 \
    --route-teacher-docs-per-query 64 \
    --route-prefixes 64,96,128 \
    --max-eval-queries 64 \
    --max-doc-rows-per-task 384 \
    --max-query-rows-per-task 96 \
    --query-repeat 4 \
    --epochs 4 \
    --target-epochs 4 \
    --train-last-layers 2 \
    --train-batch-size 4 \
    --project-batch-size 16 \
    --support-weight 2.0 \
    --support-cosine-weight 1.0 \
    --active-bce-weight 0.6 \
    --active-magnitude-weight 1.0 \
    --inactive-weight 0.03 \
    --learning-rate 5.0e-6 \
    --head-learning-rate 3.0e-4 \
    --route-temperature 0.08 \
    --route-listwise-weight 1.0 \
    --local-files-only
```

## Current Blocker

At creation time, `spark-2` is occupied by Scale-RAE GPU work and should not be
used.  `spark-1` SSH is returning banner-exchange timeouts.  The grid runner is
ready, but the run should wait until `spark-1` is reachable or another confirmed
idle PPLX-capable GPU environment is available.
