# M1304 Protected-Pair Teacher Native Replay

## Question

M1303 showed that `pair_target_head_any` keeps enough target coverage in a
teacher/source audit.  M1304 tests whether that source converts into native
pair movement.

This is still a teacher-side capacity test.  It is not deployable because the
teacher uses dense-boundary target labels.

## Run

Command:

```bash
python3 scripts/replay_m1304_protected_pair_teacher_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,teacher_head_any,teacher_target \
  --budgets 96,192 \
  --risk-penalties 0.5 \
  --scales 0.05 \
  --feature-groups pair \
  --limit-queries 25 \
  --output-root runs/m1304_protected_pair_teacher_native_limit25_v1
```

Artifacts:

- JSON: `runs/m1304_protected_pair_teacher_native_limit25_v1/m1304_native.json`
- Markdown: `runs/m1304_protected_pair_teacher_native_limit25_v1/m1304_native.md`

## Result

| Variant | PairSuccess | Fixed | Regressed | Top95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_pair_risk0.5_b192_s0.05` | 0.700000 | 54 | 0 | 0.959211 | 0.981611 | 0.008333 |
| `pair_pair_risk0.5_b96_s0.05` | 0.660000 | 42 | 0 | 0.963684 | 0.970197 | 0.014323 |
| `teacher_target_pair_risk0.5_b192_s0.05` | 0.570000 | 15 | 0 | 0.979474 | 0.867470 | 0.000000 |
| `teacher_head_any_pair_risk0.5_b192_s0.05` | 0.566667 | 14 | 0 | 0.981053 | 0.767280 | 0.000000 |

## Interpretation

M1304 fails the native replay gate.

The teacher variants are very safe, but too weak:

- top95 improves to `0.979-0.981`;
- selected negative-only risk goes to `0`;
- pair success falls to `0.566-0.570`, far below pair control `0.700`.

This means the M1288 pair control does not win only by selecting clean
teacher-target atoms.  Its movement comes from a broader set of native-useful
atoms, including atoms that the current target-only teacher excludes.

## Decision

Stop the protected-pair teacher replay line.

Do not scale `teacher_head_any` or `teacher_target` to shared15.

The key lesson is structural:

- target-only source construction is too conservative;
- post-hoc set coverage can move pairs but spends protected overlap;
- protected target-only source preserves head but loses movement.

The next valid direction is not another target-filter replay.  It needs a
teacher that labels native-useful movement atoms directly, while keeping
protected-head constraints in the label, not only in the selector.  In other
words, the label should be based on marginal native effect or pair movement
under a protected floor, not just dense-boundary target membership.
