# M1298 Query-CUB Pair Oracle Native Replay

## Purpose

M1297 showed that a global CUB-specific allowed atom prior is too coarse to
improve the M1288 pair/witness source.  M1298 tests the stronger oracle form:
use query-specific CUB target atoms and compose them with pair/witness source
selection.

This is not deployable.  It is a capacity test:

- if query-specific CUB target atoms improve pair/witness source construction,
  then approximating that source may be valuable;
- if even oracle query-specific composition does not help, CUB and pair should
  stay separate teacher signals rather than be merged.

## Command

```bash
python3 scripts/replay_m1298_query_cub_pair_oracle_native.py \
  --datasets arguana,cqadupstack,fiqa,scidocs \
  --sources pair,cub_front,cub_only \
  --budgets 96,192 \
  --risk-penalties 0.5,1 \
  --scales 0.05 \
  --limit-queries 25 \
  --output-root runs/m1298_query_cub_pair_oracle_native_limit25_v1
```

Outputs:

- `runs/m1298_query_cub_pair_oracle_native_limit25_v1/m1298_native.json`
- `runs/m1298_query_cub_pair_oracle_native_limit25_v1/m1298_native.md`

## Query-CUB Target Folds

| Heldout | Queries | TrainAtomRows | AllowedAtoms | MeanTargets |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 25 | 3000 | 79 | 0.120 |
| `cqadupstack` | 25 | 2919 | 53 | 0.240 |
| `fiqa` | 25 | 2776 | 54 | 0.280 |
| `scidocs` | 25 | 2798 | 30 | 0.440 |

The query-specific CUB target source is very sparse on this surface.

## Result

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | MinDatasetTop95 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cub_front_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.956579 | 0.981611 | 0.008333 |
| `cub_front_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.953947 | 0.972099 | 0.002214 |
| `cub_only_risk0.5_b192_s0.05` | 0.520000 | 0.520000 | 0 | 0 | 0.999737 | 0.998684 | 0.000000 | 0.000000 |

`cub_front` ties pair-only because there are too few query-CUB atoms to change
the ordering.  `cub_only` preserves the head by selecting almost no useful
pair-moving atoms.

## Interpretation

M1298 gives a clear answer for this merge path:

- query-specific CUB target atoms are too sparse to improve pair/witness source
  selection on this surface;
- CUB-only is safe but has no pair movement;
- CUB-front is effectively a no-op relative to pair-only.

This means the current CUB-specific teacher and pair/witness teacher should not
be merged by intersection/front-loading.  They remain useful independently:

- CUB-specific teacher: support-aware macro objective signal.
- Pair/witness teacher: dense-boundary rank movement signal.

But a new source must be constructed directly around the final objective.  A
simple composition of existing teacher outputs does not create a stronger
frontier.

## Decision

Stop CUB/pair merge experiments in this form:

- no global CUB prior plus pair;
- no query-specific CUB-front or CUB-only pair composition;
- no further thresholding around this merge without a new source.

Next valid direction:

1. Treat CUB and pair as separate diagnostic teachers.
2. Build a fresh source that is objective-native from the start, not a merge of
   two teacher outputs.
3. Candidate source should encode both movement and support constraints at
   generation time, likely by constructing document-pair-supported atoms with
   explicit protected-head witnesses.

## Verification

```bash
python3 -m py_compile scripts/replay_m1298_query_cub_pair_oracle_native.py
git diff --check -- scripts/replay_m1298_query_cub_pair_oracle_native.py
```

Passed.
