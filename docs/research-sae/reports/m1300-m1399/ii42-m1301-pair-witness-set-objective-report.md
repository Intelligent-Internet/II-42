# M1301 Pair/Witness Set Objective Native Replay

## Question

M1300 stopped selector/source churn and proposed one valid next test:
select added atoms as a set, not as independent row-level scores.

M1301 tests a small set-level pair/witness objective:

- keep the M1288 pair/witness model score as the movement base;
- greedily reward newly covered dense-boundary positive witness documents;
- greedily penalize newly covered dense-boundary negative witness documents;
- replay selected atoms through the native P1 scorer;
- compare directly against the M1288 pair-only control.

This is a capacity probe.  It still uses dense-boundary witness documents, so it
is not deployable.

## Run

Command:

```bash
python3 scripts/replay_m1301_pair_witness_set_objective_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,set_cov025,set_cov05,set_cov1 \
  --budgets 96,192 \
  --risk-penalties 0.5,1 \
  --scales 0.05 \
  --feature-groups pair \
  --limit-queries 25 \
  --output-root runs/m1301_pair_witness_set_objective_limit25_v1
```

Artifacts:

- JSON: `runs/m1301_pair_witness_set_objective_limit25_v1/m1301_native.json`
- Markdown: `runs/m1301_pair_witness_set_objective_limit25_v1/m1301_native.md`

## Result

Best set objective versus pair-only control:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | Top100 | Top256 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `set_cov1_pair_risk0.5_b192_s0.05` | 0.706667 | 0.520000 | 56 | 0 | 0.942105 | 0.951250 | 0.954199 | 0.974001 | 0.008203 |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.957000 | 0.962109 | 0.981611 | 0.008333 |

The set objective improves pair success by `+0.006667` and fixes two more
dense-boundary pairs with no pair regressions.  However, it spends protected
head geometry:

- Top95 drops by `-0.017105`.
- Top100 drops by `-0.005750`.
- Top256 drops by `-0.007910`.
- Target recall drops by `-0.007609`.

## Per-Dataset Shape

Best variant by dataset:

| Dataset | Best Variant | PairSuccess | Fixed | Regressed | Top95 |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `set_cov1_pair_risk0.5_b192_s0.05` | 0.718750 | 22 | 0 | 0.940081 |
| `cqadupstack` | `pair_pair_risk0.5_b192_s0.05` | 0.666667 | 11 | 0 | 0.956579 |
| `fiqa` | `set_cov025_pair_risk0.5_b192_s0.05` | 0.661765 | 9 | 0 | 0.948538 |
| `scidocs` | `set_cov05_pair_risk1_b192_s0.05` | 0.776316 | 15 | 0 | 0.941053 |

The set objective helps `arguana`, `fiqa`, and `scidocs`, but not
`cqadupstack`.  The row-level shape is therefore still mixed, and the gain is
not a safe global frontier improvement.

## Interpretation

M1301 is not a breakthrough, but it is informative:

- Set-level coverage can create additional pair movement.
- The improvement is real enough to reject the idea that set interaction is
  completely useless.
- The improvement is not clean enough to scale, because it trades movement for
  protected-head overlap.
- This repeats the broader pattern from M1251-M1253 and M1295-M1299: useful
  movement exists, but post-hoc selection still lacks a stable safety signal.

## Decision

Do not scale M1301 to full shared15.

Stop ordinary coverage-weight variants over this interface:

- no broader run of `set_cov1` as a default candidate;
- no fine grid over coverage weights;
- no simple guard around top95 from the same feature family.

The next valid branch must change the training interface or candidate
construction.  A useful next test should make protected-head preservation part
of the selected-set objective itself, not a post-hoc gate after native replay.

Minimum requirement for the next branch:

1. Beat `pair_pair_risk0.5_b192_s0.05` on pair success.
2. Keep top95 at or above the pair-only control, or explicitly recover it
   inside the objective.
3. Show the same small gate on the four-dataset `limit25` surface before any
   full shared15 replay.
