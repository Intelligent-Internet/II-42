# M705 Budgeted Set Selector Report

M705 tested whether the M704 budgeted set-selection oracle can be approximated
by a trainable global selector plus impact model.

This is still a first-stage dense-equivalence probe:

- no BM25 signal in ranking
- no reranker
- no qrels loss
- no learned gate from previous speculative lines
- frozen native semantic scorer and P1.3/M549U query/doc geometry

## Why This Was Run

M704 showed a strong oracle result on the expanded interface:

- `safe_fill_delta_target_b384_s0.02`
- pair success: `0.541176 -> 0.617647`
- fixed/regressed: `234/0`
- macro top95 overlap: `0.967023`

That proved the expanded atom interface contains useful boundary-crossing
atoms. M705 asks whether a trainable global selector can recover that oracle
without labels at inference time.

## Method

The selector uses candidate atom features from the expanded interface:

- `source384`
- `doc_atom_head=48`
- `max_atom_candidates_per_query=1536`

Training labels are generated from dense-boundary target atoms on the M653
train split. Evaluation is dev plus test split.

Models:

- `HistGradientBoostingClassifier` for target-atom probability
- `HistGradientBoostingRegressor` for impact magnitude/direction

Variants sweep:

- budgets: `24, 48, 96, 192`
- scales: `0.005, 0.01, 0.02, 0.05`
- impact sources: `candidate`, `predicted`

Gate:

- eval pair-success lift at least `0.01`
- fixed pairs at least `2x` regressed pairs
- top95 overlap at least `0.95`

## Canary Result

Run:

```bash
python3 scripts/train_m705_budgeted_set_selector.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --max-atom-train-rows 0 \
    --budgets 24,48,96,192 \
    --scales 0.005,0.01,0.02,0.05 \
    --impact-sources candidate,predicted \
    --output-root runs/m705_budgeted_set_selector_canary_v1
```

Model stats:

| Rows | Positives | Positive share | AUC | Impact MAE |
| ---: | ---: | ---: | ---: | ---: |
| 388608 | 9553 | 0.024583 | 0.829882 | 0.110391 |

Best eval variant:

| Variant | Pair success | Baseline | Lift | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `learned_predicted_b192_s0.01` | 0.563734 | 0.561041 | 0.002693 | 12 | 9 | 0.959625 |

This does not pass the gate.

Best train variant:

| Variant | Pair success | Baseline | Lift | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `learned_predicted_b48_s0.02` | 0.535457 | 0.529805 | 0.005653 | 30 | 19 | 0.954982 |

The train split has weak positive movement, but it is not strong and does not
transfer reliably to eval.

## Interpretation

M705 is not a training-depth success case.

The expanded interface is valid because M704 oracle movement is large and safe.
The failure is the learned selector/impact approximation:

- target-atom classification AUC is non-trivial, but not enough for boundary
  movement;
- safer low-scale variants preserve top95 but barely move pair success;
- stronger variants move more pairs but regress too many and spend top95;
- train has weak gains, eval does not pass the gate.

This means the current feature set and objective can identify some target-like
atoms, but cannot decide which atoms safely cross the native top100 boundary.

## Decision

Do not scale this M705 model directly.

The next useful step is M706 failure attribution:

1. Compare M704 oracle-selected atoms against M705 selected atoms.
2. Quantify target atom recall, rank, impact sign, and negative-only leakage by
   query bucket.
3. Determine whether the missing ingredient is feature coverage, query-context
   conditioning, impact calibration, or the label definition itself.
4. Only after M706 identifies a repairable gap should another trainable
   selector be attempted.

M704 remains a positive route signal. M705 is a useful negative result: the
oracle is not yet trainably recoverable with this simple global selector.

