# M801 Generated-Bundle Repair Summary

## Scope

This note summarizes the M798-M800 repair pass after M795/M796/M797.

The route under test is still the M794 generated query-local bundle pool.
The objective is to recover part of the M795 strict-positive oracle without
spending dense overlap or relying on dataset-specific tuning.

## What Changed

M796 used only bundle metadata and native score-displacement features.  It
failed to recover the M795 strict-positive oracle.

M798 added M784-style moved-document lexical interaction witnesses to the same
generated-bundle pool.  These features are query-time observable: they use the
query text, the documents moved inside top100 by the candidate bundle, and
corpus-local IDF/token overlap.  Qrels are used only as labels/evaluation.

## Positive Signal

M798 found strong transferable feature signal on generated bundles:

| Score family | Best cross-zero feature | Dev AUC | Test AUC |
| --- | --- | ---: | ---: |
| score_mean_margin | demoted_lex_coverage_mean | 0.661366 | 0.758906 |
| margin_bundle | boosted_lex_coverage_mean | 0.698135 | 0.769127 |
| conservative_margin | boosted_lex_coverage_mean | 0.661049 | 0.750247 |

This is materially stronger than the native-only selector surface.  The
direction is stable: strict-positive generated bundles tend to move documents
whose lexical coverage with the query is higher.

## Selector Result

M799 trained a bounded interaction-aware selector with a cross-zero safety
filter.  It improved macro ranking signal but did not pass the strict clean
gate:

| Best M799 row | Gate | Clean | Applied | dMAP | dNDCG | dCUB | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| conservative_margin + HGB | 1 | 0 | 34.3 | +0.000063 | +0.000139 | +0.000080 | +0.000000 | +0.000245 |

The repeated negative rows were concentrated in:

- webis-touche2020
- trec-covid
- nfcorpus
- climate-fever

This means interaction witnesses help rank candidates, but the selector still
admits harmful rows on specific retrieval surfaces.

## Threshold Oracle

M800 swept test thresholds over the fixed M799 selector scores.  The best
useful unsafe operating points were:

| Case | Gate | Clean | Applied | dMAP | dNDCG | dO@100 | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| conservative_margin + HGB best-any | 1 | 0 | 63.3 | +0.000147 | +0.000441 | +0.000000 | +0.000648 |
| margin_bundle + logistic best-any | 1 | 0 | 60.3 | +0.000063 | +0.000148 | +0.000000 | +0.000239 |

But the best clean operating points were only tiny:

| Case | Gate | Clean | Applied | dMAP | dNDCG | Utility |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| margin_bundle + HGB clean | 1 | 1 | 4.0 | +0.000010 | +0.000023 | +0.000046 |
| score_mean_margin + logistic clean | 1 | 1 | 1.3 | +0.000005 | +0.000000 | +0.000005 |

Therefore M799 did not fail mainly because of threshold calibration.  Its
score ordering is still too weak to recover the M795 oracle safely.

## Decision

Do not promote M799.

Do not keep tuning the same selector architecture.  M798 proved the new
interaction features are real, but M799/M800 proved that a generic global
selector with a single threshold is not enough.

## Next Repair

The next useful repair is not another classifier swap.  It should change the
safety objective/interface:

1. Persist a generated-bundle feature table so M798/M799/M800 do not repeatedly
   recompute interaction features.
2. Train with explicit harmful-row supervision, not only strict-positive labels:
   separate strict-positive, harmless-neutral, and task-negative rows.
3. Optimize worst-surface safety during threshold selection instead of only
   macro utility.
4. Add a per-query abstention margin: accept only if the top selected bundle is
   clearly separated from the next risky bundle.
5. Keep cross-zero as a hard safety set until a richer safety objective proves
   otherwise.

Stop condition for the next selector: if explicit harmful-row supervision still
cannot produce a clean operating point above M800's tiny clean utility, abandon
selector recovery for this proposal shape and move the interaction witnesses
into a different generated-posting objective.
