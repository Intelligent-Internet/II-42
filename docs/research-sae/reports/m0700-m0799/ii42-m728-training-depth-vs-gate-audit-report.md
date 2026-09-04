# M728 Training Depth vs Dense Gate Audit

Status: `depth_not_primary_bottleneck`

M728 is a read-only audit of existing first-stage compiler traces.  It
checks whether longer training tends to improve strict dense-equivalence
gates or instead lowers loss while harming topK overlap.  It does not
train a model and does not use BM25, reranker, learned gates, or qrels
as an objective.

## Aggregate

```json
{
  "decision_gate_pass_count": 3,
  "final_gate_pass_count": 4,
  "final_o100_negative_count": 14,
  "final_o256_negative_count": 19,
  "first_to_final_o100_down_count": 16,
  "first_to_final_o256_down_count": 19,
  "loss_decreased_count": 29,
  "loss_decreased_o100_down_count": 16,
  "loss_decreased_o256_down_count": 19,
  "run_count": 30,
  "selected_trained_checkpoint_count": 14,
  "trace_row_count": 334
}
```

## Runs

| Run | Steps | Loss down | Final pass | Selected trained | Final dO@100 | Final dO@256 | Final dR@100 | First->Final dO@100 | First->Final dO@256 | Best epoch |
| --- | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m654r_allq_lock_ridge01_s001_floor98_seed6541` | 60 | yes | no | yes | +0.000458 | -0.000537 | +0.004167 | +0.000500 | -0.000439 | 8 |
| `m654r_allq_lock_ridge1_s001_floor98_seed6541` | 60 | yes | no | yes | +0.001375 | -0.001449 | +0.000000 | +0.001000 | -0.001514 | 3 |
| `m654r_allq_p1_support_residual_lock_ridge1_s001_seed6541` | 60 | yes | no | no | +0.001375 | -0.001449 | +0.000000 | +0.001000 | -0.001514 | 12 |
| `m654r_allq_p1_support_residual_ridge10_seed6541` | 60 | yes | no | no | +0.000792 | -0.000798 | +0.000000 | -0.000333 | -0.000195 | 9 |
| `m654r_allq_p1_support_residual_ridge1_s001_seed6541` | 60 | yes | no | no | +0.001875 | -0.001367 | +0.000000 | +0.001292 | -0.001432 | 12 |
| `m654r_allq_p1_support_residual_ridge1_seed6541` | 60 | yes | no | no | +0.000625 | -0.000521 | +0.000000 | -0.000542 | -0.000798 | 4 |
| `m654r_diag_lock_ridge1_s001_floor998_seed6541` | 60 | yes | no | no | +0.001375 | -0.001449 | +0.000000 | +0.001000 | -0.001514 | 12 |
| `m654r_p1_support_residual_ridge10_seed6541` | 48 | yes | no | no | -0.001871 | -0.000682 | +0.000000 | -0.000131 | -0.000078 | 10 |
| `m654r_probe_lock_ridge1_s001_metrics_seed6541` | 15 | yes | no | no | +0.000542 | +0.000000 | +0.004167 | +0.000167 | -0.000065 | 2 |
| `m654r_ridge10_delta64_seed6541` | 48 | yes | no | no | -0.004612 | -0.001257 | -0.003125 | -0.003844 | -0.001926 | 3 |
| `m654r_ridge10_query_compiler_seed6541` | 48 | yes | no | no | -0.001545 | -0.000503 | -0.003125 | -0.000321 | -0.000752 | 2 |
| `m654s_canary_ridge01_s00005_seed6543` | 60 | yes | no | no | +0.000500 | +0.000232 | -0.002976 | +0.000500 | +0.000330 | 10 |
| `m654s_canary_ridge01_s0001_seed6543` | 60 | yes | no | yes | +0.000812 | +0.000244 | -0.002976 | +0.000563 | +0.000281 | 2 |
| `m654s_canary_ridge01_s001_seed6543` | 60 | yes | no | no | +0.001844 | +0.000134 | -0.002976 | +0.001562 | +0.000281 | 7 |
| `m654s_smoke_fiqa_seed6542` | 1 | no | yes | yes | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1 |
| `m654u_canary_boundary_seed6544` | 60 | yes | no | yes | -0.000250 | +0.000070 | +0.000000 | -0.000250 | +0.000070 | 3 |
| `m654u_shared15_boundary_seed6545` | 204 | yes | yes | yes | +0.000157 | +0.000014 | +0.000000 | +0.000074 | +0.000082 | 11 |
| `m654u_smoke_fiqa_seed6544` | 12 | yes | yes | yes | +0.001000 | +0.000391 | +0.000000 | +0.000000 | +0.000391 | 2 |
| `m654v_shared15_overlap50_seed6546` | 204 | yes | no | no | -0.000061 | -0.000084 | +0.000000 | -0.000061 | -0.000126 | 1 |
| `m654w_shared15_boundary_loss_w01_seed6545` | 204 | yes | yes | yes | +0.000157 | +0.000086 | +0.000000 | +0.000074 | +0.000126 | 10 |
| `m654w_shared15_boundary_loss_w01_seed6547` | 204 | yes | no | yes | -0.000083 | -0.000010 | +0.000000 | -0.000074 | -0.000010 | 9 |
| `m654y_shared15_swap_w025_seed6545` | 204 | yes | no | yes | -0.000293 | +0.000157 | +0.000000 | -0.000321 | +0.000089 | 3 |
| `m654y_shared15_swap_w1_seed6545` | 204 | yes | no | yes | -0.000526 | -0.000043 | +0.000000 | -0.000809 | -0.000124 | 1 |
| `m654y_shared15_swap_w1_seed6547` | 204 | yes | no | no | +0.000491 | -0.000310 | +0.000159 | +0.000479 | -0.000171 | 8 |
| `m654z_shared15_swap_w1_noloss_seed6545` | 204 | yes | no | yes | -0.000526 | -0.000043 | +0.000000 | -0.000809 | -0.000124 | 1 |
| `m654z_shared15_swap_w1_strictloss_seed6545` | 204 | yes | no | no | -0.000526 | -0.000043 | +0.000000 | -0.000809 | -0.000124 | 1 |
| `m657_shared15_listwise1_seed6545` | 204 | yes | no | no | -0.000813 | -0.000003 | -0.000064 | -0.000927 | -0.000092 | 2 |
| `m657_shared15_listwise1_unclamped_seed6545` | 204 | yes | no | no | -0.000480 | +0.000243 | -0.000064 | -0.000575 | +0.000184 | 1 |
| `m659_shared15_allq_swap1_top100replay1_seed6545` | 204 | yes | no | yes | -0.000792 | -0.000090 | +0.000000 | -0.000282 | +0.000648 | 10 |
| `m659_shared15_swap1_top100replay1_seed6590` | 48 | yes | no | yes | -0.001359 | -0.000336 | +0.000000 | -0.001644 | -0.000637 | 1 |

## Interpretation

Across 30 trace-bearing runs, 29 runs reduced the training loss by the final epoch.  Among those, 16 runs moved O@100 downward from the first traced checkpoint, and 19 runs moved O@256 downward.  That pattern is inconsistent with a simple under-training explanation: deeper optimization frequently improves the surrogate loss without improving the dense-equivalence gate.

## Decision

Do not respond to the recent failures by simply increasing epochs or
batch count on M654/M657/M659-style losses.  The traces show that the
problem is objective/architecture alignment: loss can go down while
dense topK boundary safety gets worse.  The next trainable probe should
change the first-stage objective toward direct dense topK/rank-margin
preservation rather than scaling the same losses.
