# M1280 Objective Source Construction Audit

## Question

M1279 showed that post-hoc support fill restores CUB but still leaves NDCG
harm.  M1280 tests whether existing positive signals can be composed into a
cleaner source before any native replay:

- CUB-specific target/harm labels from M1224
- query-time signed/source deltas from M1251
- M1277 clean-compiler probabilities

This is an observability audit only.  If the composed source does not beat
`source_abs`, it should not be replayed.

## Result

Hard-row smoke was run with top8, top3, and top1.  Top8 was saturated because
the candidate universe averages only about 4.7 atoms per query, so all source
variants selected the same atoms.

The decision-grade top3 run showed the same issue:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `source_abs_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `signed_positive_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `model_prob_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `model_x_source_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `model_x_signed_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `model_x_signed_source_top3` | 0.797235 | 0.252555 | 0.010219 | 0.242336 | 2.819 |
| `model_x_prior_signed_top3` | 0.788018 | 0.249635 | 0.010219 | 0.239416 | 2.819 |

## Interpretation

M1280 is a useful negative result.  Directly composing the retained positive
signals does not create a new source.  Most variants collapse to exactly the
same ordering as `source_abs`; the one variant that differs is worse.

This confirms that the bottleneck is not a missing multiplicative scoring
formula over current features.  The current source is too narrow or too
homogeneous for this kind of composition to expose new target/harm separation.

## Decision

Do not replay M1280.

Do not continue with more direct `model_prob * source * signed` variants.  The
next objective must change the label/source interface, not just combine the
existing scalar signals.

## Artifacts

- Script: `scripts/audit_m1280_objective_source_construction.py`
- Top3 JSON: `runs/m1280_objective_source_construction_top3_smoke_v1/m1280_objective_source_construction.json`
- Top3 Markdown: `runs/m1280_objective_source_construction_top3_smoke_v1/m1280_objective_source_construction.md`
- Top1 JSON: `runs/m1280_objective_source_construction_top1_smoke_v1/m1280_objective_source_construction.json`
- Top8 JSON: `runs/m1280_objective_source_construction_smoke_v1/m1280_objective_source_construction.json`
