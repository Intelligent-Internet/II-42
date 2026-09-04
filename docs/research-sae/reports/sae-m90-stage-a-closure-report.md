# SAE M90 Stage-A Closure Report

Date: 2026-05-22

Status: promoted for Stage A. Stage A is now closed at `8192/k1024` with a
single interpolated sparse-preservation encoder.

## Decision

M90 closes the Stage-A sparse-preservation gate by interpolating two compatible
M87-family checkpoints:

- recall-favorable checkpoint: M89 `selection_metric=recall_at_10`, selected
  epoch 2;
- rank-favorable checkpoint: M88 `selection_metric=mrr`, selected epoch 9;
- interpolation: one final checkpoint, not a runtime ensemble;
- canonical promoted point: `alpha=0.35`, no extra score calibration
  (`baseline: raw_dot=0, overlap=0, doc_norm=0, doc_mass=0`).

This means the current representation can preserve the dense student well
enough if active support is widened to `k1024`. The remaining work is no longer
"can Stage A close at all"; it is support compression and payload/query-time
cost reduction.

## Stage-A Gate

Stage A closure gate:

| Gate | Requirement |
| --- | ---: |
| Overall Recall@10 tax | `>= -0.006` |
| Overall MRR tax | `>= -0.020` |
| Overall NDCG@10 tax | `>= -0.015` |
| Every source-family NDCG@10 tax | `>= -0.020` |
| Every source-family Recall@10 tax | `>= -0.010` |

Promoted M90 point:

| Run | Recall@10 tax | MRR tax | NDCG@10 tax | Passed |
| --- | ---: | ---: | ---: | --- |
| `alpha=0.35 baseline` | `-0.00449` | `-0.01864` | `-0.00844` | yes |

Family-level tax:

| Family | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: |
| BEIR15 current eval | `+0.00522` | `-0.02308` | `-0.01391` |
| Broad generated query | `-0.00596` | `-0.01878` | `-0.00780` |
| Large supervised split | `+0.00000` | `-0.01111` | `-0.00880` |

MRR is reported by family for diagnosis, but the Stage-A gate only requires
family Recall@10 and NDCG@10. The promoted point passes every defined gate.

## Evidence Matrix

M83-M86 did not close Stage A:

| Milestone | Finding |
| --- | --- |
| M83 | Balanced sampling and closure-aware selection repeated the M82 tradeoff. |
| M84 | Label-positive CE at low weights worsened early MRR. |
| M85 | `k512` improved NDCG/Recall but still missed MRR. |
| M86 | Positive-margin scalar supervision worsened MRR even at low weight. |

M87-M89 isolated the true tradeoff:

| Run | Selection | Recall@10 tax | MRR tax | NDCG@10 tax | Failure |
| --- | --- | ---: | ---: | ---: | --- |
| M87 | closure gate | `-0.00700` | `-0.02081` | `-0.01322` | recall, MRR |
| M88 | MRR | `-0.00868` | `-0.01834` | `-0.01427` | recall |
| M89 | Recall@10 | `-0.00358` | `-0.02293` | `-0.01282` | MRR |

M90 then combined the two endpoints by checkpoint interpolation. This is a
single model checkpoint, so it does not introduce runtime multi-encoder
complexity.

Selected passing M90 points:

| Alpha | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax |
| ---: | --- | ---: | ---: | ---: |
| `0.15` | baseline | `-0.00415` | `-0.01949` | `-0.01000` |
| `0.25` | baseline | `-0.00533` | `-0.01818` | `-0.00986` |
| `0.35` | baseline | `-0.00449` | `-0.01864` | `-0.00844` |
| `0.55` | baseline | `-0.00358` | `-0.01819` | `-0.00891` |

The canonical point is `alpha=0.35 baseline` because it passes without any
extra score calibration and has the best clean NDCG margin among the baseline
passing points.

## Reproduction Handles

Spark host:

```text
huoju@100.123.2.95
```

Input checkpoints:

```text
/home/huoju/leask/runs/m89-k1024-kl4-pair05-recon025-select-recall-e10/m80_sparse_sae.best.pt
/home/huoju/leask/runs/m88-k1024-kl4-pair05-recon025-select-mrr-e10/m80_sparse_sae.best.pt
```

M90 artifacts:

```text
/home/huoju/leask/runs/m90-k1024-checkpoint-interp-v0
```

Local reproduction helper:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_checkpoint_interpolate.py \
    --base-checkpoint <recall_selected_checkpoint> \
    --target-checkpoint <mrr_selected_checkpoint> \
    --output-dir <interpolated_checkpoint_dir> \
    --alpha 0.15 --alpha 0.25 --alpha 0.35 --alpha 0.55
```

Then evaluate generated checkpoints with:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_cache_calibration_grid.py \
    --corpus-dir <large_stage_a_corpus> \
    --sparse-checkpoint <interpolated_checkpoint> \
    --dense-cache-dir <documents_shard_or_queries_shard> \
    --output-dir <calibration_output> \
    --candidate-batch-rows 8 \
    --device cuda \
    --config baseline:0,0,0,0
```

## Interpretation

The result is important but not yet a product-ready profile:

- Stage A is representation-feasible.
- `k1024` is too expensive to treat as the final query-time profile.
- CE, margin loss, and simple sampling were not the answer.
- The useful signal came from preserving two nearby optima and interpolating
  them into a stable single checkpoint.

The next phase should focus on support compression:

- distill the M90 `k1024` checkpoint into lower active budgets;
- learn atom allocation so high-value semantic mass is concentrated earlier;
- use M90 as the teacher for `k384/k512` compression instead of returning to
  scalar qrel losses;
- keep Stage B parked until a compressed Stage-A profile has an acceptable
  physical cost frontier.
