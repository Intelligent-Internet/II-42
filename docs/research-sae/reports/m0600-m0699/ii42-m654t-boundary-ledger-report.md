# M654T Boundary Ledger

Status: `boundary_ledger_complete`

M654T audits query-level boundary movement for M654S runs.
It does not train a model and does not use BM25, rerankers, learned
gates, or qrels-driven objectives.

This ledger summarizes the selected checkpoint query rows.  For runs
where no trained checkpoint passed dev, the selected checkpoint is
epoch0/no-op; their training-trace failures should be read from the
M654S synthesis report.

## Runs

### `m654s_canary_ridge01_s001_seed6543`

Decision: `dense_equivalence_gate_failed`

Failed checks: `trained_checkpoint_selected`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 40 | no_overlap_movement=40 | 0.000000000 | 0.000000000 | 0.000000000 | 0.000000000 |
| `test` | 40 | no_overlap_movement=40 | 0.000000000 | 0.000000000 | 0.000000000 | 0.000000000 |


### `m654s_canary_ridge01_s0001_seed6543`

Decision: `dense_equivalence_gate_failed`

Failed checks: `dense_overlap_100_safe`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 40 | head_gain_tail_flat=1, no_overlap_movement=36, o100_flat_o256_loss=1, tail_gain_head_flat=2 | 0.000250000 | 0.000097656 | 0.000000000 | -0.000010823 |
| `test` | 40 | no_overlap_movement=38, o100_loss_o256_flat=1, tail_gain_head_flat=1 | -0.000250000 | 0.000097656 | 0.000000000 | 0.000006398 |


### `m654s_canary_ridge01_s00005_seed6543`

Decision: `dense_equivalence_gate_failed`

Failed checks: `trained_checkpoint_selected`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 40 | no_overlap_movement=40 | 0.000000000 | 0.000000000 | 0.000000000 | 0.000000000 |
| `test` | 40 | no_overlap_movement=40 | 0.000000000 | 0.000000000 | 0.000000000 | 0.000000000 |


## Interpretation

The safe transfer window is narrow.  Scale 0.001 can pass dev but fails held-out O@100, while neighboring scales either fail O@256 or spend Recall/support.  The next objective needs an explicit boundary no-loss teacher instead of another scale sweep.

## Next Step

Build M654U as a boundary-constrained teacher: accept only per-query teacher deltas that preserve dense top100 before attempting O@256 or score-distribution improvements.
