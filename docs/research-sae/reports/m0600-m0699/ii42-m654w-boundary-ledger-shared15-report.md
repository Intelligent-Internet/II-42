# M654W Boundary Ledger Shared15

Status: `boundary_ledger_complete`

M654T audits query-level boundary movement for M654W runs.
It does not train a model and does not use BM25, rerankers, learned
gates, or qrels-driven objectives.

This ledger summarizes the selected checkpoint query rows.  For runs
where no trained checkpoint passed dev, the selected checkpoint is
epoch0/no-op; their training-trace failures should be read from the
M654W synthesis report.

## Runs

### `m654w_shared15_boundary_loss_w01_seed6545`

Decision: `dense_equivalence_gate_failed`

Failed checks: `dense_overlap_100_safe, dense_overlap_256_safe`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 134 | head_gain_tail_flat=2, no_overlap_movement=121, o100_flat_o256_loss=3, tail_gain_head_flat=8 | 0.000149254 | 0.000145756 | 0.000000000 | -0.000000607 |
| `test` | 134 | head_gain_tail_flat=1, no_overlap_movement=126, o100_flat_o256_loss=2, o100_loss_o256_flat=3, tail_gain_head_flat=2 | -0.000149254 | 0.000000000 | 0.000000000 | 0.000006361 |


## Interpretation

M654W boundary preservation loss does not pass strict first-stage validation. On the same shared15 split as M654U, it keeps Recall/CUB/support but worsens dense O@100 and O@256, so the protected-vs-negative hinge is not the right fix.

## Next Step

Do not scale this loss. Inspect boundary swaps at doc level or change the compiler architecture/teacher transfer objective; selection-only and this margin-only boundary loss are both insufficient.
