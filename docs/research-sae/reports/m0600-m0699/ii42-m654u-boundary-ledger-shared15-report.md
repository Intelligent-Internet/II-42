# M654U Boundary Ledger Shared15

Status: `boundary_ledger_complete`

M654T audits query-level boundary movement for M654U runs.
It does not train a model and does not use BM25, rerankers, learned
gates, or qrels-driven objectives.

This ledger summarizes the selected checkpoint query rows.  For runs
where no trained checkpoint passed dev, the selected checkpoint is
epoch0/no-op; their training-trace failures should be read from the
M654U synthesis report.

## Runs

### `m654u_shared15_boundary_seed6545`

Decision: `dense_equivalence_gate_failed`

Failed checks: `dense_overlap_100_safe`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 134 | head_gain_tail_flat=2, no_overlap_movement=125, o100_flat_o256_loss=1, tail_gain_head_flat=6 | 0.000149254 | 0.000145756 | 0.000000000 | -0.000008920 |
| `test` | 134 | head_gain_tail_flat=1, no_overlap_movement=128, o100_flat_o256_loss=1, o100_loss_o256_flat=2, tail_gain_head_flat=2 | -0.000074627 | 0.000029151 | 0.000000000 | 0.000005676 |


## Interpretation

M654U broader shared15 does not pass the strict first-stage gate. The selected trained checkpoint preserves Recall, CUB, support, and gives tiny O@256/MAP gains, but held-out O@100 regresses by a very small amount, so it cannot be promoted.

## Next Step

Inspect the O@100-loss query classes and tighten selection or checkpoint choice before any larger run. Do not promote based on MAP/O@256 while O@100 is negative.
