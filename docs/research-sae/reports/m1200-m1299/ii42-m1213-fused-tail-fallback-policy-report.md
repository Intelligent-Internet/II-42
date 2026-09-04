# M1213 Fused-Tail Fallback Policy

M1213 tests the only follow-up left by M1212: replace the M1210 low
`top1_s0.10` action with a fused-tail proposal source.

Action family:

- high: `bm25_top_docs3_s0.10`
- mid: `bm25_top_docs3_s0.075`
- low: `fused_tail_docs3_s0.10`

The goal is not to find another threshold.  The question is whether the
safe-but-small fused-tail source can act as a better fallback inside the
ordered action policy.

## Smoke Result

Hard-row smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `low_fused_tail` | 0 | +0.001582 | +0.000658 | +0.000774 | +0.002524 | +0.000000 | 0.016482 |
| `fused_fallback_utility_or_cub_logistic` | 2 | +0.000170 | +0.001056 | +0.000951 | -0.000285 | -0.000522 | -0.011056 |
| `oracle_best3_actions` | 0 | +0.007535 | +0.006646 | +0.006291 | +0.005029 | +0.001085 | 0.081337 |
| `oracle_best4_with_baseline` | 0 | +0.008544 | +0.007490 | +0.007550 | +0.005471 | +0.001888 | 0.093118 |

Smoke already showed the split:

- The fused-tail action itself is safe and useful on hard rows.
- The learned qrels-free selector is not safe on hard rows.
- The oracle action family has a large ceiling.

## Full Shared15 Result

| Variant | Action Mix | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best4_with_baseline` | baseline 0.676 / high 0.206 / mid 0.030 / low 0.089 | 0 | +0.003229 | +0.007562 | +0.007882 | +0.006619 | +0.001013 | 0.068847 |
| `oracle_best3_actions` | high 0.822 / mid 0.055 / low 0.123 | 0 | +0.002974 | +0.007260 | +0.007447 | +0.006378 | +0.000681 | 0.064980 |
| `mid_top3_s0.075` | mid 1.000 | 1 | +0.001610 | +0.003509 | +0.003358 | +0.002682 | -0.000033 | 0.029799 |
| `high_top3_s0.10` | high 1.000 | 1 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 | 0.029555 |
| `fused_fallback_utility_or_cub_logistic` | high 0.744 / mid 0.117 / low 0.139 | 1 | +0.001304 | +0.003117 | +0.002697 | +0.001710 | -0.000067 | 0.022934 |
| `fused_fallback_any_metric_logistic` | high 0.761 / mid 0.098 / low 0.141 | 1 | +0.001582 | +0.002777 | +0.002490 | +0.000724 | -0.000207 | 0.017295 |
| `fused_fallback_rank_metric_logistic` | high 0.754 / mid 0.115 / low 0.130 | 1 | +0.001702 | +0.002489 | +0.002006 | +0.000584 | -0.000238 | 0.014957 |
| `low_fused_tail` | low 1.000 | 0 | +0.000376 | +0.000765 | +0.000957 | +0.001092 | +0.000081 | 0.008356 |

## Comparison To The Current Frontier

M1210 best learned policy:

- `ordered_utility_or_cub_logistic`
- dRecall +0.002009
- dMAP +0.003318
- dNDCG +0.002748
- dMRR +0.002275
- dCUB +0.000004
- score 0.030049

M1191 current deployable default:

- `veto_utility_or_cub_logistic`
- dRecall +0.002065
- dMAP +0.003412
- dNDCG +0.002557
- dMRR +0.002157
- dCUB +0.000002

M1213 does not beat either learned frontier.  Its best learned policy regresses
CUB, and its weighted score is materially lower than M1210.

The useful signal is the oracle:

- M1210 `oracle_best4_with_baseline`: score 0.060505
- M1213 `oracle_best4_with_baseline`: score 0.068847

Adding fused-tail as a fallback increases the action-family ceiling, but the
current qrels-free logistic selector cannot choose it safely.

## Decision

- Do not promote the fused-tail fallback ordered policy.
- Keep M1191 as the safer deployable default.
- Keep M1210 as the closest learned alternative.
- Keep fused-tail as a proposal source for future trained selector work.
- Stop same-family threshold or action-substitution sweeps.

## Lesson

The bottleneck is no longer action availability.  M1210, M1211, and M1213 all
show high oracle ceilings, but learned qrels-free selectors either plateau or
damage CUB.  The next useful experiment must change selector supervision or
observability, not swap another scale or fallback source.

## Next Direction

Run M1214 as an action-value selector audit:

- collect per-query action deltas for baseline, high, mid, low/top1, and
  fused-tail actions
- train or audit a held-out action-value model, not a binary harm classifier
- include action-specific movement features instead of only query-global risk
  features
- accept only if the learned policy beats M1191/M1210 while keeping CUB
  non-negative

If M1214 cannot beat the M1191/M1210 frontier, the local policy route is capped
and the next stage should return to a generated-posting objective or a richer
native proposal generator.

## Artifacts

- Smoke JSON: `runs/m1213_fused_tail_fallback_policy_smoke_v1/m1213_fused_tail_fallback_policy.json`
- Smoke Markdown: `runs/m1213_fused_tail_fallback_policy_smoke_v1/m1213_fused_tail_fallback_policy.md`
- Full JSON: `runs/m1213_fused_tail_fallback_policy_v1/m1213_fused_tail_fallback_policy.json`
- Full Markdown: `runs/m1213_fused_tail_fallback_policy_v1/m1213_fused_tail_fallback_policy.md`
- Script: `scripts/audit_m1213_fused_tail_fallback_policy.py`
