# M659 Top100 Boundary Replay Audit Report

## Summary

M659 tested whether M658's near-miss could be fixed by replaying the exact
P1 top100 boundary. The added loss protects the P1 rank100 document when that
document is already a dense top100 hit, and asks the query compiler to preserve
its baseline margin over the rank101 neighborhood.

Result: this does not produce a candidate. It reduces the macro O@100 loss
slightly, but it expands doc-level churn and damages support geometry. The
failure is therefore not evidence that the current run simply needed more
epochs. It is evidence that this replay loss has the wrong shape.

## Configuration

Baseline comparison:

- M658: `m658_shared15_swap1_nearmiss_seed6545`
- M659: `m659_shared15_allq_swap1_top100replay1_seed6545`

Shared settings:

- `teacher_query_source=all_queries`
- `train/dev/test examples = 1074/134/134`
- `teacher_policy=boundary_constrained`
- `swap_preservation_weight=1.0`
- `swap_gate_max_dense_hit_loss_queries=0`
- `select_best_rejected_checkpoint=true`
- no BM25, no reranker, no learned gate, no qrels loss

M659 added:

- `top100_boundary_replay_weight=1.0`
- `top100_boundary_replay_window_k=8`
- `top100_boundary_replay_max_pairs=8`
- `top100_boundary_replay_extra_margin=0.0`

The new constraint is not empty:

| Split | Boundary pair query rate | Pair count mean |
| --- | ---: | ---: |
| train | `0.497207` | `3.977654` |
| dev | `0.500000` | `4.000000` |
| test | `0.537313` | `4.298507` |

## Result Versus M658

| Run | Selected epoch | Dev gainQ | Dev lossQ | Test gainQ | Test lossQ | Test net docs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M658 | `1` | `4` | `1` | `2` | `7` | `-5` |
| M659 | `2` | `22` | `30` | `24` | `29` | `-6` |

Test deltas versus P1:

| Metric | M658 | M659 |
| --- | ---: | ---: |
| dense overlap@100 | `-0.000404521` | `-0.000218855` |
| dense overlap@256 | `-0.000023345` | `-0.000370173` |
| Recall@100 | `+0.000595238` | `+0.000568182` |
| MAP@100 | `+0.000142345` | `-0.000409687` |
| NDCG@10 | `0.000000000` | `-0.000355876` |
| MRR@20 | `0.000000000` | `0.000000000` |
| support cosine | `-0.000000381` | `-0.000094779` |

M659 is strictly worse as a first-stage candidate. It still fails dense
overlap, also fails support floors, and converts the tiny M658 near-miss into
broader top100 churn.

## Doc-Swap Audit

Artifacts:

- `runs/m659_top100_boundary_replay_audit_v1/m659_top100replay_dev_doc_swaps.json`
- `runs/m659_top100_boundary_replay_audit_v1/m659_top100replay_test_doc_swaps.json`
- `docs/research-sae/reports/m0600-m0699/ii42-m659-top100-boundary-replay-doc-swap-dev-report.md`
- `docs/research-sae/reports/m0600-m0699/ii42-m659-top100-boundary-replay-doc-swap-test-report.md`

Dev:

- dense-hit gain rows: `12`
- dense-hit loss rows: `18`
- dense-hit swap-flat rows: `10`
- lost dense docs: `30`
- gained dense docs: `24`
- new nondense docs: `38`

Test:

- dense-hit gain rows: `15`
- dense-hit loss rows: `21`
- dense-hit swap-flat rows: `8`
- lost dense docs: `30`
- gained dense docs: `24`
- new nondense docs: `41`

This confirms that M659 does not localize the failure. The added top100 replay
loss creates broader dense-hit replacement and non-dense churn.

## Training-Depth Interpretation

The fast iteration ratio is still justified for this class of probes.
M659 had enough signal to show the mechanism quickly:

- the constraint covered about half of the queries;
- the selected checkpoint was trained, not epoch0;
- loss kept decreasing through epoch12;
- gate behavior worsened instead of converging toward no-loss;
- support cosine regressed by two orders of magnitude more than M658.

Longer training of this exact objective is unlikely to repair the gate. The
problem is not insufficient runtime; it is that the replay objective changes
too many local boundaries and spends support geometry.

## Conclusion

M659 is a negative but useful result.

Keep:

- M658 near-miss as the most informative current direction.
- The diagnosis that failures are boundary/micro-margin related.
- The strict no-loss gate.

Reject:

- Simple rank100 replay as implemented here.
- Treating macro O@100 improvement as sufficient when doc-level lossQ grows.
- More epochs on this objective without changing the constraint shape.

Next direction:

- use a smaller trust-region on query delta or score delta, not a stronger
  boundary hinge;
- constrain movement only for queries whose baseline rank100/rank101 margin is
  below a tiny threshold;
- consider per-query movement budget selected by baseline boundary margin;
- keep selection strict and keep BM25/reranker/qrels out of this phase.
