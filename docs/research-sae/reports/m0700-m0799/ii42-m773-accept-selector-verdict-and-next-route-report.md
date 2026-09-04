# M773 Accept Selector Verdict and Next Route

## Decision

The current M758 proposal pool still contains useful safe-positive mass, but
the current feature set cannot train a robust global accept selector.

This means:

- do not rebuild the proposal generator yet;
- do not continue threshold-only rank-trace tuning;
- do not simply make the selector model larger;
- next work should add failure-aware native feedback features and task-row
  damage witnesses.

## Evidence

M771 measured the oracle ceiling inside the same M758 proposal pool:

| Policy | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m768_guard | 1 | 1 | 6.0 | +0.000005 | +0.000000 | +0.000000 | +0.000005 | +0.000000 |
| strict_positive_oracle | 1 | 1 | 26.0 | +0.000261 | +0.000227 | +0.000123 | +0.000016 | +0.000000 |
| utility_positive_oracle | 1 | 1 | 29.0 | +0.000259 | +0.000227 | +0.000123 | +0.000100 | +0.000000 |

The ceiling is real: a correct accept/fallback selector would be around 50x
larger than the M768 robust guard on dMAP.

M772 trained global selectors on dev surfaces and replayed one fixed threshold
on test:

| Model | Label | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| hgb | query_utility_positive | 1 | 0 | 6.0 | +0.000028 | +0.000073 | +0.000000 | +0.000014 | +0.000000 |
| hgb | query_strict_positive | 0 | 0 | 10.3 | +0.000034 | +0.000062 | +0.000000 | +0.000011 | +0.000000 |
| logistic | query_strict_positive | 0 | 0 | 36.0 | +0.000028 | +0.000068 | +0.000000 | +0.000011 | +0.000000 |
| logistic | query_utility_positive | 0 | 0 | 4.0 | -0.000005 | +0.000059 | +0.000000 | +0.000000 | +0.000000 |

Best M772 candidate:

- model: HGB;
- label: query_utility_positive;
- macro gate passes;
- clean fails because `trec-covid` regresses on original and seed7642;
- dMAP +0.000028 and dNDCG +0.000073, still far below M771 oracle.

## Interpretation

The route has not hit a proposal ceiling.  It has hit an accept-selector
feature ceiling.

M770 showed:

- test selected rows: 680;
- strict-positive rows: 78;
- utility-positive rows: 87;
- M768 robust guard rows: 18;
- M768 and strict-positive intersection: 5;
- dense-overlap spend: 0.

So the current proposal pool contains enough positive rows, but robust
rank-trace stability mostly selects near-no-op rows.

M772 shows that generic context + rank-trace features recover weak positive
signal, but cannot identify task-local damage.  The recurring failed tasks
are `trec-covid`, `climate-fever`, `scidocs`, and `webis-touche2020`.

## What Not To Do

Do not run another:

- single-feature threshold search;
- unconstrained larger selector;
- per-dataset tuned selector;
- proposal rebuild before explaining why positive rows are missed.

Those would repeat M757/M760/M764/M768/M772 failure modes.

## Next Route: M774 Failure-Aware Native Feedback

Goal: add features that explain task-local damage before training another
selector.

Required audit:

1. For each M772 selected row, record task-level metric deltas, not only macro
   deltas.
2. Compare selected rows that cause negative tasks against oracle-positive rows
   in the same task.
3. Add qrels-free native damage witnesses:
   - tail displacement concentration;
   - relevant candidate boundary margin proxy;
   - top-k score entropy before and after delta;
   - number of documents crossing top10/top20/top50/top100;
   - reciprocal rank movement of baseline high-score documents;
   - per-query score distribution skew/kurtosis after delta.
4. Re-run a selector only if these features separate:
   - strict-positive vs task-damaging rows;
   - trec-covid/climate-fever failures vs safe rows.

Acceptance for the next selector:

- macro gate passes;
- no negative tasks across all three test surfaces;
- dMAP and dNDCG exceed M768 by at least 5x;
- dO@100 remains zero;
- applied queries remain above diagnostic scale.

Stop condition:

If failure-aware native feedback still cannot separate task-damaging rows, stop
the accept-selector route and move to a new proposal generator.
