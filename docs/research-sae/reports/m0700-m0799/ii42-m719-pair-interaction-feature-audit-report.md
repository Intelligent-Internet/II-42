# M719 Pair-Interaction Feature Audit

M719 tests whether the failed selector line was under-trained or missing the
right information. It adds dense-boundary positive/negative document atom
interaction features and measures target-set recovery before any native scorer
replay.

This remains first-stage only:

- no BM25;
- no reranker;
- no qrels-driven objective;
- no learned gate;
- no dataset-specific tuning;
- frozen `P1.3 / M549U native signed-dot` baseline.

## Motivation

M716-M718 failed because scalar candidate-row selectors could not recover the
target atom sets found by the M715 oracle. There are two possible explanations:

1. the route was abandoned too early and only needed deeper training;
2. the selector surface lacked the information needed to identify useful atoms.

M719 tests the second explanation directly by adding pair-interaction features
computed from the dense-boundary positive and negative documents.

## Run

```bash
python3 scripts/audit_m719_pair_interaction_features.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --limit-queries 25 \
    --budgets 16,32,64 \
    --output-root runs/m719_pair_interaction_features_v1
```

Runtime was `19.47s`.

## Training Surface

The pair-feature model uses only interaction features:

- `pair_abs_delta`
- `pair_abs_ratio`
- `pair_candidate_align`
- `pair_count_delta`
- `pair_negative_abs`
- `pair_negative_count`
- `pair_negative_impact`
- `pair_net_impact`
- `pair_positive_abs`
- `pair_positive_count`
- `pair_positive_impact`
- `pair_positive_share`
- `pair_signed_count_delta`

Training summary:

| Metric | Value |
| --- | ---: |
| Train rows | 92160 |
| Target positive share | 0.024512 |
| Risk positive share | 0.022222 |
| Pair-HGB target AUC | 0.997423 |
| Pair-HGB risk AUC | 0.997286 |

The very high train AUC is not itself a success condition. The useful question
is whether held-out eval target-set recovery improves without selecting many
negative-only atoms.

## Eval Selection Surface

Best held-out eval rows:

| Variant | Queries | Target recall | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: |
| `pair_hgb_rp0_b64` | 40 | 0.924540 | 0.569531 | 0.017578 |
| `pair_hgb_rp05_b64` | 40 | 0.890932 | 0.548828 | 0.006250 |
| `pair_hgb_rp1_b64` | 40 | 0.860495 | 0.530078 | 0.001172 |
| `pair_hgb_rp0_b32` | 40 | 0.678503 | 0.835938 | 0.005469 |
| `pair_hgb_rp05_b32` | 40 | 0.636018 | 0.783594 | 0.000000 |
| `pair_hgb_rp1_b32` | 40 | 0.629677 | 0.775781 | 0.000000 |
| `pair_net_impact_b64` | 40 | 0.580850 | 0.357812 | 0.010547 |
| `pair_candidate_align_b64` | 40 | 0.576411 | 0.355078 | 0.010547 |

The best pair-HGB rows are far above the M716-M718 target-recall regime. The
deterministic single-feature rows are weaker, which indicates that the signal
is not one obvious scalar. The combined interaction surface is doing the work.

## Per-Dataset Pattern

Best `pair_hgb_rp0_b64` rows by dataset:

| Dataset | Queries | Target recall | Target selected | Negative-only |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 25 | 0.926901 | 0.594375 | 0.012500 |
| `cqadupstack` | 25 | 0.934737 | 0.555000 | 0.039375 |
| `fiqa` | 25 | 0.935078 | 0.603125 | 0.013125 |
| `scidocs` | 25 | 0.932184 | 0.506875 | 0.025625 |

The signal is not isolated to one dataset. `cqadupstack` has the highest
negative-only share and should be treated as the first stress row in native
replay.

## Interpretation

M719 is the first strong positive signal after M715.

The result argues against the simple "not enough training" explanation for
M716-M718. Those models were not merely shallow; they were trained on a weak
scalar candidate surface. Once the feature surface includes direct
positive/negative dense-boundary interactions, target-set recovery becomes
separable on held-out queries.

This does not yet prove retrieval improvement. M719 does not run the selected
sets through the native scorer, does not test pair success, and does not measure
dense overlap after boundary movement. It only proves that the missing
target-set information exists in pair interactions.

## Decision

Promote this line to native replay, but do not promote M719 itself as a model.

M720 should train or reuse the pair-HGB selector as a native-gated selector and
evaluate actual dense-boundary movement:

1. Select `b32` and `b64` sets with `pair_hgb_rp0`, `pair_hgb_rp05`, and
   `pair_hgb_rp1`.
2. Apply small candidate-impact scales such as `0.05` and `0.10`.
3. Replay on the native signed-dot surface.
4. Require held-out eval pair success to improve over baseline.
5. Require `fixed > regressed`.
6. Require `top95` and dense-overlap guards to stay above floor.
7. Reject variants that only improve train or only work on one dataset.

## Current Training-Depth Assessment

The current exploration ratio is acceptable only because each small test is
being used as a mechanism test, not as a final metric claim. Small-scale
experiments should answer whether the failure mode changes. M719 changes the
failure mode from "target sets cannot be recovered" to "target sets can be
recovered from pair-interaction features; native replay must now prove they
move ranking safely."

The next expansion should therefore be native replay, not deeper training of
the old scalar selector. If M720 native replay fails despite M719 target-set
recovery, the bottleneck is score application or boundary movement, not
selection depth.

## Files

- `scripts/audit_m719_pair_interaction_features.py`
- `runs/m719_pair_interaction_features_v1/m719_summary.json`
- `runs/m719_pair_interaction_features_v1/m719_report.md`

