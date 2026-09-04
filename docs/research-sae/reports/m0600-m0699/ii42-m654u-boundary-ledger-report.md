# M654U Boundary Ledger

Status: `boundary_ledger_complete`

M654T audits query-level boundary movement for M654U runs.
It does not train a model and does not use BM25, rerankers, learned
gates, or qrels-driven objectives.

This ledger summarizes the selected checkpoint query rows.  For runs
where no trained checkpoint passed dev, the selected checkpoint is
epoch0/no-op; their training-trace failures should be read from the
M654U synthesis report.

## Runs

### `m654u_canary_boundary_seed6544`

Decision: `dense_equivalence_gate_passed`

Failed checks: `none`

| Split | Rows | Main classes | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `dev` | 40 | no_overlap_movement=38, tail_gain_head_flat=2 | 0.000000000 | 0.000195313 | 0.000000000 | 0.000000000 |
| `test` | 40 | no_overlap_movement=38, tail_gain_head_flat=2 | 0.000000000 | 0.000195313 | 0.000000000 | 0.000008735 |


## Interpretation

M654U removes the M654S held-out O@100 regression on the four-dataset canary. The selected trained checkpoint keeps O@100, CUB, Recall@100, active support, and support cosine safe while adding small O@256 and MAP gains.

## Next Step

Run a broader shared15/native validation before promotion. If the same no-loss tail gain survives, treat M654U as the next first-stage candidate; if it collapses to zero or spends O@100/support, keep it as a canary-only diagnostic.
