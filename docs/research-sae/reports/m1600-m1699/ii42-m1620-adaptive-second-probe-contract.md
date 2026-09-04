# M1620 Adaptive Second-Probe Contract

## Question

M1610B showed that its locked 8,192-key document source can contain the dense
tail, but a direct query router cannot select the required keys. M1620 asks one
finite question before abandoning that source:

> Does the first unified-posting probe expose enough query-conditioned corpus
> evidence to choose useful second-probe keys without qrels or dense scores?

This is not another router training run. It is an exact deterministic replay
over the locked M1610B initialization.

## Product Boundary

The tested policy uses:

- one encoder and the locked M1610B query/document key routes;
- one scalar unified posting namespace;
- seed query keys for a first probe capped at `0.03x` corpus reads;
- only first-hit document posting keys, impacts, rank, and global DF for the
  second probe;
- one final posting union under total caps of `0.075x` and `0.15x`.

All O@K values use a fixed dense-target denominator of K. An underfilled
candidate list is therefore penalized rather than evaluated only over the
documents it happened to return.

It does not use BM25, ANN, qrels, dense scores, dataset identity, or a second
index to generate deployable actions. Dense top256 is used only to construct
an oracle upper bound and to measure source overlap after the policy is locked.

## Fixed Policies

Only four deterministic policies are authorized:

1. reciprocal-rank evidence times document impact and IDF, using first top8;
2. reciprocal-rank evidence times document impact and IDF, using first top32;
3. normalized first-probe score times document impact and IDF, using top8;
4. normalized first-probe score times document impact and IDF, using top32.

Each policy may append at most 128 second-probe keys. Seed keys remain first in
the route, and the exact evaluator enforces the total posting-read cap.

## Gate

A mechanism signal requires, at the `0.15x` cap:

- O@100 and O@256 each improve by at least `0.05` over direct qTopK=2;
- each improvement closes at least 20% of the conditional-oracle gap;
- mean recall of the oracle second-probe action set at 128 actions is at least
  `0.15`;
- O@10 does not regress by more than `0.005`;
- the same policy is non-regressive at `0.075x`.

Dense-equivalence promotion still requires O@100 >= `0.95` and O@256 >=
`0.90`. A mechanism signal below that threshold may justify one learned
controller experiment. Without the mechanism signal, close adaptive probing
and do not tune evidence depth, action limits, weights, or classifiers.

## Historical Stop Evidence

M1230 already showed that static atom-document native context did not improve
target/harm separation. M1317-M1322 found some two-stage ranking value, but it
did not generalize into a row-safe candidate policy. M1620 is still distinct:
it tests dynamic document-to-key bridge actions on the new M1610B source. A
failure would align the old native-atom and new token-key surfaces and close
the shared observability premise.
