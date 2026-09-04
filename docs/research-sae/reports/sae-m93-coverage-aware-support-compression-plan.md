# SAE M93 Coverage-Aware Support Compression Plan

Date: 2026-05-22

Status: closed. `m93-k768-teacher025` passed the Stage-A gate and is promoted
as the compressed Stage-A checkpoint.

## Goal

M91-M92 showed that direct `k512` support compression is close on aggregate
quality but still loses BEIR-current coverage. M93 moves one step less
aggressively:

```text
k1024 promoted M90 -> k640/k768 coverage-preserving compression
```

The goal is not to reach final query-time cost in one jump. The goal is to find
the first lower-support checkpoint that passes the same Stage-A gate, then use
that checkpoint as the next teacher for later compression.

## Why This Direction

The failed `k512` result is informative:

- M91 `k512/teacher025` nearly preserved the dense student but missed MRR/NDCG.
- M91 `k512/teacher050` improved ranking but lost too much recall.
- M92 `k512` interpolation recovered aggregate Recall@10/NDCG@10, but
  BEIR-current Recall@10 stayed below the source-family gate.

This points to a coverage bottleneck, not a generic ranking bottleneck. Jumping
directly from `k1024` to `k512` removes too many useful atoms for some BEIR
queries. M93 tests whether an intermediate budget keeps enough semantic support
while still reducing the active payload from M90.

## Matrix

All runs use:

- corpus: `large-stage-a-v0-stable`;
- dense checkpoint: M80-A1 saved-best dense student;
- initialization: M90 `alpha=0.35`;
- sparse teacher: M90 `alpha=0.35`;
- selection: `stage_a_closure_gate`;
- evaluation: hard TopK, same Stage-A gate as M90-M92;
- Aim experiment: `sae-m93`.

| Run | Active dims | Sparse teacher weight | Epochs | Purpose |
| --- | ---: | ---: | ---: | --- |
| `m93-k640-teacher025` | `640` | `0.25` | `8` | Main intermediate compression point |
| `m93-k640-teacher015` | `640` | `0.15` | `8` | More recall-preserving teacher pressure |
| `m93-k768-teacher025` | `768` | `0.25` | `8` | Wider support, expected coverage recovery |

If none pass, do not keep sweeping calibration. The next model change should be
explicit coverage-aware atom allocation, not another scalar objective tweak.

## Gate

M93 uses the same Stage-A gate:

| Gate | Requirement |
| --- | ---: |
| Overall Recall@10 tax | `>= -0.006` |
| Overall MRR tax | `>= -0.020` |
| Overall NDCG@10 tax | `>= -0.015` |
| Every source-family NDCG@10 tax | `>= -0.020` |
| Every source-family Recall@10 tax | `>= -0.010` |

Primary decision rule:

- promote the smallest active budget that passes every gate;
- if `k768` passes but `k640` fails, use `k768` as the next compression teacher;
- if only aggregate gates pass but BEIR-current recall still fails, mark M93 as
  partial and move to coverage-aware atom allocation.

## Reproduction Handles

Spark host:

```text
huoju@100.123.2.95
```

Output root:

```text
/home/huoju/leask/runs/m93-coverage-support-v0
```

Final result:

```text
/home/huoju/leask/runs/m93-k768-teacher025
```

