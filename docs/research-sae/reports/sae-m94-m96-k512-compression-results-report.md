# SAE M94-M96 k512 Compression Results Report

Date: 2026-05-22

Status: promoted for compressed Stage A. M96 finds the first `8192/k512`
profile that passes the unchanged Stage-A gate.

## Decision

Promote `interp_alpha_0.35` with baseline scoring:

```text
latent dims: 8192
active dims: 512
base checkpoint: m94-k512-from768-teacher015
target checkpoint: m94-k512-from768-teacher025
interpolation alpha: 0.35
score calibration: baseline
```

This reduces active support from M93 `k768` to `k512`. M93 remains the safe
fallback and the teacher used to produce the two M94 endpoints. M96 is the new
best compressed Stage-A checkpoint because it passes the same gate without
extra score calibration.

## Gate Result

Stage-A gate:

| Gate | Requirement |
| --- | ---: |
| Overall Recall@10 tax | `>= -0.006` |
| Overall MRR tax | `>= -0.020` |
| Overall NDCG@10 tax | `>= -0.015` |
| Every source-family NDCG@10 tax | `>= -0.020` |
| Every source-family Recall@10 tax | `>= -0.010` |

Promoted point:

| Run | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax | Passed |
| --- | --- | ---: | ---: | ---: | --- |
| `interp_alpha_0.35` | `baseline` | `-0.00495` | `-0.01855` | `-0.01058` | yes |

The `alpha=0.35` endpoint also passes with several lightweight calibration
settings, but the promoted checkpoint uses baseline scoring because it is the
simplest and has the best gate margin among the passing points.

## Family-Level Tax

Promoted `interp_alpha_0.35/baseline`:

| Family | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: |
| BEIR15 current eval | `-0.00025` | `-0.00841` | `-0.00594` |
| Broad generated query | `-0.00590` | `-0.02095` | `-0.01171` |
| Large supervised split | `+0.00000` | `-0.00370` | `-0.00343` |

The key repair versus M94 is the large-supervised Recall@10 family gate:
direct k512 compression missed by one top-10 row, while the interpolated
checkpoint recovers it without losing the aggregate ranking gate.

## M94 Direct k512 Compression

M94 trained four `k512` checkpoints from the M93 `k768` teacher. No direct
training point passed.

Best direct point:

| Run | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax | Failures |
| --- | --- | ---: | ---: | ---: | --- |
| `m94-k512-from768-teacher015` | `overp002` | `-0.00597` | `-0.02031` | `-0.01139` | overall MRR, large-supervised Recall |

Direct `k512` was already close: aggregate Recall/NDCG passed, but MRR missed
by `0.00031`, and the large supervised split still had Recall@10 tax
`-0.01587`.

## M94 Calibration Refine

A denser calibration grid on the best direct checkpoint did not fix the
family-level coverage miss.

Best refine points:

| Calibration | Recall@10 tax | MRR tax | NDCG@10 tax | Failures |
| --- | ---: | ---: | ---: | --- |
| `overp008` | `-0.00602` | `-0.01955` | `-0.01132` | overall Recall, large-supervised Recall |
| `overp010` | `-0.00603` | `-0.01940` | `-0.01134` | overall Recall, large-supervised Recall |

This shows that score calibration alone cannot recover a missing document from
the `k512` support surface.

## M95 Family Repair

M95 tried a short family/dataset-balanced fine-tune from the best M94
checkpoint. It is negative evidence.

Best M95 point:

| Run | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax | Failures |
| --- | --- | ---: | ---: | ---: | --- |
| `m95-k512-ft-family-pair075` | `rawm001_overp010` | `-0.00921` | `-0.02086` | `-0.01415` | overall Recall, overall MRR, broad Recall, large Recall |

The family-balanced repair worsened aggregate recall and did not recover the
large-supervised split. This path should not be continued without a different
objective.

## M96 Checkpoint Interpolation

M96 interpolated between the two compatible M94 endpoints:

```text
base:   m94-k512-from768-teacher015/m80_sparse_sae.best.pt
target: m94-k512-from768-teacher025/m80_sparse_sae.best.pt
```

Passing points:

| Alpha | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax |
| ---: | --- | ---: | ---: | ---: |
| `0.35` | `baseline` | `-0.00495` | `-0.01855` | `-0.01058` |
| `0.35` | `overp002` | `-0.00498` | `-0.01831` | `-0.01051` |
| `0.35` | `overp004` | `-0.00507` | `-0.01789` | `-0.01044` |
| `0.35` | `overp006` | `-0.00555` | `-0.01793` | `-0.01081` |
| `0.35` | `overp008` | `-0.00543` | `-0.01808` | `-0.01068` |
| `0.35` | `overp010` | `-0.00554` | `-0.01808` | `-0.01074` |
| `0.35` | `rawm001_overp008` | `-0.00523` | `-0.01788` | `-0.01031` |
| `0.35` | `rawm001_overp010` | `-0.00527` | `-0.01789` | `-0.01047` |
| `0.40` | `baseline` | `-0.00555` | `-0.01925` | `-0.01160` |

The useful region is narrow. `alpha=0.45` and later usually fail overall
Recall, while lower alphas keep the large-supervised Recall miss. The promoted
point is therefore `alpha=0.35/baseline`.

## Evidence Handles

Spark host:

```text
huoju@100.123.2.95
```

Artifacts:

```text
/home/huoju/leask/runs/m94-k768-to-k512-v0
/home/huoju/leask/runs/m94-k512-calibration-refine-v0
/home/huoju/leask/runs/m95-k512-family-repair-v0
/home/huoju/leask/runs/m96-k512-checkpoint-interp-v0
/home/huoju/leask/runs/m96-k512-checkpoint-interp-v0/checkpoints/interp_alpha_0.35.pt
```

Aim experiments:

```text
sae-m94
sae-m95
sae-m96
```

Launchers:

```text
scripts/run_sae_m94_k768_to_k512.sh
scripts/run_sae_m96_k512_checkpoint_interp.sh
```

## Interpretation

M96 changes the compression status:

- `k768` is no longer the best compressed Stage-A endpoint; it is now the safe
  fallback and teacher.
- `k512` is promoted through checkpoint interpolation, not direct training.
- score calibration is not required for the promoted point.
- family-balanced fine-tuning is parked as negative evidence.

The next phase should use M96 `8192/k512` as the Stage-A sparse preservation
checkpoint. Product readiness still requires later validation of physical
payload cost and Stage-B BM25+SAE ranking behavior; M96 only closes the
Stage-A preservation/compression gate.
