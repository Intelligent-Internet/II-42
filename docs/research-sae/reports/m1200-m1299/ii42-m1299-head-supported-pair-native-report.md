# M1299 Head-Supported Pair Native Replay

## Purpose

M1297/M1298 stopped CUB/pair source composition.  M1299 tests another
objective-native source idea:

- keep M1288 pair/witness rank-moving candidates;
- prefer or restrict to atoms also supported by native baseline top95 document
  atoms;
- replay through the native P1 scorer.

The intent is to test whether protected-head document support can preserve the
head while keeping most pair/witness movement.

## Command

```bash
python3 scripts/replay_m1299_head_supported_pair_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,head_front,head_only,head_agree_front,head_agree_only \
  --budgets 96,192 \
  --risk-penalties 0.5,1 \
  --scales 0.05 \
  --limit-queries 25 \
  --output-root runs/m1299_head_supported_pair_native_limit25_v1
```

Outputs:

- `runs/m1299_head_supported_pair_native_limit25_v1/m1299_native.json`
- `runs/m1299_head_supported_pair_native_limit25_v1/m1299_native.md`

## Result

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `head_agree_only_risk1_b192_s0.05` | 0.686667 | 0.520000 | 50 | 0 | 0.963947 | 0.956579 | 0.911858 | 0.002214 |
| `head_only_risk1_b192_s0.05` | 0.686667 | 0.520000 | 50 | 0 | 0.963947 | 0.956579 | 0.911858 | 0.002214 |
| `head_agree_only_risk0.5_b192_s0.05` | 0.686667 | 0.520000 | 50 | 0 | 0.962368 | 0.958947 | 0.921370 | 0.008073 |
| `head_front_risk0.5_b192_s0.05` | 0.510000 | 0.520000 | 5 | 8 | 0.957632 | 0.954386 | 0.535828 | 0.079036 |

The best remains pair-only.

`head_only` and `head_agree_only` improve protected overlap but lose pair
success and target recall.  `head_front` is worse: it creates regressions and
selects many noisy atoms.

## Interpretation

Protected-head support is not a rank-moving source.  It behaves like a
conservative constraint:

- it can raise top95 overlap;
- it reduces the amount of useful pair movement;
- it does not create a new pair-success frontier;
- front-loading supported atoms is actively harmful.

This is useful evidence because it separates two roles:

- pair/witness carries the rank movement;
- protected-head support can only act as a guard or regularizer.

It should not be used as a primary source constructor.

## Decision

Stop this source-construction branch:

- do not scale `head_front`;
- do not tune support thresholds;
- do not replace pair/witness ordering with protected-head ordering.

If protected-head support is used later, it should be an explicit constraint in
a generated objective, not a source ordering rule.

The broader conclusion after M1295-M1299:

- hand scoring failed;
- native-outcome scalar ranker failed;
- CUB/pair merge failed;
- protected-head source support failed.

The remaining viable route is not another source composition.  It is a new
objective/training interface where movement source and protection constraint
are optimized jointly.

## Verification

```bash
python3 -m py_compile scripts/replay_m1299_head_supported_pair_native.py
git diff --check -- scripts/replay_m1299_head_supported_pair_native.py
```

Passed.
