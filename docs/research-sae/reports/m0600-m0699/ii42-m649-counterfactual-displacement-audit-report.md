# M649 / Counterfactual Displacement Audit Report

## Status

M649 finds a real positive signal.

It replays the fixed best M646 and M647 operating points, extracts observed
top100 entrants/exits, and tests whether current native pair features can
separate:

- safe replacement: under-ranked positive enters top100 while a non-positive
  exits,
- destructive replacement: non-positive enters top100 while a positive exits.

The answer is yes, with important caveats.  M646 replacement pairs show
`0.921` logistic cross-validation AUC, and M647 replacement pairs show `0.863`
AUC.  This is stronger than the scalar M648 protected-risk result and supports
one more targeted second-stage attempt.

Run output:

- `runs/m649_counterfactual_displacement_audit_v1/m649_audit.json`
- `runs/m649_counterfactual_displacement_audit_v1/m649_audit.md`

## Event Counts

| Run | promoted positive | displaced positive | promoted nonpositive | displaced nonpositive | missed under positive |
| --- | ---: | ---: | ---: | ---: | ---: |
| M646 promote | 13 | 7 | 597 | 603 | 3206 |
| M647 assignment | 27 | 23 | 1419 | 1423 | 3192 |

M646 is safe but narrow.  M647 moves more rows and finds more positives, but it
also displaces too many positives.

## Pair Separability

| Run | safe pairs | destructive pairs | neutral pairs | logistic CV AUC | best univariate |
| --- | ---: | ---: | ---: | ---: | --- |
| M646 promote | 19 | 6 | 1467 | 0.921 | M646 score delta, 0.649 |
| M647 assignment | 70 | 57 | 7021 | 0.863 | atom signed-dot delta, 0.664 |

The important observation is not the univariate feature result.  Single fields
are weak.  The positive signal appears in the combined pair feature vector:
candidate features, slot features, feature deltas, absolute deltas, and rank
margin.

This means M648 failed because it collapsed the problem into a scalar risk
penalty.  The useful signal is pair-local: this candidate may replace this
specific slot.

## Dataset Movement

M646 positive movement is concentrated and small:

| Dataset | promoted positive | displaced positive |
| --- | ---: | ---: |
| trec-covid | 6 | 5 |
| nfcorpus | 4 | 1 |
| cqadupstack | 1 | 0 |
| dbpedia-entity | 1 | 1 |
| fiqa | 1 | 0 |

M647 broadens movement:

| Dataset | promoted positive | displaced positive |
| --- | ---: | ---: |
| trec-covid | 15 | 9 |
| dbpedia-entity | 4 | 7 |
| nfcorpus | 4 | 2 |
| msmarco | 2 | 1 |
| scidocs | 2 | 2 |
| cqadupstack | 0 | 2 |

This explains the current bottleneck.  M647 has the broader search behavior we
want, but without a reliable counterfactual safety model it loses too much on
some rows.

## What This Proves

1. The M646/M647 second-stage family is not exhausted.
   M648 closed the scalar-risk path, but M649 shows pair-local replacement
   separability remains strong enough to test.

2. The next scorer must be candidate-slot pairwise, not row-score-only.
   Row scoring cannot express "replace this slot but not that slot."

3. The useful features are combined native features, not one obvious scalar.
   The best univariate AUCs are only `0.649` and `0.664`; combined CV AUC is
   much higher.

4. This is still not promotion evidence.
   Pair counts are small, especially for M646.  The next step must validate on
   replay metrics, not just pair AUC.

## Current Block

The block has narrowed:

- It is not candidate generation.
- It is not fixed alpha.
- It is not scalar protected-risk penalty.
- It is specifically counterfactual boundary replacement:
  deciding whether a candidate should replace a particular top100 boundary
  occupant.

## Next Step

Proceed to M650 candidate-slot swap scorer.

Design constraints:

- Train a global pairwise replacement classifier from safe/destructive
  replacement pairs plus sampled neutral negatives.
- Use candidate-slot pair features, not a scalar row score.
- Replay as constrained assignments, but with M646 as the safety floor.
- Gate against M646 promote, not only against P1.3 baseline.
- Require:
  - Recall@100 above M646 promote,
  - MAP@100 not below M646 promote,
  - no NDCG@10/MRR@20 regression,
  - no dataset Recall regression below `-0.001`,
  - positive gain share below `0.75`.

Stop condition:

If M650 cannot turn this pair separability into replay improvement over M646,
then the native second-stage boundary family should stop, and the next real
move should be retrieval-constrained generated-posting objective work.
