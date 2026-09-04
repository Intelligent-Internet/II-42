# M1287 Pair/Witness Selector Audit

M1287 tested whether source-document witness identity can recover the target
atoms lost by the aggregated M1286 source view.

This is not a deployable policy.  The pair/witness features use
dense-boundary positive/negative witness documents, so the result is a
teacher/source-construction signal that must pass native replay before it can
be promoted.

## Setup

- Surface: `arguana,cqadupstack,fiqa,scidocs`
- Limit: 25 queries per dataset
- Source: expanded candidate atoms from the M1286 path
- Models: shallow target/risk classifiers over raw, pair, and raw+pair
  features
- Output:
  `runs/m1287_pair_witness_selector_limit25_v1/m1287_pair_witness_selector.json`

## Model Separability

| Group | Rows | Features | TargetAUC | RiskAUC | TargetShare | RiskShare |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `raw` | 92160 | 24 | 0.827892 | 0.839195 | 0.024512 | 0.022222 |
| `pair` | 92160 | 15 | 0.997502 | 0.997306 | 0.024512 | 0.022222 |
| `raw_pair` | 92160 | 37 | 0.997647 | 0.997602 | 0.024512 | 0.022222 |

The pair/witness view is dramatically more separable than the raw candidate
view.  This matches the M1000+ diagnosis: useful atoms exist, but the current
aggregated source hides the target/harm structure.

## Eval Frontier

| Variant | Budget | TargetRecall | VisibleRecall | TargetVis | NegOnly | OracleOverlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_risk0_b384` | 384 | 0.983513 | 1.000000 | 0.983513 | 0.032943 | 0.398828 |
| `pair_risk0_b192` | 192 | 0.983513 | 1.000000 | 0.983513 | 0.041146 | 0.416146 |
| `pair_risk0.5_b384` | 384 | 0.981611 | 0.998066 | 0.983513 | 0.004167 | 0.378255 |
| `pair_risk1_b384` | 384 | 0.972099 | 0.988395 | 0.983513 | 0.001107 | 0.372656 |
| `abs_vote_b384` | 384 | 0.703234 | 0.715023 | 0.983513 | 0.057292 | 0.442448 |

The best pair/witness selector improves target recall by `+0.280279` over the
M1286 `abs_vote_b384` baseline while reducing negative-only selection.

## Interpretation

M1287 is a real source-level positive signal, not an engineering solution.
It says the missing information is witness/set structure, not another
post-hoc threshold on the old atom scores.

The next required gate is native replay:

1. Replay top pair/witness variants through the native P1 posting scorer.
2. Use candidate impact first, not oracle target impact.
3. Require pair success lift, protected-head overlap, and low regression.
4. If native replay fails, keep M1287 only as a teacher design signal.

This aligns with the broader lesson from M1224/M1225 and M1244/M1245:
future progress should change teacher/source construction so target/harm
separation is built into the source, not patched by another selector.
