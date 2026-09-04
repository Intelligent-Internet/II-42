# M717 Risk-Aware Target-Set Selector

M717 tests whether M716 failed because the selector lacked query-relative
features and explicit negative-only risk control.

This remains first-stage only:

- no BM25;
- no reranker;
- no qrels-driven objective;
- no learned gate;
- no dataset-specific tuning;
- frozen P1.3 / M549U native signed-dot baseline.

## Motivation

M715 showed that a small target-like atom set is enough to move dense-boundary
pairs. M716 showed the existing selector cannot recover that set on eval.

M717 adds two targeted repairs:

1. Query-normalized features: raw atom features plus per-query z-score and
   max-relative forms.
2. Negative-only risk model: a separate classifier predicts negative-only
   atoms, and selection uses `target_probability - risk_penalty * risk`.

## Run

```bash
python3 scripts/train_m717_risk_aware_target_set_selector.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --limit-queries 25 \
    --budgets 16,32,64 \
    --scales 0.05,0.1 \
    --risk-penalties 0,0.5,1,2 \
    --output-root runs/m717_risk_aware_target_set_selector_v1
```

Runtime was `224.75s`. The cost is native scorer replay over all selected
variants; future sweeps should prune variants earlier.

## Training Stats

| Model | Rows | Positive share | AUC |
| --- | ---: | ---: | ---: |
| target classifier | 92160 | 0.024512 | 0.851492 |
| risk classifier | 92160 | 0.022222 | 0.840143 |

The AUCs are non-trivial but do not imply retrieval success. The gate is held
out native pair movement plus dense-head preservation.

## Eval Results

Best eval variants:

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target recall | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `riskaware_b64_s0.05_rp2` | 0.516667 | 0.520000 | 1 | 2 | 0.989211 | 0.005707 | 0.003516 | 0.003125 |
| `riskaware_b32_s0.1_rp2` | 0.516667 | 0.520000 | 1 | 2 | 0.986842 | 0.001902 | 0.002344 | 0.005469 |
| `riskaware_b16_s0.1_rp0.5` | 0.516667 | 0.520000 | 5 | 6 | 0.960000 | 0.100190 | 0.246875 | 0.151562 |
| `riskaware_b32_s0.05_rp2` | 0.513333 | 0.520000 | 0 | 2 | 0.991842 | 0.001902 | 0.002344 | 0.005469 |
| `riskaware_b16_s0.05_rp2` | 0.513333 | 0.520000 | 0 | 2 | 0.993947 | 0.001268 | 0.003125 | 0.007812 |

M717 does not pass the canary gate.

## Train/Eval Pattern

The train split still shows movement:

| Variant | Train pair success | Baseline | Fixed | Regressed | Top95 | Target recall | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `riskaware_b32_s0.1_rp0.5` | 0.574786 | 0.544872 | 17 | 3 | 0.934386 | 0.193394 | 0.124479 |
| `riskaware_b32_s0.1_rp0` | 0.574786 | 0.544872 | 18 | 4 | 0.933333 | 0.192525 | 0.149479 |
| `riskaware_b64_s0.1_rp0.5` | 0.574786 | 0.544872 | 19 | 5 | 0.912982 | 0.309865 | 0.114323 |

This repeats the M716 failure pattern: train improves, eval does not.

## Dataset Observation

Some dataset-local variants improve, but not the same variant:

- ArguAna best local rows are around `riskaware_b32_s0.05_rp0` or
  `riskaware_b64_s0.05_rp0.5`.
- CQADupStack prefers `riskaware_b16_s0.1_rp0.5`.
- FiQA prefers high risk penalty, e.g. `riskaware_b64_s0.1_rp2`.
- SCIDOCS prefers lower risk penalty and larger movement.

This is not acceptable as a first-stage global model. It shows the selector
surface is still unstable across query distributions.

## Interpretation

M717 is a sharper negative result.

The risk model can reduce negative-only leakage, but it also suppresses target
recovery. Low risk penalties recover more target atoms but also select too many
negative-only atoms and regress pair success. High risk penalties preserve head
overlap, but target recall collapses near zero.

Therefore the current scalar/query-normalized feature interface cannot
separate deep target atoms from risk atoms well enough. The missing signal is
not more training depth; it is more direct query-document or boundary-pair
interaction information.

## Decision

Do not scale M717.

The next useful step is not another classifier over the same candidate-row
features. It should add interaction evidence:

1. For each candidate atom, measure its direct signed support in the
   dense-positive boundary docs versus dense-negative boundary docs.
2. Add per-query pair interaction features, not only popularity/rank/vote
   features.
3. Keep the target-set objective at b16/b32 and candidate-impact direction.
4. Native pair success remains a downstream gate.

Suggested next probe: M718 pair-interaction target-set selector.

If M718 still cannot recover target sets at b16/b32, the selector route is
likely exhausted and the first-stage work should return to compiler/teacher
architecture rather than more selector training.

## Files

- `scripts/train_m717_risk_aware_target_set_selector.py`
- `runs/m717_risk_aware_target_set_selector_v1/m717_summary.json`
- `runs/m717_risk_aware_target_set_selector_v1/m717_report.md`
