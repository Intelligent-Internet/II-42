# M716 Target-Set Recovery Selector

M716 tests whether the existing learnable target selector can recover the
M715 oracle regime when the sweep is aligned to the right budget and scale.

This is still first-stage only:

- no BM25;
- no reranker;
- no learned gate;
- no dataset-specific tuning;
- frozen P1.3 / M549U native signed-dot baseline.

## Why This Was Needed

M715 showed that a small target-like set is enough:

- `target_delta_candidate_b16_s0.05`: `0.535156 -> 0.570312`,
  fixed/regressed `27/0`, top95 `0.978421`;
- `target_delta_candidate_b32_s0.05`: `0.535156 -> 0.593750`,
  fixed/regressed `45/0`, top95 `0.974947`;
- `target_delta_candidate_b32_s0.10`: `0.535156 -> 0.630208`,
  fixed/regressed `73/0`, top95 `0.954211`.

M716 asks whether M705 failed because it did not test this budget/scale regime,
or because the selector itself cannot generalize.

## Run

```bash
python3 scripts/train_m705_budgeted_set_selector.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --limit-queries 25 \
    --budgets 16,32,64 \
    --scales 0.05,0.1 \
    --impact-sources candidate \
    --output-root runs/m716_target_set_recovery_selector_v1
```

Model stats:

| Rows | Positives | Positive share | AUC | Impact MAE |
| ---: | ---: | ---: | ---: | ---: |
| 92160 | 2259 | 0.024512 | 0.845182 | 0.111230 |

## Eval Results

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target visibility | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `learned_candidate_b64_s0.05` | 0.510000 | 0.520000 | 3 | 6 | 0.960000 | 0.303107 | 0.186719 | 0.140234 |
| `learned_candidate_b16_s0.05` | 0.506667 | 0.520000 | 1 | 5 | 0.976842 | 0.117311 | 0.289062 | 0.193750 |
| `learned_candidate_b16_s0.10` | 0.506667 | 0.520000 | 2 | 6 | 0.960526 | 0.117311 | 0.289062 | 0.193750 |
| `learned_candidate_b32_s0.05` | 0.500000 | 0.520000 | 0 | 6 | 0.968947 | 0.192137 | 0.236719 | 0.173437 |
| `learned_candidate_b32_s0.10` | 0.483333 | 0.520000 | 1 | 12 | 0.943684 | 0.192137 | 0.236719 | 0.173437 |

## Train/Eval Split Pattern

The train split does show movement:

| Variant | Train pair success | Baseline | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `learned_candidate_b16_s0.10` | 0.574786 | 0.544872 | 16 | 2 | 0.954912 |
| `learned_candidate_b32_s0.10` | 0.572650 | 0.544872 | 19 | 6 | 0.934211 |
| `learned_candidate_b64_s0.05` | 0.570513 | 0.544872 | 14 | 2 | 0.952632 |

That train-only lift does not transfer to eval. This is a selector
generalization failure, not an oracle-regime failure.

## Interpretation

M716 is a useful negative result.

It rules out the simplest explanation for M705: the problem was not merely
that the earlier sweep missed the b16/b32/b64 and scale 0.05/0.10 operating
region. In the exact regime where M715 oracle target sets work, the learned
selector still regresses eval pair success.

The failure shape is specific:

- target visibility is too low on eval (`0.117` at b16, `0.192` at b32,
  `0.303` at b64);
- negative-only leakage is high (`0.14-0.19`);
- train improves while eval regresses;
- stronger scales increase regression and spend top95.

This says the next bottleneck is target-set recovery under held-out query
distribution, not training depth or native scorer feasibility.

## Decision

Do not scale this M705-style selector.

The next credible direction is a selector feature/interface redesign:

1. Add query/document interaction context rather than relying mostly on scalar
   candidate features.
2. Train for target-set recall at b16/b32 explicitly, not just atom-level AUC.
3. Penalize negative-only leakage directly.
4. Keep native pair success as a downstream gate, not the only training label.

If the redesigned selector still cannot recover target-like sets at b16/b32,
then the route should move from selector training to compiler/teacher redesign.

## Files

- `runs/m716_target_set_recovery_selector_v1/m705_summary.json`
- `runs/m716_target_set_recovery_selector_v1/m705_report.md`
