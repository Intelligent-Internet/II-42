# SAE M42 Utility-Aware Payload Allocation Plan

Status: completed; see `sae-m42-utility-aware-payload-allocation-results-report.md`.

## Summary

M41 proved that M40's higher SAE posting cost is trainable, but scalar fanout
regularization only creates a tradeoff frontier. M42 tests a more structural
idea:

```text
atom utility = ranking contribution / posting fanout
```

The goal is not another global SAE weight sweep. The goal is to decide which
query atoms deserve to enter the physical payload before the sparse engine
touches postings.

## Questions

M42 answers three questions:

1. Can a ranking-aware utility loss improve the query atom distribution?
2. Can utility/fanout-aware export selection keep M40 quality while reducing
   postings?
3. Is the next best path more training, or a better payload allocation policy?

## Workstreams

### M42.1 Training Utility Target

Add an opt-in training target to the M31 trainer:

```text
--atom-utility-loss-weight
--atom-utility-top-dims
--atom-utility-fanout-power
--atom-utility-qrel-mix
--atom-utility-teacher-prior
--atom-utility-restrict-teacher-dims
```

For each training example, compute a candidate-set utility target:

```text
teacher/qrel doc target
-> centered candidate contribution per SAE atom
-> posting-fanout discount
-> sparse support distribution target
```

The first safe variant restricts utility targets to teacher query atoms. This
prevents the direct text encoder from chasing doc-only atoms that are not
predictable from query text.

### M42.2 Export-Time Utility/Fanout Selection

Add a standalone sweep:

```text
scripts/research_sae_m42_atom_utility_prune_sweep.py
```

This reads existing query latents and simulates runtime payload allocation:

```text
selected_score = query_atom_weight / df(atom)^alpha
```

It sweeps:

- exported active dimensions;
- fanout discount power;
- the existing lexical-DF low/high SAE gate.

This isolates whether M42 needs new training or whether M40 already contains
enough useful atoms and only needs a better payload selector.

## Acceptance Criteria

M42 is positive if any point keeps M40-level NDCG/MAP while reducing SAE
postings materially.

M42 is especially strong if it can beat M40's `NDCG + MAP` score while lowering
posting cost. It does not need to solve the dense-removal product gate by
itself.

Reject further utility-loss training if it improves ranking only by increasing
high-fanout teacher atoms.
