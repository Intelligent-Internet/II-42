# M1302 Protected Set Objective Native Replay

## Question

M1301 showed that set-level pair/witness coverage can improve pair movement,
but only by spending protected-head overlap.  M1302 asks whether protected-head
preservation can be folded into the set objective itself.

The objective adds two native-context terms:

- reward alignment with baseline protected-head documents;
- penalize alignment with baseline tail documents.

The test remains a small capacity probe on the same four-dataset `limit25`
surface.

## Run

Command:

```bash
python3 scripts/replay_m1302_protected_set_objective_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,set_cov1,set_protect025,set_protect05,set_tailguard05 \
  --budgets 96,192 \
  --risk-penalties 0.5,1 \
  --scales 0.05 \
  --feature-groups pair \
  --limit-queries 25 \
  --output-root runs/m1302_protected_set_objective_limit25_v1
```

Artifacts:

- JSON: `runs/m1302_protected_set_objective_limit25_v1/m1302_native.json`
- Markdown: `runs/m1302_protected_set_objective_limit25_v1/m1302_native.md`

## Result

Best movement variant and pair-only control:

| Variant | PairSuccess | Fixed | Regressed | Top95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `set_cov1_pair_risk0.5_b192_s0.05` | 0.706667 | 56 | 0 | 0.942105 | 0.974001 | 0.008203 |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 54 | 0 | 0.959211 | 0.981611 | 0.008333 |

Best protected variants:

| Variant | PairSuccess | Fixed | Regressed | Top95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `set_protect025_pair_risk1_b192_s0.05` | 0.696667 | 53 | 0 | 0.966842 | 0.951173 | 0.001953 |
| `set_protect025_pair_risk0.5_b192_s0.05` | 0.696667 | 53 | 0 | 0.965263 | 0.974001 | 0.008073 |
| `set_protect05_pair_risk1_b192_s0.05` | 0.690000 | 51 | 0 | 0.967632 | 0.945466 | 0.001693 |
| `set_tailguard05_pair_risk0.5_b192_s0.05` | 0.690000 | 51 | 0 | 0.962895 | 0.968928 | 0.007812 |

## Interpretation

M1302 confirms the tradeoff rather than solving it:

- M1301-style `set_cov1` keeps the best movement, but top95 drops from
  `0.959211` to `0.942105`.
- Protected variants recover top95 above the pair-only control, but pair
  success falls below the pair-only control.
- The protected objective therefore changes the frontier, but does not create a
  better one.

This is not a training-depth issue in the current interface.  It is a conflict
between the pair/witness movement source and protected-head preservation when
both are applied as post-hoc set-selection terms.

## Decision

Do not scale M1302 to full shared15.

Stop this family:

- no finer protected-head weight grid;
- no extra top95 guard over the same selected atom rows;
- no promotion of `set_cov1` despite its pair movement, because it breaks the
  protected-head floor.

The next useful direction must change the source or loss so that protection is
native to atom generation, not recovered through a scoring term after candidate
construction.

Concrete next branch:

1. Build a protected-pair teacher where positive movement labels are defined
   only for atoms that also satisfy protected-head floor constraints.
2. Audit whether that teacher still has enough target coverage.
3. Only if target coverage survives, run native replay against the M1288
   pair-only control.
