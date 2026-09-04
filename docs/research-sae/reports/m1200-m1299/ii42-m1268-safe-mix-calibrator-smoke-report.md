# M1268 Safe Mix Calibrator Smoke

## Question

M1267 showed strong qrels-oracle capacity in the raw/uniform interpolation
anchor family.  The safe oracle improves full `shared15` score from raw
`+0.030620` to `+0.039580`, with zero losses against raw.

M1268 asks whether current qrels-free source-geometry features can recover that
safe oracle on hard rows using a small deployability smoke:

- train a simple multinomial logistic classifier;
- target is the M1267 safe oracle mix;
- validate leave-one-dataset-out on hard rows;
- replay predicted mix choices through the same precomputed native metrics.

## Run

- Script: `scripts/train_m1268_safe_mix_calibrator_smoke.py`
- Output root: `runs/m1268_safe_mix_calibrator_smoke_v1/`
- JSON:
  `runs/m1268_safe_mix_calibrator_smoke_v1/m1268_safe_mix_calibrator_smoke.json`
- Markdown:
  `runs/m1268_safe_mix_calibrator_smoke_v1/m1268_safe_mix_calibrator_smoke.md`
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`

## Label Distribution

Safe-oracle labels:

| Mix | Count |
| --- | ---: |
| `m1266_mix0` | 207 |
| `m1266_mix0p25` | 6 |
| `m1266_mix0p5` | 6 |
| `m1266_mix0p75` | 17 |
| `m1266_mix1` | 13 |

Only `42 / 249` hard-row queries should switch away from raw.

## Result

| Policy | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score | MeanScoreVsRaw | WinsVsRaw | LossesVsRaw |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `raw` | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 | +0.000000 | 0 | 0 |
| `oracle_safe_mix` | 0 | +0.001640 | +0.001209 | +0.000472 | +0.001013 | +0.000031 | +0.014825 | +0.005947 | 42 | 0 |
| `model_lodo_safe_mix` | 1 | +0.001133 | +0.000733 | -0.000116 | +0.000678 | +0.000031 | +0.007860 | -0.002307 | 17 | 19 |

The learned model is better than raw macro score but fails the deployability
gate:

- it still has one negative macro metric (`NDCG@10`);
- it loses mean query-level score against raw;
- it creates `19` losses against raw;
- it over-selects nonraw classes.

## Fold Diagnostics

| Dataset | Accuracy | NonRawLabel | NonRawPred | QueryCount |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | 0.330 | 12 | 66 | 100 |
| `scidocs` | 0.600 | 22 | 31 | 100 |
| `webis-touche2020` | 0.469 | 8 | 22 | 49 |

The main failure is false-positive nonraw selection, especially on
`cqadupstack`.

## Interpretation

M1268 is a useful stop signal.

What remains true:

- M1267 safe oracle proves nonraw impact calibration can help;
- the raw/uniform anchor family has real capacity;
- rank-sensitive impact calibration is a legitimate objective.

What M1268 rejects:

- current source-geometry features are not enough to safely choose mix class;
- a simple balanced logistic classifier is not deployable;
- scaling this exact feature/model family to full `shared15` is not justified.

## Decision

Do not run full `shared15` for M1268.

Stop this feature-family training branch.  The next route must change what is
observable or how the source is constructed.  Reasonable next questions:

1. add document/rank-context observability for the candidate documents that move
   under each mix, not only selected-atom geometry;
2. train impact calibration on pair/listwise rank movement directly, with
   explicit false-positive nonraw penalties;
3. use M1267 safe oracle as teacher, but only after introducing features that
   explain why `cqadupstack` false positives are harmful.

Until then, keep raw as the direct policy and keep raw/uniform/safe-oracle as
objective-design evidence, not a deployable scorer.
