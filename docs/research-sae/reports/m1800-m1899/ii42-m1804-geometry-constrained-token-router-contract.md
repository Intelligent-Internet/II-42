# M1804 Geometry-Constrained Token Router Contract

## Trigger

M1803's unconstrained low-rank correction lowers training loss while heldout
target recall and retrieval collapse monotonically. The correction acts
directly in 8,192-key logit space, so it can memorize training key identities
without preserving contextual-token geometry.

M1804 tests one structural repair, not another weight sweep:

```text
query token state
    + zero-initialized rank-8 hidden residual
    -> frozen normalized key codebook
    -> query route
```

## Objective

Only keys already in the frozen base top16 compete with the witness target.
The loss is:

- target-versus-hardest-local-negative softplus margin;
- KL preservation over the frozen base top64 distribution;
- hidden-state residual norm.

This changes the trainable space and the loss support together. It cannot
create an arbitrary key-specific output table.

## Frozen Surface

- dense backbone, token states, document router, document keys, index, and
  64-dimensional int8 payload remain frozen;
- rank 8 hidden adapter, 128 maximum steps;
- fixed evaluations at steps `1,2,4,8,16,32,64,128`;
- qTopK 4, dTopK 2, 2.0x primary retrieval policy;
- no qrels or dataset-specific selection.

## Gate And Stop

The M1803 retrieval gate is unchanged. In addition, train/validation target
recall is reported at every checkpoint. If no checkpoint passes, stop this raw
BGE/frozen-document-router family. Do not continue with ranks, seeds, learning
rates, margins, or longer schedules. A later token-routed product attempt must
start from a genuinely late-interaction-trained checkpoint or jointly retrain
query and document token routing on a substantially broader corpus.
