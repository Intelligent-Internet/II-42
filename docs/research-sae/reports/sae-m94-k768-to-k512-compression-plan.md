# SAE M94 k768-To-k512 Compression Plan

Date: 2026-05-22

Status: closed. Direct M94 training did not pass, but the follow-up M96
checkpoint-interpolation run promoted an `8192/k512` profile.

## Goal

M93 promoted `8192/k768` as the first compressed Stage-A checkpoint. M94 now
tests whether that checkpoint can serve as a closer teacher for `k512`
compression:

```text
M90 k1024 -> M93 k768 -> M94 k512
```

This is intentionally a second-stage compression. M91 tried to jump directly
from M90 to `k512` and missed coverage. M94 uses the closer `k768` support
surface to reduce the gap before attempting `k512`.

## Matrix

All runs use:

- corpus: `large-stage-a-v0-stable`;
- dense checkpoint: M80-A1 saved-best dense student;
- initialization: M93 `k768/teacher025` best checkpoint;
- sparse teacher: M93 `k768/teacher025` best checkpoint;
- active support: `k512`;
- selection: `stage_a_closure_gate`;
- evaluation: hard TopK, same Stage-A gate as M90-M93;
- Aim experiment: `sae-m94`.

| Run | Active dims | Sparse teacher weight | Epochs | Purpose |
| --- | ---: | ---: | ---: | --- |
| `m94-k512-from768-teacher025` | `512` | `0.25` | `8` | Main second-stage compression point |
| `m94-k512-from768-teacher015` | `512` | `0.15` | `8` | More recall-preserving teacher pressure |
| `m94-k512-from768-teacher010` | `512` | `0.10` | `8` | Minimal teacher pressure |
| `m94-k512-from768-teacher035` | `512` | `0.35` | `8` | Stronger ranking-preservation control |

## Gate

M94 uses the unchanged Stage-A gate:

| Gate | Requirement |
| --- | ---: |
| Overall Recall@10 tax | `>= -0.006` |
| Overall MRR tax | `>= -0.020` |
| Overall NDCG@10 tax | `>= -0.015` |
| Every source-family NDCG@10 tax | `>= -0.020` |
| Every source-family Recall@10 tax | `>= -0.010` |

Promotion rule:

- promote the simplest baseline-scored `k512` point that passes every gate;
- if only calibrated `k512` points pass, record the calibration but do not
  treat it as final until repeated once;
- if no `k512` point passes but the best gap is below `0.001`, run one
  focused follow-up instead of returning to broad sweeps.

## Reproduction Handles

Spark host:

```text
huoju@100.123.2.95
```

Expected output root:

```text
/home/huoju/leask/runs/m94-k768-to-k512-v0
```

Teacher checkpoint:

```text
/home/huoju/leask/runs/m93-k768-teacher025/m80_sparse_sae.best.pt
```

## Outcome

M94 direct `k512` compression was close but failed. The best direct point,
`m94-k512-from768-teacher015/overp002`, reached overall
Recall@10/MRR/NDCG@10 tax `-0.00597/-0.02031/-0.01139`, but missed the
overall MRR gate and the large-supervised Recall@10 family gate.

The successful follow-up is recorded in
`sae-m94-m96-k512-compression-results-report.md`. M96 interpolated the
`teacher015` and `teacher025` k512 endpoints and promoted
`interp_alpha_0.35` with baseline scoring:

```text
Recall@10 tax: -0.00495
MRR tax:       -0.01855
NDCG@10 tax:   -0.01058
```

Promoted checkpoint:

```text
/home/huoju/leask/runs/m96-k512-checkpoint-interp-v0/checkpoints/interp_alpha_0.35.pt
```
