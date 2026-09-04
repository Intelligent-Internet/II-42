# M1722-M1726 Residual-Native Training Final Report

## Final Conclusion

The dense-minus-BM25 residual is a real retrieval signal, but the tested
joint-source formulation cannot turn it into a net full-scale improvement.
The route is **stopped**, not merely paused for more training.

The experiments separate five hypotheses that had previously been mixed:

| Experiment | Causal question | Result |
| --- | --- | --- |
| M1722 | Is a better teacher enough over a frozen source? | No; oracle improves, observability worsens. |
| M1723 | Can source and query compatibility move jointly? | Yes on 4k, but the quality optimum exceeds read cost. |
| M1724 | Is minibatch query-cost control sufficient? | No; it underestimates corpus posting load. |
| M1725 | Does exact train-corpus DF repair cost? | Partly; a cost-safe canary remains below O@100 gate. |
| M1726 | Is the remaining failure caused by small data? | No; load gap shrinks, but all trained full-scale checkpoints lose to initialization. |

## What Was Learned

### 1. Residual supervision is causally useful

M1722's residual teacher improves source-oracle O@256 from `0.971445` to
`0.989352`. M1723-M1726 repeatedly show that a residual arm is better than an
equal-capacity dense-control arm. At M1726 step 400 the matched advantage is
`+0.009630/+0.015797` O@100/O@256.

This validates the product intuition: semantic postings should focus on dense
evidence that lexical retrieval does not already express well.

### 2. Teacher quality is not query-time observability

With the M1600 source frozen, residual teacher-key AUC falls from `0.730598`
to `0.721456`, and selected-key recall falls from `0.139077` to `0.111170`.
The corpus-aware action is better, but it is harder to infer from the original
query/source geometry.

### 3. Joint movement creates a quality/cost frontier

On the 4k canary, joint training produces genuine hard-metric gains. M1725's
quality peak gains O@100/O@256 by `+0.027000/+0.051438`, but costs `0.209341x`
reads. Its cost-safe checkpoint keeps useful O@256 and recovery gains but
misses the fixed O@100 gate.

### 4. More data fixes load estimation, not representation interference

M1726 reduces the train-validation read gap from `0.037376x` to `0.013314x`.
Nevertheless, the frozen full initialization is stronger than every trained
checkpoint. The residual loss makes joint updates less destructive than the
dense loss, but does not preserve the initialized semantic geometry.

### 5. Training depth was tested rather than assumed

M1722 runs 1,500 updates, M1723 runs 2,000, and M1724-M1726 evaluate repeated
checkpoints through 1,200-1,600 updates. Hard retrieval peaks early and then
regresses. The failure is not evidence that an otherwise improving run was
stopped before convergence.

## Retained Artifact And Product Boundary

Retain:

- frozen M1600 full source as the current semantic substrate;
- dense-minus-BM25 residual teacher construction;
- exact full-corpus DF and query-selected read accounting;
- equal-capacity dense controls and epoch-zero selection;
- the `0.18x` read and `0.02` max-DF limits.

Do not retain:

- replacement/joint movement of the base source under the current objective;
- the claim that canary gains authorize full training;
- more loss-weight, depth, probe, seed, or threshold search;
- exact dense reranking as a deployable scorer.

## Only Justified Continuation

The next representation test must preserve the base path by construction:

```text
frozen lexical postings
  + frozen M1600 semantic postings
  + budgeted residual semantic namespace
  -> one physical posting map and accumulator
```

The residual namespace receives at most the unused budget between the frozen
base (`0.146086x` on M1726 validation) and the hard cap (`0.18x`). Its first
experiment is a deterministic oracle, not training:

1. freeze every base document key, query key, and score;
2. assign only base-missed dense-minus-BM25 documents to residual keys;
3. cap incremental reads at `0.0339x` and max residual DF explicitly;
4. measure capacity upper bound and cosine/query observability separately;
5. train a residual head only if both pass on disjoint full-scale pools.

This continuation differs structurally from M1723-M1726: failure in the new
path cannot damage the existing source. If the deterministic residual oracle
cannot reach at least `+0.01` O@100 and `+0.005` O@256 within the incremental
budget, stop additive residual training before creating a model.

## Status

M1722-M1726 is complete. It produced a clean negative product result and a
positive causal result about lexical-residual supervision. No checkpoint is
promoted and no native/qrel benchmark is warranted for this family.
