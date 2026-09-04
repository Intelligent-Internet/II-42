# M1297 CUB-Prior Pair Native Replay

## Purpose

M1297 tests a structural source change after M1295/M1296 stopped selector
micro-tuning.

The idea is to combine two real signals:

- M1288 pair/witness source: strong native pair movement, but teacher-side.
- M1225 CUB-specific source: support-aware macro gain, but row-fragile.

Instead of training another row-level gate, M1297 learns a cross-dataset
CUB-specific allowed atom prior and uses it to reorder or restrict the
pair/witness source before native replay.

## Command

```bash
python3 scripts/replay_m1297_cub_prior_pair_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,cub_front,cub_only \
  --budgets 96,192 \
  --risk-penalties 0.5,1 \
  --scales 0.05 \
  --limit-queries 25 \
  --output-root runs/m1297_cub_prior_pair_native_limit25_v1
```

Outputs:

- `runs/m1297_cub_prior_pair_native_limit25_v1/m1297_native.json`
- `runs/m1297_cub_prior_pair_native_limit25_v1/m1297_native.md`

## CUB Prior Folds

| Heldout | TrainAtomRows | AllowedAtoms |
| --- | ---: | ---: |
| `arguana` | 3000 | 79 |
| `cqadupstack` | 2919 | 53 |
| `fiqa` | 2776 | 54 |
| `scidocs` | 2798 | 30 |

## Result

Best eval variants:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `cub_front_risk1_b192_s0.05` | 0.680000 | 0.520000 | 48 | 0 | 0.959474 | 0.953947 | 0.972099 | 0.014974 |
| `cub_front_risk0.5_b192_s0.05` | 0.673333 | 0.520000 | 46 | 0 | 0.960263 | 0.953947 | 0.981611 | 0.020182 |
| `cub_only_risk0.5_b192_s0.05` | 0.500000 | 0.520000 | 0 | 6 | 0.981842 | 0.978138 | 0.070387 | 0.050484 |

`cub_front` slightly changes protected overlap, but loses pair success and
increases negative-only share.  `cub_only` mostly preserves the head by doing
little useful pair movement.

## Interpretation

The simple global CUB prior is too coarse:

- it does not preserve the M1288 pair/witness frontier;
- it does not reduce negative-only selection;
- it turns CUB-specific safety into conservative head preservation rather than
  useful rank movement.

This does not refute M1225.  M1225 is query/action-specific, while M1297 uses
a cross-dataset global allowed atom prior.  The negative result only says the
global prior cannot be bolted onto pair/witness as a source construction.

## Decision

Stop the `global CUB prior + pair/witness` branch.

The remaining useful question is more precise:

Can query-specific CUB target atoms and pair/witness target atoms share enough
support to form a stronger teacher/source?  If the oracle intersection/union
has no native frontier advantage, then CUB and pair should stay separate
teacher signals rather than be merged.

## Verification

```bash
python3 -m py_compile scripts/replay_m1297_cub_prior_pair_native.py
git diff --check -- scripts/replay_m1297_cub_prior_pair_native.py
```

Passed.
