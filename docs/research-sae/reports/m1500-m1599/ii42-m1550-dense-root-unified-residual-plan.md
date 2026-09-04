# M1550 Dense-Root Unified Residual Route

## Decision boundary

M1549 proves that lexical evidence can add recall without damaging the dense
head when the runtime preserves dense ranks 1-99 and admits one lexical result
at rank 100.  It does **not** prove that a single query-document score can
represent that rank-conditioned policy.

M1550 therefore starts with a representability audit, not another training
run.  The route advances only when the preceding gate passes.

## Architecture under test

```text
text
  -> frozen dense-root backbone
       -> semantic signed-posting compiler
       -> lexical residual posting head
  -> one sparse posting map
  -> one II-42 native index
```

The semantic and lexical namespaces may be separate inside the posting map,
but they share one checkpoint, one publisher contract, and one native index.
The protected-tail controller remains an allowed internal index policy because
M1549's top99 rule depends on corpus-level rank context.

## Evidence behind the design

- M549U establishes a dense-root signed-posting warm start with near-dense
  overlap when the official PPLX root is used.
- M636 shows that unconstrained output-compiler training can lower the fitting
  loss while reducing O@100 and Recall.  Loss improvement alone is not a gate.
- M1146-M1148 show that useful boundary atoms exist, but static query-side
  features recover too little of the held-out target to justify promotion.
- M1549 establishes a qrels-free, row-safe teacher on official BEIR15:
  Recall@100 `+0.013873`, CUB `+0.036921`, MAP@100 `+0.000371`, and no
  NDCG@10 or MRR@20 regression.

The staged design is also consistent with:

- [SPLADE v2](https://arxiv.org/abs/2109.10086): sparse retrieval benefits
  from distillation and retrieval-oriented training rather than reconstruction
  alone.
- [LED](https://arxiv.org/abs/2208.13661): lexical supervision should be
  constrained by rank consistency with the dense teacher.
- [Interpreting Dense Retrieval with Sparse Latent
  Features](https://arxiv.org/abs/2411.00786): reconstruction plus
  retrieval-oriented KL is materially better than reconstruction alone, but
  sparse conversion still does not automatically preserve dense retrieval.
- [Scaling Sparse and Dense Retrieval](https://arxiv.org/abs/2502.15526):
  deeper knowledge distillation without contrastive structure is not a
  reliable scaling strategy.
- [Beyond Hard Negatives](https://arxiv.org/abs/2604.04734): candidate score
  strata matter; top-K imitation alone is an incomplete teacher.

## Stages and gates

### M1550A: teacher reproduction

Rebuild the M1549 teacher from native VectorChord and II-42 BM25 results.  The
teacher uses no qrels.  Qrels are loaded only after rankings are frozen for
evaluation.

Gate:

- exact dense top256 plus at most 16 unique lexical candidates;
- dense top99 preserved exactly;
- the selected lexical boundary document matches M1549;
- reproduced canary metrics agree with the recorded M1549 surface.

### M1550B: score representability

Compare three fixed families under leave-one-dataset-out validation:

1. `local_linear`: one weighted semantic plus lexical score;
2. `local_nonlinear`: a fixed nonlinear scorer with only pair-local scores;
3. `context_nonlinear`: the same scorer with rank and boundary context.

Teacher imitation, not qrels, selects parameters.  Report dense-head
preservation, boundary-target recovery, teacher top100 overlap, retrieval gain
retention, and row harms.

Gate for neural residual training:

- held-out dense-head preservation at least `0.99`;
- held-out boundary-target recovery at least `0.50`;
- at least `0.50` of M1549 Recall gain retained;
- no held-out NDCG@10 or MRR@20 harm.

If only the context model passes, the protected-tail controller is a required
part of the unified index contract.  A blind one-score encoder is rejected.

### M1551: frozen residual head

Only after M1550B passes, freeze the dense root and semantic compiler.  Train
the lexical residual head from M1549 candidate admission and boundary ordering:

```text
L = L_added_candidate
  + lambda_boundary * L_boundary_listwise
  + lambda_head * L_head_inversion
  + lambda_sparse * L_sparsity_fanout
```

`L_head_inversion` is a hard margin against moving any dense top99 document
below a lexical candidate.  It is not replaced by a soft KL average.

Scale in order: 30-query smoke, three full canaries, cross-dataset heldout,
then shared15/native replay.  Track training in ClearML when GPU training
starts.

### M1552: controlled joint tuning

Only if M1551 passes the frozen-head gate, unfreeze the final dense layer or a
small LoRA adapter at low learning rate.  Reject checkpoints unless all dense
equivalence and retrieval gates improve over the frozen-head checkpoint.

## Final acceptance and stop rules

Promote only when the native unified path retains at least `0.80` of M1549's
Recall gain, has no macro or dataset-row NDCG/MRR harm, and keeps the dense
overlap floors.

Stop neuralizing the protected-tail policy when:

- gain retention remains below `0.50` on held-out datasets;
- pair-local models fail while rank-context models pass;
- deeper training lowers loss without improving the hard gates;
- improvement depends on qrels, dataset identity, or dataset-specific tuning.

In that case, retain M549U semantic postings plus a deterministic lexical head
and protected-tail controller inside the single II-42 index.
