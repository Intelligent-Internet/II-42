# M1250 Low-Action Reserve Native Replay

## Goal

M1249 showed that low-action reserve improves the target/harm frontier.  M1250
tests whether that source transfers to native retrieval metrics.

This replay uses query-time signed deltas from the action rows.  It is closer
to deployable behavior than mean teacher deltas, but it is still a bounded
research replay.

## Runs

- Smoke: `runs/m1250_low_action_reserve_native_smoke_v1/`
- Full shared15: `runs/m1250_low_action_reserve_native_v1/`

## Full Shared15 Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m1250_top8_low0_s1 | 8.958 | 0 | +0.001562 | +0.003536 | +0.003268 | +0.002995 | +0.000092 | +0.031034 |
| m1250_top8_low0_s0.5 | 8.958 | 0 | +0.000460 | +0.002249 | +0.001935 | +0.001634 | +0.000036 | +0.016222 |
| m1250_top8_low2_s0.5 | 9.928 | 0 | +0.000736 | +0.002339 | +0.001981 | +0.001152 | +0.000199 | +0.017163 |
| m1250_top8_low4_s0.5 | 11.806 | 0 | +0.000642 | +0.002116 | +0.001711 | +0.000963 | +0.000201 | +0.015111 |
| m1250_top8_low8_s0.5 | 15.013 | 0 | +0.001557 | +0.002029 | +0.001422 | +0.001025 | +0.000124 | +0.018888 |
| m1250_top8_low2_s1 | 9.928 | 1 | +0.001448 | +0.003477 | +0.003271 | +0.002472 | -0.000156 | +0.025088 |
| m1250_top8_low4_s1 | 11.806 | 1 | +0.001164 | +0.003140 | +0.003094 | +0.002636 | -0.000132 | +0.023259 |
| m1250_top8_low8_s1 | 15.013 | 1 | +0.000211 | +0.003365 | +0.003384 | +0.003660 | -0.000125 | +0.021994 |

## Interpretation

The native winner is not low-action reserve.  The best safe variant is
`m1250_top8_low0_s1`, which is all-actions top8 with query-time signed deltas.
It improves all five macro metrics with no negative metric:

- Recall@100 +0.001562
- MAP@100 +0.003536
- NDCG@10 +0.003268
- MRR@20 +0.002995
- CUB +0.000092

Low-action reserve remains useful as a target/harm source-frontier result, but
in native replay it introduces either no extra benefit or CUB risk.

## Decision

Promote the next line as query-time signed-delta transfer, not low-action
reserve.

The next experiment should isolate and validate `all-actions top8 +
query-time signed delta`:

1. compare signed sum vs max-action signed delta vs mean-teacher delta;
2. run per-dataset harm/benefit audit for the top8 query-delta route;
3. if stable, promote this route to the next native policy candidate.
