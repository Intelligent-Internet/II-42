# M1275 Action-Prior Weighted Signed-Sum

M1275 tests a fixed source/impact construction derived from earlier evidence:

- M1235: CUB target rows skew toward `high`, while harm rows skew toward
  `mid`/`low`.
- M1251: equal all-action `signed_sum` is a strong query-time native signal.

The hypothesis is that fixed action-prior weights might reduce harm inside the
source/impact construction without learning an action router or adding a gate.

## Run

- Surface: hard-row smoke
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`
- Output JSON:
  `runs/m1275_action_prior_weighted_signed_sum_smoke_v1/m1275_action_prior_weighted_signed_sum.json`
- Output detail:
  `runs/m1275_action_prior_weighted_signed_sum_smoke_v1/m1275_action_prior_weighted_signed_sum.md`

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1275_equal_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1275_harm_down_s1` | 4.602 | 0 | +0.000289 | +0.000387 | +0.000153 | +0.000890 | +0.000015 | +0.004707 |
| `m1275_target_prior_s1` | 4.602 | 1 | +0.000365 | +0.000433 | +0.000114 | +0.000890 | -0.000788 | -0.015354 |
| `m1275_log_ratio_s1` | 4.602 | 2 | +0.000674 | +0.000354 | -0.000064 | +0.000220 | -0.000803 | -0.016780 |
| `m1275_high_only_s1` | 1.851 | 2 | +0.001120 | +0.000298 | -0.000381 | +0.000778 | -0.000788 | -0.017006 |
| `m1275_no_low_s1` | 1.851 | 1 | +0.001538 | +0.000464 | +0.000357 | +0.000566 | -0.001591 | -0.030438 |

## Interpretation

Fixed action-prior weighting does not improve the frontier.

- `harm_down` removes the NDCG regression and is macro-safe, but it is weaker
  than equal signed_sum.
- Variants that strongly suppress `low` or emphasize `high` lose CUB.
- This matches the older action-router failures: action context is meaningful,
  but hard-coded action preference throws away support.

The important detail is that this is not a learned-router failure.  Even a
fixed, theory-backed prior cannot safely beat equal signed_sum on the hard-row
surface.  The current action context should therefore remain a diagnostic or
training signal, not a direct source-weighting policy.

## Decision

Do not scale M1275.

The retained route facts are now:

1. Equal signed_sum remains the best simple query-time impact signal.
2. Uniform impact geometry can repair some hard-row ranking loss.
3. Fixed action prior and low-tail reserve do not compose into a stronger
   standalone source.
4. The next useful move must be a generated objective/source that optimizes
   dense/native support and retrieval gain together, not another fixed
   hand-designed source transform.
