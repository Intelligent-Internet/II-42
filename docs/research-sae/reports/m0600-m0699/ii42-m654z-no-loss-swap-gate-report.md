# M654Z Strict No-Loss Swap Gate Report

## Summary

M654Z tightened the M654Y selection rule from macro overlap preservation to
query-level no-loss preservation. A candidate checkpoint is accepted only if it
does not lose any dense-hit document from the P1 top100 boundary on the dev
split.

Result: no trained checkpoint passed the strict gate. The final artifact falls
back to epoch0/P1 baseline, so M654Z is a negative result rather than a new
first-stage candidate.

## Why This Probe Was Needed

M654Y found one macro-safe checkpoint:

- O@100: `+0.000026`
- O@256: `+0.000131`
- Recall/CUB: flat
- MAP@100: `-0.000026`

The M654X doc-swap audit showed that this was not truly loss-free. It had 8
dense-hit gain queries and 8 dense-hit loss queries, so the macro average hid
rank-boundary churn. M654Z tests whether the same training shape can be made
strictly safe by selecting only checkpoints with zero lost dense-hit docs.

## Implementation Fix

The first M654Z gate used net dense-hit delta, which could let a query pass if
it gained more dense-hit docs than it lost. That was too weak for first-stage
dense-equivalence.

The strict gate now counts a loss query whenever `lost_dense_count > 0`,
independent of any gained docs in the same query. This makes the no-loss
condition literal: any lost dense-hit top100 document fails the swap gate.

## Run

Command output:

- JSON: `runs/m654z_strict_no_loss_swap_gate_v1/m654z_shared15_swap_w1_strictloss_seed6545/m654z_shared15_swap_w1_strictloss_seed6545.json`
- Generated report: `runs/m654z_strict_no_loss_swap_gate_v1/m654z_shared15_swap_w1_strictloss_seed6545/m654z_shared15_swap_w1_strictloss_seed6545.md`
- Root generated report: `docs/research-sae/reports/m0600-m0699/ii42-m654z-strict-no-loss-swap-gate-report.md`

Training setup:

- frozen doc posting geometry
- query-side compiler only
- no BM25
- no reranker
- no learned gate
- no qrels loss
- `swap_preservation_weight=1.0`
- `swap_gate_max_dense_hit_loss_queries=0`
- seed `6545`

## Evidence

Final selection:

```json
{
  "selected_epoch": 0,
  "selected_global_step": 0,
  "selected_trained_checkpoint": false,
  "failed_checks": ["trained_checkpoint_selected"]
}
```

Final test metrics equal P1 because no trained checkpoint was accepted:

| Source | CUB | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dense_root | 0.954660 | 1.000000 | 1.000000 | 0.746682 | 0.628131 | 0.828092 | 0.844676 |
| p1_native | 0.954421 | 0.945478 | 0.949211 | 0.744424 | 0.629055 | 0.827751 | 0.842907 |
| m654z_strict_no_loss_swap | 0.954421 | 0.945478 | 0.949211 | 0.744424 | 0.629055 | 0.827751 | 0.842907 |

Dev boundary trace:

| Epoch | Failed checks | Gain queries | Loss queries | Gained docs | Lost docs | Net |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 1 | `swap_dense_hit_loss_query_safe` | 4 | 1 | 4 | 1 | 3 |
| 2 | `dense_overlap_100_safe`, `swap_dense_hit_loss_query_safe` | 5 | 5 | 5 | 5 | 0 |
| 3 | `swap_dense_hit_loss_query_safe` | 7 | 6 | 7 | 6 | 1 |
| 4 | `cub_safe`, `dense_overlap_100_safe`, `swap_dense_hit_loss_query_safe` | 8 | 11 | 8 | 11 | -3 |
| 5 | `dense_overlap_100_safe`, `swap_dense_hit_loss_query_safe` | 12 | 12 | 12 | 12 | 0 |
| 6 | `dense_overlap_100_safe`, `swap_dense_hit_loss_query_safe` | 12 | 16 | 12 | 16 | -4 |
| 7 | `dense_overlap_100_safe`, `dense_overlap_256_safe`, `support_cosine_not_regressed`, `swap_dense_hit_loss_query_safe` | 14 | 19 | 14 | 19 | -5 |
| 8 | `dense_overlap_100_safe`, `dense_overlap_256_safe`, `support_cosine_not_regressed`, `swap_dense_hit_loss_query_safe` | 16 | 22 | 16 | 22 | -6 |
| 9 | `dense_overlap_100_safe`, `support_cosine_not_regressed`, `swap_dense_hit_loss_query_safe` | 14 | 23 | 14 | 23 | -9 |
| 10 | `recall_safe`, `dense_overlap_100_safe`, `support_cosine_not_regressed`, `swap_dense_hit_loss_query_safe` | 16 | 24 | 17 | 24 | -7 |
| 11 | `dense_overlap_100_safe`, `support_cosine_not_regressed`, `swap_dense_hit_loss_query_safe` | 15 | 23 | 15 | 23 | -8 |
| 12 | `dense_overlap_100_safe`, `dense_overlap_256_safe`, `support_cosine_not_regressed`, `swap_dense_hit_loss_query_safe` | 18 | 25 | 18 | 25 | -7 |

## Interpretation

This result argues against extending M654Y by simple longer training. Loss
decreases across epochs, but strict dense-hit losses increase from 1 loss query
at epoch 1 to 25 loss queries at epoch 12. The failure mode becomes stronger
as training progresses.

The teacher still contains a small dense-equivalence improvement signal:

- test teacher O@100 delta: `+0.0000746`
- test teacher O@256 delta: `+0.0003790`
- active support delta: `0.0`
- support cosine delta: `-0.0000007`

The blocker is therefore not that the teacher has no direction. The blocker is
that the current query-side compiler and swap-pair preservation loss cannot
transfer that direction without crossing unsafe top100 boundaries.

## Conclusion

M654Z does not produce a new candidate. It preserves the M654Y/M654X conclusion:
macro overlap is too weak, and pairwise swap preservation is not enough to make
boundary movement safe.

The next useful first-stage direction should not be more training of the same
objective. It should change the compiler constraint surface so that boundary
docs are protected directly, for example:

- per-query protected dense-top100 replay loss against the generated scores
- hard masking or trust-region limits near rank100/rank101 boundaries
- listwise dense-top100 preservation loss with explicit no-loss hinge
- separate "do not demote protected dense docs" constraint before allowing any
  teacher-driven expansion

Stop signal: M654Y/M654Z should not be promoted. They are useful diagnostics,
not the first-stage breakthrough.
