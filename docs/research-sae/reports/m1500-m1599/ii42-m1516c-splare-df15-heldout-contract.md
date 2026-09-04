# M1516C SPLARE DF15 Heldout Contract

## Locked candidate

M1516B identified one post-hoc candidate on NFCorpus, SciFact, and FiQA:

- document K=128;
- query K=32;
- corpus DF cap=0.15;
- no added IDF factor.

M1516C freezes that configuration before encoding a new evaluation surface.
No cap, budget, weight, or fallback may change after seeing the heldout row.

## New surface

Encode the complete official ArguAna corpus and test queries with the unchanged
M1510 independent SPLARE reproduction. The model and original D400/Q40 output
are created without ArguAna qrels. Qrels are read only by final evaluation.

Compare exactly two configurations: original D400/Q40 and the locked
DF15/D128/Q32 candidate.

## Gate

The candidate authorizes one bounded M1517 objective canary only if ArguAna:

- retains at least 90% of baseline NDCG@10, MAP@100, Recall@100, and MRR@20;
- retains at least 90% of candidate upper bound;
- has mean touched-document ratio at most 0.50;
- has empty-query rate at most 1%.

Failure stops DF-budget source construction. Passing does not promote the
runtime method; it only establishes a four-dataset capacity shape worth
internalizing in a training objective.
