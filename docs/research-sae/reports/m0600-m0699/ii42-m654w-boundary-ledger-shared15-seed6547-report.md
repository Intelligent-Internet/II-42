# M654W Boundary Ledger Shared15 Seed6547

Status: `boundary_ledger_complete`

M654T audits query-level boundary movement for M654W runs.
It does not train a model and does not use BM25, rerankers, learned
gates, or qrels-driven objectives.

This ledger summarizes the selected checkpoint query rows.  For runs
where no trained checkpoint passed dev, the selected checkpoint is
epoch0/no-op; their training-trace failures should be read from the
M654W synthesis report.

## Runs

### `m654w_shared15_boundary_loss_w01_seed6547`

Decision: `dense_equivalence_gate_failed`

Failed checks: `dense_overlap_100_safe, dense_overlap_256_safe`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 134 | head_gain_tail_flat=1, no_overlap_movement=129, overlap_both_loss=1, tail_gain_head_flat=3 | -0.000000000 | 0.000058302 | 0.000000000 | -0.000018226 |
| `test` | 134 | no_overlap_movement=127, o100_flat_o256_loss=3, o100_loss_o256_flat=1, tail_gain_head_flat=3 | -0.000074627 | 0.000000000 | 0.000000000 | 0.000007174 |


## Interpretation

M654W seed6547 independently confirms the boundary preservation loss is not robust: selected dev checkpoint fails held-out O@50/O@100/O@256 despite preserving Recall/CUB/support.

## Next Step

Keep this as negative evidence. The next route should use doc-level swap diagnosis or a different transfer architecture, not a larger weight on this loss.
