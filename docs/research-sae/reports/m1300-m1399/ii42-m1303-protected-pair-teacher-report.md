# M1303 Protected-Pair Teacher Audit

## Question

M1302 showed that adding protected-head terms after candidate construction
cannot jointly improve pair movement and top95 overlap.  M1303 moves the
constraint upstream and asks:

> Can we construct a protected-pair teacher that keeps enough pair/witness
> target coverage before any native replay?

This is a source audit only.  It does not run the native scorer.

## Run

Command:

```bash
python3 scripts/audit_m1303_protected_pair_teacher.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --limit-queries 25 \
  --output-root runs/m1303_protected_pair_teacher_limit25_v1
```

Artifacts:

- JSON: `runs/m1303_protected_pair_teacher_limit25_v1/m1303_teacher.json`
- Markdown: `runs/m1303_protected_pair_teacher_limit25_v1/m1303_teacher.md`

## Eval Coverage

| Policy | TargetRecall | VisibleTargetRecall | TargetShare | RiskShare | Selected/Query | QueryAnyTarget |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_target` | 0.867470 | 0.882012 | 1.000000 | 0.000000 | 34.200000 | 1.000000 |
| `pair_target_head_any` | 0.767280 | 0.780142 | 1.000000 | 0.000000 | 30.250000 | 1.000000 |
| `pair_target_head_ge_tail` | 0.213697 | 0.217279 | 1.000000 | 0.000000 | 8.425000 | 1.000000 |
| `pair_target_tail_zero` | 0.027901 | 0.028369 | 1.000000 | 0.000000 | 1.100000 | 0.725000 |
| `pair_target_head2_tail` | 0.064680 | 0.065764 | 1.000000 | 0.000000 | 2.550000 | 0.900000 |

## Interpretation

The strict protected policies are too sparse:

- `head_ge_tail` keeps only `0.213697` target recall.
- `tail_zero` and `head2_tail` are effectively unusable.

But `pair_target_head_any` is viable as a source:

- it retains `0.767280 / 0.867470 = 88.5%` of pair-target recall;
- it keeps `QueryAnyTarget = 1.0`;
- it has no selected negative-only risk in this teacher view;
- it is stable across the four small-surface datasets.

This means M1302 did not fail because protected constraints necessarily kill
the source.  It failed because protection was introduced as a late scoring term
rather than as part of source construction.

## Decision

Proceed to one small native replay:

- source: `pair_target_head_any`;
- surface: the same four-dataset `limit25`;
- control: M1288 `pair_pair_risk0.5_b192_s0.05`;
- required pass: pair success must beat or match pair-only while top95 does not
  drop below pair-only.

If native replay fails, stop this protected-pair teacher line.  The remaining
problem would be impact calibration / native score geometry, not source
coverage.
