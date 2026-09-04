# M1811 CITADEL Learned-Routing Contract

## Purpose

M1810 proves that static spherical routing can expose more than 90% of the
ColBERT top-100 candidates inside the cost envelope, but cannot preserve the
late-interaction score. M1811 tests the only materially different routing
mechanism justified by that result: a router jointly trained with token
retrieval.

The control uses the official CITADEL checkpoint at SHA-256
`23b7e4f7e355d0d5d26c885b55b04974b831364f7aa4098d5c0760e42c94e268`.
The checkpoint and upstream code are research-only CC-BY-NC artifacts. They
are evidence for the representation, not a product dependency.

## Product Boundary

The tested engine remains one physical inverted index:

```text
learned lexical key -> (document id, 32d token payload, route weight)
```

Query scoring opens only matching learned keys and applies max-per-query-token
followed by sum. No ANN, BM25 candidate source, qrels, dataset identity, or
post-hoc reranker is allowed.

CITADEL's optional CLS dense score is reported only as a diagnostic control.
It is excluded from every authorization gate because it requires corpus-wide
dense access.

## Attribution Ladder

1. Unrestricted 32-dimensional token MaxSim tests the representation.
2. Exhaustive learned-key scoring tests the trained routing function.
3. Budgeted key-to-payload replay tests the single-index implementation.

Document route depths `1,2,5`, pruning thresholds `0,0.5,0.9`, query route
depths `1,2`, and bounded read budgets are evaluated without parameter fitting.

## Gate

At least one int8-32 policy inside `1 MiB/query` and `10 KiB/document` must
satisfy all of:

- unrestricted MaxSim positive MRR@100 at least `0.70`;
- exhaustive learned route O@100 against MaxSim at least `0.75`;
- exhaustive learned-route positive MRR no more than `0.05` below MaxSim;
- candidate-upper O@100 against the exhaustive learned route at least `0.90`;
- direct budgeted O@100 retains at least `0.90` of candidate upper;
- direct positive MRR is within `0.03` of the exhaustive learned route.

## Stop Rule

If representation or exhaustive learned routing fails, stop importing this
model family. If those pass but budgeted replay fails, the remaining gap is an
engine/cost boundary, not another output-head training problem. Only a passing
token-only policy can authorize a small native or BEIR replay.
