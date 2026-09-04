# M1265 Uniform Blend Observability

## Question

M1264 showed that `uniform_l1` improves full `shared15` macro score over raw
signed-sum, but loses average query-level score against raw.  M1265 asks whether
that uniform-help / uniform-harm split is observable from source geometry before
building any query-time blend, gate, or learned calibrator.

This is an observability audit, not a native policy promotion.

## Run

- Script: `scripts/audit_m1265_uniform_blend_observability.py`
- Output root: `runs/m1265_uniform_blend_observability_v1/`
- JSON:
  `runs/m1265_uniform_blend_observability_v1/m1265_uniform_blend_observability.json`
- Markdown:
  `runs/m1265_uniform_blend_observability_v1/m1265_uniform_blend_observability.md`
- Surface: full `shared15`
- Query count: `1342`

## Uniform vs Raw Baseline

Query-level uniform-vs-raw:

| Wins | Losses | Ties | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 176 | 196 | 970 | +0.000336 | +0.000124 | -0.000099 | -0.000097 | +0.000301 | -0.000008 |

This query-average view is stricter than the M1264 macro summary.  It confirms
the same qualitative result: uniform helps Recall/CUB but creates enough
rank-sensitive losses that it cannot be used blindly.

## Feature Separability

Best single-feature separability:

| Feature | OrientedAUC | AUC | Effect | BenefitMean | HarmMean |
| --- | ---: | ---: | ---: | ---: | ---: |
| `abs_std` | 0.6510 | 0.6510 | -0.0320 | 0.003508 | 0.003555 |
| `score_std` | 0.6510 | 0.6510 | -0.0320 | 0.003508 | 0.003555 |
| `abs_cv` | 0.6213 | 0.6213 | -0.0840 | 0.216074 | 0.223695 |
| `top1_abs_share` | 0.6179 | 0.6179 | -0.0863 | 0.180580 | 0.183986 |
| `abs_entropy` | 0.6178 | 0.3822 | +0.0844 | 0.987459 | 0.986619 |

The weak AUCs say there is some signal in magnitude concentration, but not
enough for a reliable deployable split.

## LODO Threshold Audit

Each fold selects the best single-feature threshold on the other 14 datasets
and applies it to the held-out dataset.

| Group | SelectedRate | Wins | Losses | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `geometry_only` | 0.911 | 154 | 170 | 2 | +0.000486 | +0.000027 | -0.000316 | -0.000258 | +0.000309 | -0.004074 |
| `geometry_plus_train_prior` | 0.698 | 119 | 119 | 1 | +0.000332 | +0.000073 | +0.000037 | -0.000309 | +0.000139 | -0.001623 |

Adding train-fold atom priors helps slightly, but not enough.  The LODO
selector still fails to improve over raw and still leaves negative ranking
metrics.

## Interpretation

M1265 confirms that `uniform_l1` should not become a query-time gate or blend
with the current observable feature family.

What is retained:

- uniform impact calibration is a real source of Recall/CUB improvement;
- selected-atom magnitude inequality is part of the score-geometry problem;
- train-fold priors provide a weak signal, but not a safe deployable split.

What is rejected:

- direct uniform/raw query-time thresholding;
- another classifier over the same source geometry features;
- treating M1263 macro gain as enough to promote `uniform_l1`.

## Decision

Stop the direct uniform/raw blend branch.

Keep `uniform_l1` only as:

- a training regularizer;
- a teacher for impact calibration;
- evidence that future objectives should penalize over-concentrated raw atom
  magnitudes while preserving rank-sensitive rows.

The next meaningful branch should not be another selector/gate over the same
features.  It should move the signal into source construction or objective
design, for example:

1. train/listwise-calibrate atom impacts using raw and uniform as two anchors;
2. include explicit rank-sensitive loss terms for rows like `scidocs`;
3. keep CUB/Recall expansion constraints from M1224/M1225;
4. run the same small-to-large gate: separability audit, smoke replay, full
   shared15 only if the first two are clean.
