# SAE M91-M92 Support Compression Results Report

Date: 2026-05-22

Status: closed. The first support-compression push did not pass the Stage-A
gate, but it narrowed the failure mode enough to define the next step.

## Decision

M90 proved Stage-A representation preservation at `8192/k1024`. M91 and M92
tested whether that behavior can be compressed to lower active support without
reopening sparse tax.

Result:

- `k384` is too aggressive under the current objective.
- `k512` is close but not closed.
- Sparse-teacher distillation helps shape ranking, but stronger teacher weight
  trades away recall.
- Checkpoint interpolation from the best `k512` compression checkpoint back
  toward the promoted `k1024` M90 checkpoint does not pass the gate.
- The next useful step is coverage-aware support compression around
  `k640/k768`, not more scalar calibration or interpolation sweeps.

## Stage-A Gate

The closure gate stays unchanged:

| Gate | Requirement |
| --- | ---: |
| Overall Recall@10 tax | `>= -0.006` |
| Overall MRR tax | `>= -0.020` |
| Overall NDCG@10 tax | `>= -0.015` |
| Every source-family NDCG@10 tax | `>= -0.020` |
| Every source-family Recall@10 tax | `>= -0.010` |

## M91 Sparse-Teacher Compression

All M91 runs initialized from the promoted M90 `alpha=0.35` checkpoint and used
M90 as a sparse teacher.

| Run | Active dims | Sparse teacher weight | Recall@10 tax | MRR tax | NDCG@10 tax | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `m91-k512-teacher025` | `512` | `0.25` | `-0.0066` | `-0.0210` | `-0.0152` | close, failed |
| `m91-k512-teacher050` | `512` | `0.50` | `-0.0090` | `-0.0176` | `-0.0136` | ranking better, recall worse |
| `m91-k384-teacher025` | `384` | `0.25` | `-0.0114` | `-0.0214` | `-0.0159` | too compressed |

The main tradeoff is clear: stronger sparse-teacher pressure improves early
rank metrics but removes too much candidate coverage. The `k512/0.25` point is
the closest clean lower-support checkpoint, but it still misses both MRR and
NDCG by a small margin.

## M92 k512 Interpolation Follow-Up

M92 interpolated the M91 `k512/teacher025` checkpoint toward the promoted M90
checkpoint, while keeping the `k512` model arguments from the base checkpoint.

Best observed calibration point:

| Alpha | Calibration | Recall@10 tax | MRR tax | NDCG@10 tax | Failure |
| ---: | --- | ---: | ---: | ---: | --- |
| `0.25` | `rawp005` | `-0.00565` | `-0.02136` | `-0.01314` | overall MRR, BEIR-current recall |

This is close to the aggregate gate: overall Recall@10 and NDCG@10 pass, and
overall MRR misses by roughly `0.00136`. However, BEIR-current Recall@10 still
misses the source-family gate by a wider margin:

| Family | Recall@10 tax | NDCG@10 tax |
| --- | ---: | ---: |
| BEIR15 current eval | `-0.01910` | `-0.01929` |

M92 therefore does not promote a `k512` profile. It shows that simple
interpolation can rebalance global recall and NDCG, but it does not repair the
family-level coverage loss.

## Evidence Handles

Spark host:

```text
huoju@100.123.2.95
```

M91 artifacts:

```text
/home/huoju/leask/runs/m91-k512-teacher025
/home/huoju/leask/runs/m91-k512-teacher050
/home/huoju/leask/runs/m91-k384-teacher025
```

M92 artifacts:

```text
/home/huoju/leask/runs/m92-k512-compression-interp-v0
```

Aim experiments:

```text
sae-m91
sae-m92
```

## Interpretation

The current evidence does not support `k384` or a direct `k512` promotion.
The next experiment should not keep sweeping the same knobs. The useful signal
is that aggregate quality is nearly closed while family-level recall remains
fragile. That means the next step should preserve coverage first and then try
to compress:

- test intermediate active support (`k640`, `k768`);
- keep M90 sparse-teacher guidance, but avoid over-weighting it;
- select by the same Stage-A gate, with explicit BEIR-current recall reporting;
- if an intermediate budget passes, use it as the new compression teacher for a
  later `k512` attempt.

