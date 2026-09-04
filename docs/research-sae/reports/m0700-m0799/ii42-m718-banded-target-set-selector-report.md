# M718 Banded Target-Set Selector

M718 reuses the M717 target/risk classifiers and changes only set
construction. Instead of global top-k, it selects atoms through rank-band
quotas.

This is still first-stage only:

- no BM25;
- no reranker;
- no qrels-driven objective;
- no learned gate;
- no dataset-specific tuning;
- frozen P1.3 / M549U native signed-dot baseline.

## Motivation

M717 showed that a risk-aware global selector does not pass eval. M718 checks
whether useful target atoms are hidden deeper in the candidate list and can be
recovered by banded selection.

Band modes:

- `balanced3`: split quota across ranks `1-384`, `385-768`, `769+`.
- `tail_focus`: reserve more quota for `769+`.

## Run

```bash
python3 scripts/train_m718_banded_target_set_selector.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --limit-queries 25 \
    --budgets 16,32 \
    --scales 0.05,0.1 \
    --risk-penalties 0,0.5 \
    --band-modes balanced3,tail_focus \
    --output-root runs/m718_banded_target_set_selector_v1
```

Runtime was `158.67s`.

## Eval Results

Best held-out eval rows:

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target recall | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `balanced3_b32_s0.1_rp0` | 0.516667 | 0.520000 | 4 | 5 | 0.968158 | 0.093215 | 0.114844 | 0.077344 |
| `tail_focus_b16_s0.05_rp0.5` | 0.513333 | 0.520000 | 1 | 3 | 0.985789 | 0.038681 | 0.095312 | 0.051562 |
| `tail_focus_b16_s0.1_rp0.5` | 0.513333 | 0.520000 | 1 | 3 | 0.975000 | 0.038681 | 0.095312 | 0.051562 |
| `balanced3_b16_s0.05_rp0.5` | 0.513333 | 0.520000 | 2 | 4 | 0.984737 | 0.044388 | 0.109375 | 0.064062 |
| `tail_focus_b32_s0.05_rp0.5` | 0.513333 | 0.520000 | 2 | 4 | 0.982632 | 0.062777 | 0.077344 | 0.058594 |

M718 does not pass the held-out eval gate.

## Train/All Pattern

The train split does improve:

| Variant | Train pair success | Baseline | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `tail_focus_b16_s0.1_rp0` | 0.574786 | 0.544872 | 14 | 0 | 0.971754 |
| `balanced3_b32_s0.1_rp0` | 0.570513 | 0.544872 | 13 | 1 | 0.962281 |
| `tail_focus_b32_s0.1_rp0` | 0.568376 | 0.544872 | 13 | 2 | 0.966140 |

The all-split aggregate also improves for some variants, but this is not an
acceptable gate because train rows dominate part of that movement. The held-out
eval result is the decision surface.

## Interpretation

M718 does not rescue M717.

Banded selection improves target recall compared with high-risk-penalty M717,
but it still cannot reach the M715 oracle regime. Eval target recall remains
around `0.04-0.09` for the best rows, far below the M715 target-set condition.
The best eval row has fixed/regressed `4/5`, so the selected set is not safely
moving the dense-boundary pairs.

This rules out a simple explanation: M717 did not fail merely because global
top-k ignored deeper atoms. The target/risk scores themselves are not
generalizing enough on held-out queries.

## Decision

Do not scale M718.

The selector route has now tested:

- pointwise target selector (M705/M716);
- pair-impact selector (M714);
- query-normalized risk-aware selector (M717);
- banded set construction over target/risk scores (M718).

All fail held-out eval. The remaining useful path is not another variant of
scalar candidate-row selection. The next experiment needs new information:

1. Direct query-document or boundary-pair interaction features.
2. A compiler/teacher architecture that generates target-like sets from the
   dense root, instead of selecting weak atoms from scalar candidate rows.

Suggested next step: M719 pair-interaction feature audit. If direct interaction
features can separate target and negative-only atoms, train a model on that
surface. If they cannot, stop the selector route and return to compiler/teacher
architecture.

## Files

- `scripts/train_m718_banded_target_set_selector.py`
- `runs/m718_banded_target_set_selector_v1/m718_summary.json`
- `runs/m718_banded_target_set_selector_v1/m718_report.md`
