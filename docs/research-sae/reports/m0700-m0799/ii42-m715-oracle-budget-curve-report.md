# M715 Oracle Budget Curve

M715 quantifies whether M713/M714 failed because single atoms are too weak, or
because the trainable selector failed to recover a small target-like atom set.

This is a first-stage diagnostic only:

- no BM25;
- no reranker;
- no learned gate;
- no dataset-specific tuning;
- frozen P1.3 / M549U native signed-dot baseline.

The run reuses the M704 upper-bound evaluator, but changes the question from
"can a large oracle set work" to "how small can the target-like set be before
native boundary movement becomes useful?"

## Runs

| Run | Datasets | Queries / dataset | Modes | Impact sources | Budgets | Scales |
| --- | --- | ---: | --- | --- | --- | --- |
| canary | arguana, cqadupstack, fiqa, scidocs | 10 | target_delta | candidate,target | 1..128 | 0.05,0.10 |
| main | arguana, cqadupstack, fiqa, scidocs | 25 | target_delta | candidate,target | 1..128 | 0.05,0.10 |

The main run command:

```bash
python3 scripts/audit_m704_budgeted_set_selection_feasibility.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --limit-queries 25 \
    --budgets 1,2,4,8,16,32,64,128 \
    --scales 0.05,0.1 \
    --modes target_delta \
    --impact-sources candidate,target \
    --output-root runs/m715_oracle_budget_curve_v1
```

## Candidate-Impact Budget Curve

Candidate impact is the more realistic direction source. It uses the candidate
row impact rather than dense-positive/negative doc-derived target impact.

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Min dataset Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `target_delta_candidate_b1_s0.05` | 0.540365 | 0.535156 | 4 | 0 | 0.992000 | 0.989895 |
| `target_delta_candidate_b2_s0.05` | 0.542969 | 0.535156 | 6 | 0 | 0.989368 | 0.986947 |
| `target_delta_candidate_b4_s0.05` | 0.548177 | 0.535156 | 10 | 0 | 0.986526 | 0.982737 |
| `target_delta_candidate_b8_s0.05` | 0.554688 | 0.535156 | 15 | 0 | 0.983474 | 0.981474 |
| `target_delta_candidate_b16_s0.05` | 0.570312 | 0.535156 | 27 | 0 | 0.978421 | 0.975579 |
| `target_delta_candidate_b32_s0.05` | 0.593750 | 0.535156 | 45 | 0 | 0.974947 | 0.970947 |
| `target_delta_candidate_b32_s0.10` | 0.630208 | 0.535156 | 73 | 0 | 0.954211 | 0.951579 |
| `target_delta_candidate_b64_s0.05` | 0.600260 | 0.535156 | 50 | 0 | 0.974842 | 0.970526 |

## Per-Dataset Check

`target_delta_candidate_b32_s0.05`:

| Dataset | Pair success | Baseline | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | 0.542553 | 0.468085 | 14 | 0 | 0.978947 |
| cqadupstack | 0.659574 | 0.617021 | 8 | 0 | 0.971789 |
| fiqa | 0.653061 | 0.607143 | 9 | 0 | 0.970947 |
| scidocs | 0.520408 | 0.448980 | 14 | 0 | 0.978105 |

`target_delta_candidate_b32_s0.10`:

| Dataset | Pair success | Baseline | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | 0.585106 | 0.468085 | 22 | 0 | 0.957895 |
| cqadupstack | 0.686170 | 0.617021 | 13 | 0 | 0.951579 |
| fiqa | 0.673469 | 0.607143 | 13 | 0 | 0.953263 |
| scidocs | 0.576531 | 0.448980 | 25 | 0 | 0.954105 |

## Interpretation

M715 gives a clearer answer than M713/M714:

- Single atoms are too weak.
- Large oracle sets are not the only working regime.
- A small target-like set of roughly `16-32` atoms is already useful.
- Candidate-impact direction is sufficient; the stronger target-impact oracle
  moves more, but spends too much top95 at higher scales.
- The effect is not a one-dataset artifact. All four checked datasets improve
  with zero regressions on the selected candidate-impact variants.

This means the current bottleneck is not training duration. It is target-like
set recovery. M714 failed because the learned selector did not actually select
target atoms on eval, not because the native scorer cannot use a small atom set.

## Decision

Keep the route active and narrow the next training target.

The next useful model is not a generic pair-impact selector over the full
candidate list. It should be a target-set recovery model with hard gates:

1. Optimize held-out target-like atom recall at budget `16-32`.
2. Preserve candidate-impact direction instead of predicting a free residual.
3. Reject checkpoints that improve pair movement while selecting no target-like
   atoms.
4. Use native pair success only as a downstream gate, not as the only label.

Suggested next probe: M716 target-set recovery selector.

Acceptance for M716 should be stricter than M714:

- eval selected target share must be materially above the M714 zero-target
  failure mode;
- native pair success must improve over baseline;
- fixed pairs must exceed regressed pairs;
- top95 must stay above `0.97` for target-set candidates, or above `0.995` if
  only tiny budgets are selected.

If M716 cannot recover target-like sets at b16/b32, the issue is not training
depth but missing query/document context in the selector features.

## Files

- `runs/m715_oracle_budget_curve_canary_v1/m704_summary.json`
- `runs/m715_oracle_budget_curve_v1/m704_summary.json`
