# M1296 Native-Outcome Pair Ranker

## Purpose

M1295 showed that hand-scored movement/support additions do not improve the
pair/witness native frontier.  M1296 tests a stronger version of the same idea:
label pair/witness candidate atoms with train-split native replay outcomes and
train a small regressor to rank eval atoms.

This is designed to answer whether the missing piece is just a learned scalar
ranking objective over the existing candidate features.

## Command

```bash
python3 scripts/replay_m1296_native_outcome_pair_ranker.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,learned \
  --budgets 96,192 \
  --scales 0.05 \
  --label-candidates-per-query 32 \
  --limit-queries 25 \
  --output-root runs/m1296_native_outcome_pair_ranker_limit25_v2
```

Outputs:

- `runs/m1296_native_outcome_pair_ranker_limit25_v2/m1296_native.json`
- `runs/m1296_native_outcome_pair_ranker_limit25_v2/m1296_native.md`

## Label Stats

| Rows | PositiveShare | ZeroShare | Mean | Min | Max | TrainAUC |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1920 | 0.023958 | 0.974479 | 0.003516 | -0.250000 | 0.250000 | 0.994037 |

The label is extremely sparse.  The model can separate the train labels, but
that separation does not transfer to eval.

## Eval Result

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_b96_s0.05` | 0.660000 | 0.520000 | 42 | 0 | 0.963684 | 0.957895 | 0.970197 | 0.014323 |
| `learned_b96_s0.05` | 0.496667 | 0.520000 | 3 | 10 | 0.976842 | 0.970526 | 0.208624 | 0.066406 |
| `learned_b192_s0.05` | 0.490000 | 0.520000 | 3 | 12 | 0.971316 | 0.965263 | 0.271401 | 0.055729 |

The learned native-outcome ranker preserves protected overlap more strongly,
but it loses the pair frontier and recovers far fewer target atoms.

## Interpretation

M1296 is a negative result for the current feature view:

- native-outcome supervision is available but very sparse;
- train separability is high, so the model can memorize/local-fit the labels;
- eval transfer is poor, so this is not just an underpowered hand-weighting
  problem;
- the pair/witness teacher remains useful, but turning it into a scalar atom
  selector over current features is not enough.

This matches the broader M1000+ lesson: useful atoms exist, but the qrels-free
observable signal for safe selection is insufficient in the current row-level
feature space.

## Decision

Stop the current scalar atom-selector line:

- no more threshold/grid/filter variants over the same features;
- no more hand-scored movement/support additions;
- no more single regressor/classifier over row features as the primary path.

The next route should change the structure, not just the selector:

1. Build a source/objective that creates harm separation before selection.
2. Keep pair/witness and CUB-specific signals as teacher diagnostics.
3. Move toward structured candidate generation or query-level policy that
   decides source/action/budget jointly, then only uses atom ranking inside the
   selected source.

## Verification

```bash
python3 -m py_compile scripts/replay_m1296_native_outcome_pair_ranker.py
git diff --check -- scripts/replay_m1296_native_outcome_pair_ranker.py
```

Passed.
