# M814 Native-Outcome Stage Summary

## Scope

M812/M813 tested the next supervision level after M802-M811 stopped
threshold/guard-based generated-bundle selector repair.

The goal was to determine whether the blocker is:

1. generated-bundle proposal/data quality;
2. selector loss/supervision;
3. feature separability/model capacity.

## M812 Result

M812 trained a native-outcome selector:

- target: native replay outcome delta, query-local centered;
- model: HGB regressor or ridge regressor plus risk classifier;
- policy: choose the best bundle per query under a predicted risk cap;
- validation: leave-surface-out.

It did not find a deployable common policy.

The best common rows still failed strict clean:

| Model | Risk Lambda | All Clean | Min Utility | Mean Utility | Applied |
| --- | ---: | ---: | ---: | ---: | ---: |
| hgb | 1.00 | 0 | +0.000108 | +0.000189 | 408 |
| hgb | 0.50 | 0 | +0.000055 | +0.000133 | 214 |
| hgb | 0.25 | 0 | +0.000013 | +0.000172 | 200 |

Interpretation: outcome supervision makes the selector more aggressive, but
does not solve safety.  It often selects many rows and spreads risk over
multiple tasks.

## M813 Result

M813 tested the oracle ceiling of the same generated-bundle pool.

Safe-positive oracle is clean and useful across every surface and split:

| Surface | Split | Applied | Utility | dMAP | dNDCG | dMRR | dCUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| original | dev | 42 | +0.000620 | +0.000319 | +0.000217 | +0.000281 | +0.000054 |
| original | test | 41 | +0.001795 | +0.000382 | +0.001376 | +0.000107 | +0.000033 |
| seed7642 | dev | 36 | +0.000472 | +0.000233 | +0.000144 | +0.000186 | +0.000117 |
| seed7642 | test | 49 | +0.001143 | +0.000358 | +0.000714 | +0.000141 | +0.000087 |
| seed7643 | dev | 44 | +0.001530 | +0.000745 | +0.000689 | +0.000401 | +0.000032 |
| seed7643 | test | 40 | +0.000801 | +0.000340 | +0.000348 | +0.000066 | +0.000201 |

This is a strong oracle signal.  The generated-bundle pool contains enough
clean useful rows.  The failure is not lack of useful candidates.

## Diagnosis

The current problem is feature-to-label extraction, not proposal absence.

The existing row features are not sufficient for a global model to identify
the safe-positive subset at useful coverage.  When the model is conservative,
it finds small clean pockets.  When it is aggressive, it captures utility but
spreads risk across tasks.

This points to a supervision/feature issue:

- row-level `strict_positive` is too sparse for generic classifiers;
- native-outcome regression over-selects because positive utility and hidden
  risk are not well separated in the current features;
- query-local competition is necessary but still not sufficient without
  stronger features that explain why the oracle chose a row.

## Decision

Do not continue by tuning M812 thresholds, risk lambdas, or regressor type.

The next real breakthrough must make the safe-positive subset more observable.

## Next Candidate: M815

M815 should build an oracle-explanation feature table:

1. For every query, mark the M813 safe oracle winner.
2. Compare the winner against rejected generated bundles for the same query.
3. Add pairwise difference features:
   - score-family identity;
   - bundle dimension overlap;
   - source-score gap;
   - lexical coverage gain/loss gap;
   - scale/direction agreement;
   - whether rejected rows share boosted/demoted evidence with the winner.
4. Train a pairwise winner model rather than an absolute row classifier.
5. Replay through the same leave-surface-out native evaluator.

Acceptance:

- one common policy across held-out surfaces;
- strict clean;
- min utility above +0.00005;
- no dense overlap or CUB regression.

Stop if M815 cannot approach the M813 oracle with common clean behavior.
