# M1723 Joint Residual Source Contract

## Why This Stage Exists

M1722A separates source capacity from observability. The frozen M1600 source
offers a residual teacher O@100/O@256 of `1.000000/0.989352` at max DF
`0.012228`, but residual key AUC and recall are lower than the dense control.
Training a larger rank-128 router for 1,500 updates captures less than `0.34%`
of the teacher gap and peaks at update 100.

Therefore the residual target is not missing capacity; it is misaligned with
the frozen document partition. M1723 changes that partition. It does not add
router depth or another post-hoc gate.

## Causal Comparison

M1723 initializes two models from the same frozen M1600 8x512 codebook and
jointly updates the geometry adapter and codebook:

- `dense_control`: candidate target is frozen dense top96;
- `residual`: candidate target is the first 96 dense documents absent from
  frozen BM25 top256.

The candidate rows, random negatives, initialization, architecture, optimizer,
temperature schedule, cost losses, update count, validation surface, and
checkpoint rule are identical. Only target probability and positive masks
differ.

## Runtime-Matched Forward Shape

- one semantic key per document/group;
- eight semantic query probes per group;
- eight groups x 512 keys = one 4,096-key semantic namespace;
- straight-through hard assignments in the forward pass;
- soft assignments supply gradients;
- BM25 terms and semantic keys are unioned as one candidate source;
- exact dense scoring is used only as the source-admission upper bound.

This fixes a train/runtime mismatch in the original M1600 joint run, whose
training forward used one query assignment per group while the useful runtime
policy probed multiple keys.

## Objective

The joint objective contains:

- listwise target probability over dense or residual candidate documents;
- positive versus hard-negative pairwise pressure;
- document-key balance and explicit max-DF penalty;
- codebook decorrelation;
- geometry and codebook anchors to the dense-root initialization.

No qrel, explicit positive label, cross-encoder score, dataset identity, or
per-query policy enters training or selection.

## Canary And Gate

The canary uses the M1600 4,000/500 pools, rank-independent codebook training,
2,000 updates, batch 16, 128 candidates, 96 teacher documents, fixed eight
query probes/group, learning rate `1e-4`, and seed 1723.

Scale to 10,000/1,000 only if the residual checkpoint:

- is a trained checkpoint;
- improves unified O@100 by `>=0.01` and O@256 by `>=0.005`;
- improves residual recovery at 100 by `>=0.01` and at 256 by `>=0.005`;
- keeps semantic reads `<=0.18x` and max DF `<=0.02`;
- exceeds the equal-capacity dense control variant score by `>=0.002`.

The full stage uses 4,000 updates and requires a second seed before native or
qrel evaluation. If hard metrics peak early and then fall, the early selected
checkpoint is retained but the schedule is not extended.

## Stop Rules

- Do not sweep query probes, key count, loss weights, temperature, or learning
  rate after a failed canary.
- Do not treat exact-dense source replay as a deployable scorer.
- If both control and residual collapse, the asymmetric ST source objective is
  closed.
- If the dense control passes but residual does not beat it, lexical residual
  supervision does not justify further source specialization.
- If residual passes source admission, direct additive score training is the
  next separate gate; BM25 alpha or reranking cannot hide a failed source.
