# M1600 Retrieval-Native Balanced Discrete Backbone Contract

## Decision Boundary

M1600 is a new research family. It does not continue the M1560-M1572
post-hoc conversion branch and it does not claim that a pure posting index has
already replaced dense access.

The tested hypothesis is narrower:

> A dense-root representation whose final geometry is jointly optimized for
> balanced discrete posting access can preserve more of the dense top-K
> neighborhood at a fixed posting-read budget than a frozen dense geometry
> followed by routing, term selection, product cells, or hashing.

The first gate is source capacity. BM25, qrels, learned reranking, and
dataset-specific tuning are forbidden until that gate passes.

## Evidence Behind The New Mechanism

The local evidence chain fixes the problem definition:

- M1541 showed that an INT8 tail codec can rank accurately after candidate
  admission. Candidate access, not score reconstruction, is the bottleneck.
- M1565 route2048 is the strongest bounded deterministic FiQA source:
  O@100 0.906235, O@256 0.842683, and mean reads 0.045015x.
- M1566-M1569 showed that exact-term capacity exists but compressing it either
  removes useful common-term edges or keeps prohibitive posting cost.
- M1570-M1572 showed that frozen product cells and overlapping hashes cannot
  make useful dense-neighbor ordering observable at practical read cost.

The literature supports three changes, not a generic loss sweep:

1. Distill-VQ (`arXiv:2204.00185`) finds that retrieval-order distillation
   over a sufficiently broad teacher Top-K is more effective than embedding
   reconstruction. It also requires both high-ranked and broad negatives.
2. DF-FLOPS (`arXiv:2505.15070`) shows that ordinary sparsity does not control
   posting-list length; document-frequency pressure must be present in the
   objective.
3. Retrieval-oriented discrete-index work (`arXiv:2105.03933`,
   `arXiv:2110.05789`) jointly optimizes the representation and its discrete
   index rather than treating quantization as an independent post-process.

M1600 therefore uses dense-neighborhood listwise distillation, hard posting
budgets, explicit key balance and max-DF pressure, and a trainable shared
geometry transform.

## M1600A Representation

The first implementation is a balanced multi-code posting source:

```text
dense-root vector
    -> shared low-rank residual geometry transform
    -> fixed orthogonal group decomposition
    -> learned balanced codebook in each group
    -> one posting key per document per group
    -> bounded highest-confidence keys per query
    -> one unified posting-map union
```

All `(group, key)` values occupy one posting namespace. There is no ANN side
index and no external lexical index. The grouped structure is an internal
parameterization of one inverted posting map.

M1600A trains on precomputed dense-root vectors. This isolates whether the new
posting geometry has source capacity. It is not yet a text-model promotion.
Only a passing M1600A checkpoint may be attached to the dense encoder and
followed by last-layer LoRA.

## Training Objective

For each qrels-free query and dense-teacher candidate set:

```text
L = L_listwise_dense_order
  + lambda_collision * L_topK_collision
  + lambda_rank * L_teacher_margin
  + lambda_balance * L_key_balance
  + lambda_df * L_max_df
  + lambda_decorrelation * L_codebook_decorrelation
  + lambda_adapter * L_geometry_anchor
```

The forward surface uses straight-through hard document assignments and hard
bounded query probes. Soft assignments carry gradients. Reconstruction MSE or
KL alone may be logged but cannot select a checkpoint.

Training data is corpus-independent dense supervision. The initial source is
the existing 10,000/1,000 MS MARCO teacher-row corpus, but its cross-encoder
scores and labels are ignored. Dense-root Top-K and broad negatives are
recomputed from the text pool. BEIR qrels are read only after checkpoint lock.

## Predeclared Stages

### M1600A-S0: Integrity And Synthetic Gate

- exact posting-union evaluator;
- deterministic initialization and checkpoint replay;
- hard document and query budgets are enforced;
- collapsed keys, non-finite scores, or train/eval surface mismatch stop the
  run.

### M1600A-S1: Qrels-Free Heldout Source Gate

Train on MS MARCO train rows and select on disjoint MS MARCO validation rows.
Selection requires improvement in both dense O@100 and O@256 at a fixed read
budget. Lower training loss without hard-overlap improvement is rejected.

The fixed-source query-router repair may scale from the 4,000/500 canary to
the predeclared 10,000/1,000 surface only after two independent seeds improve
both overlap metrics. Cross-corpus work is authorized only when the unchanged
full-scale run also satisfies all of the following:

- delta O@100 is at least `+0.01`;
- delta O@256 is at least `+0.005`;
- O@10 does not decrease by more than `0.001`;
- a trained checkpoint, rather than initialization, is selected.

A smaller replicated gain is evidence that query-key ordering is learnable,
but not evidence that the mechanism can close the product-quality gap.

### M1600A-S2: Official FiQA Matched-Cost Gate

After locking the checkpoint, run full FiQA without using qrels for policy or
budget selection. Compare against M1565 route2048.

The minimum continuation gate is:

- O@100 > 0.906235;
- O@256 > 0.842683;
- mean posting reads <= 0.075x for the matched-cost row;
- max DF <= 0.05;
- no query-specific or dataset-specific tuning.

The desired dense-equivalence gate remains:

- O@100 >= 0.95;
- O@256 >= 0.90;
- mean posting reads <= 0.30x.

### M1600B: Cross-Corpus Gate

Only after M1600A-S2 passes, evaluate NFCorpus and SciFact with the same
checkpoint and fixed query policy. No BEIR row may choose its own budget.
Native Recall, CUB, NDCG, MAP, and MRR are diagnostics after source lock, not
checkpoint-selection inputs.

### M1600C: Encoder Integration

Only after M1600B passes, attach the head to the dense-root text encoder and
test low-rate last-layer LoRA. The native output remains one posting map. BM25
or retrieval expansion remains out of scope until dense-faithful posting
access is established.

## Stop Rules

Stop a branch immediately when any of the following holds:

- the qrels-free source oracle cannot beat M1565 at comparable reads;
- loss, score KL, or pair accuracy improves while hard O@100/O@256 does not;
- the only passing checkpoint is initialization or an untrained fallback;
- key balance is achieved only by destroying dense overlap;
- gains require qrels, dataset identity, per-row thresholds, or a post-hoc
  ANN/dense guard;
- a second seed or cross-corpus row removes the gain;
- the same representation is only being retried with a new scalar weight.

One structural repair is allowed after a failed M1600A run, and it must be
supported by measured failure anatomy. Ordinary threshold, loss-weight, or
depth grids are not authorized.

## Required Outputs

- exact configuration, environment, seed, artifact hashes, and ClearML task;
- qrels-free training and validation curves;
- key DF histogram, max DF, posting reads, raw candidate ratio;
- dense O@10/O@100/O@256 and score-order diagnostics;
- native retrieval metrics only after checkpoint lock;
- explicit comparison with M1565 and the M1560-M1572 final frontier;
- a final go/no-go report for the pure single-encoder, single-posting-index
  product constraint.
