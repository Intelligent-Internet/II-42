# M1556 Boundary Action Observability Report

## Question

M1553 and M1554 show that a token-aware lexical tail action is strongly
macro-positive, while M1555 shows that unconditional rank-100 replacement is
not row safe.  Before training another selector, M1556 tests whether gain and
harm actions can be separated from features available at native query time.

The feature view contains only query atoms/text statistics, semantic boundary
scores and margins, lexical scores/ranks, source overlap, and selected versus
displaced score geometry.  Dataset identity and qrels are excluded.  Qrels-
derived Recall/MAP deltas create diagnostic gain/harm labels only; no model
from this audit is deployable.

The active-action smoke includes every observed harm and at most 200 gains per
dataset from the official9 M1553 run.  Validation is leave-one-dataset-out.

## Result

| Model | Rows | OOF AUC | OOF AP | Gains accepted | Harms accepted |
| --- | ---: | ---: | ---: | ---: | ---: |
| Balanced linear | 586 | 0.7956 | 0.2521 | 89.96% | 46.43% |
| Shallow nonlinear | 586 | 0.8043 | 0.1716 | 96.95% | 75.00% |

The global AUC is superficially positive, but it does not satisfy the action
gate.  The safe operating point must retain at least half the gains, accept no
more than 25% of harms, and reject all harmful actions when arguana is held
out.  Neither model does so.

Critical held-out failures include:

- arguana: linear AUC `0.50`, one of two harms accepted, and its only gain
  rejected;
- arguana: nonlinear AUC `0.00`, both harms accepted;
- cqadupstack: both models accept all five harms;
- NFCorpus: linear accepts 50% and nonlinear accepts 87.5% of harms;
- TREC-COVID: nonlinear accepts every harm.

## Interpretation

The feature family contains enough aggregate correlation to rank some action
risk, but not enough calibrated, cross-corpus information to enforce row
safety.  Higher model capacity worsens harm retention.  This reproduces the
earlier M727/M1245 observability boundary rather than opening a new selector
line: useful actions exist, but their relevance outcome is not reliably
visible before retrieval evaluation.

The failure is especially decisive for the proposed M1549-to-P1 teacher
transfer.  M1549's top99 policy is safe relative to the true dense ranking.
P1/M549U has different full-corpus candidate geometry, so the same action can
be harmful, and available boundary features do not identify that transfer
risk reliably.

## Decision

**Stop boundary-guard training.**  Do not run the full-gain M1556 expansion,
deepen the classifier, tune its threshold, or train a qrels-labeled product
guard.  Keep the deterministic lexical residual as mechanism/capacity
evidence, but not as a row-safe P1 default.

The next architecture decision must use the absolute dense comparison:
retain native ANN/dense access as the frozen semantic substrate if full-
corpus dense equivalence is required.  A pure P1 posting path may remain a
compact fallback or candidate supplement, but this experiment does not
support replacing ANN with it.
