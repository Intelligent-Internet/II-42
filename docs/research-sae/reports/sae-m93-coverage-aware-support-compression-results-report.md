# SAE M93 Coverage-Aware Support Compression Results Report

Date: 2026-05-22

Status: promoted as the safe compressed Stage-A fallback. M93 finds the first
lower-support profile that passes the same Stage-A gate as M90, and it was
later used as the teacher for the M96 `k512` promotion.

## Decision

Promote `m93-k768-teacher025` with baseline scoring:

```text
latent dims: 8192
active dims: 768
initialization: M90 alpha=0.35
sparse teacher: M90 alpha=0.35
sparse teacher weight: 0.25
score calibration: baseline
```

This reduces active support from M90 `k1024` to `k768` while preserving the
dense-student retrieval surface under the Stage-A gate. It does not yet close
final product cost; it creates the next compression teacher and a stronger
handoff point for later `k512` attempts.

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
| `m93-k768-teacher025` | `baseline` | `-0.00390` | `-0.01943` | `-0.01023` | yes |

The same checkpoint also passes with `rawp001`, `rawm001`, and `overp002`, but
the promoted point stays on baseline scoring because it is simpler and has the
cleanest overall recall/NDCG balance.

## Family-Level Tax

| Family | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: |
| BEIR15 current eval | `+0.00521` | `-0.00919` | `-0.00728` |
| Broad generated query | `-0.00525` | `-0.02196` | `-0.01118` |
| Large supervised split | `+0.00000` | `-0.00317` | `-0.00294` |

The M92 failure was dominated by BEIR-current recall. M93 fixes that by moving
from direct `k512` compression to an intermediate `k768` support size.

## Full M93 Matrix

| Run | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax | Decision |
| --- | --- | ---: | ---: | ---: | --- |
| `m93-k768-teacher025` | `baseline` | `-0.00390` | `-0.01943` | `-0.01023` | promoted |
| `m93-k768-teacher025` | `rawp001` | `-0.00390` | `-0.01959` | `-0.01024` | passed |
| `m93-k768-teacher025` | `rawm001` | `-0.00412` | `-0.01980` | `-0.01062` | passed |
| `m93-k768-teacher025` | `overp002` | `-0.00439` | `-0.01941` | `-0.01025` | passed |
| `m93-k640-teacher025` | `baseline` | `-0.00688` | `-0.01980` | `-0.01191` | failed recall only |
| `m93-k640-teacher015` | `overp002` | `-0.00635` | `-0.02082` | `-0.01104` | failed recall/MRR |

`k640` is close but does not pass. Reducing sparse-teacher weight from `0.25`
to `0.15` does not improve the frontier; it worsens MRR and still misses
recall. The useful lower-cost point is therefore `k768`, not `k640`.

## Evidence Handles

Spark host:

```text
huoju@100.123.2.95
```

Artifacts:

```text
/home/huoju/leask/runs/m93-coverage-support-v0
/home/huoju/leask/runs/m93-k768-teacher025
/home/huoju/leask/runs/m93-k640-teacher025
/home/huoju/leask/runs/m93-k640-teacher015
```

Aim experiment:

```text
sae-m93
```

Launcher:

```text
scripts/run_sae_m93_coverage_support.sh
```

## Interpretation

M93 confirms the core diagnosis from M91-M92:

- direct `k512` compression loses too much support;
- `k640` is near the boundary but still misses overall recall;
- `k768` is enough to preserve coverage and ranking under the current gate.

This gives a compressed Stage-A checkpoint that is 25% lower active support
than M90. The follow-up M94-M96 run confirmed the intended use: M93 `k768`
served as the teacher for a second compression stage, and M96 promoted the
current `k512` checkpoint through interpolation.
