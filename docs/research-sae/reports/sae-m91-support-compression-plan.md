# SAE M91 Support Compression Plan

Date: 2026-05-22

Status: active.

## Goal

M90 closed Stage A at `8192/k1024`, proving that the representation can
preserve the dense student under a wide active support. M91 asks the next
question:

```text
Can the M90 k1024 behavior be compressed into k512 or k384 without reopening
the Stage-A sparse tax?
```

This is still Stage-A work. Stage B remains parked until a lower-cost Stage-A
profile exists.

## Training Change

M91 adds optional sparse-teacher distillation to
`research_sae_m80_sparse_preservation.py`:

- `--sparse-teacher-checkpoint`: promoted M90 checkpoint;
- `--sparse-teacher-weight`: KL weight for matching sparse-teacher candidate
  scores;
- `--sparse-teacher-baseline-scoring`: default enabled, so the teacher uses
  the M90 promoted baseline score surface rather than inherited raw-dot
  calibration.

The lower-k student still trains against the dense teacher. The sparse teacher
is an additional support-compression guide, not a replacement.

## First Matrix

Use the same large Stage-A surface and dense cache as M87-M90.

| Run | Active dims | Sparse teacher weight | Init | Purpose |
| --- | ---: | ---: | --- | --- |
| `m91-k512-teacher025` | 512 | 0.25 | M90 alpha 0.35 | Main compression probe |
| `m91-k512-teacher050` | 512 | 0.50 | M90 alpha 0.35 | Stronger teacher probe |
| `m91-k384-teacher025` | 384 | 0.25 | M90 alpha 0.35 | Aggressive compression |

All runs log to Aim under `sae-m91`.

## Gate

The first pass uses the same Stage-A closure gate:

- overall Recall@10 tax `>= -0.006`;
- overall MRR tax `>= -0.020`;
- overall NDCG@10 tax `>= -0.015`;
- every source-family NDCG@10 tax `>= -0.020`;
- no source-family Recall@10 tax below `-0.010`.

If no lower-k run passes, M91 should still report the Pareto frontier and the
dominant failure mode. The next step would then be atom allocation or a
different representation, not more CE/margin tuning.
