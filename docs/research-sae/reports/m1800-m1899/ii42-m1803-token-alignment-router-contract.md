# M1803 Frozen Token-Alignment Router Contract

## Authorization

M1802 passes every predeclared observability gate. Weak-pair witness keys have
median query-router rank 2, recall@4 0.754, recall@16 0.927, and recall@64
0.988. Emitting only those witness keys reaches positive MRR@100 0.885 at
0.928 document-equivalent reads.

This authorizes one query-only training canary. It does not authorize changing
the dense backbone, document routes, token payload, index, or qrels evaluation.

## Model

The frozen M1610 8,192-key router supplies base logits. M1803 adds a
zero-initialized rank-16 correction:

```text
query token state
    -> frozen base key logits
    + low-rank query-only logit correction
    -> query routing keys
```

Document token keys, codebook, BGE states, and 64-dimensional int8 payloads are
immutable. At initialization the model is exactly the M1802 router.

## Supervision

For each query token, find the maximum-cosine token in its disjoint weak-pair
positive document. The witness document token's two frozen keys form a
multi-label target. The loss contains:

- multi-target softmax NLL;
- a target-versus-hardest-nontarget margin;
- L2 control on the low-rank correction.

No BEIR qrels, dataset identity, pooled-dense target ranks, or native outcome is
used for training.

## Fixed Canary

- train: M1610 1,000-row token cache;
- validation: disjoint 250-row token cache;
- rank: 16;
- steps: 400;
- batch: 256 witness tokens;
- evaluations: initialization and every 50 steps;
- primary retrieval policy: qTopK 4, dTopK 2, 2.0x decoded-entry cap;
- secondary diagnostics: qTopK 1/2 and 1.0x cap.

## Gate

A checkpoint is eligible only if, versus initialization on heldout data:

- target-key recall@4 improves by at least `0.05`;
- positive MRR@100 improves by at least `0.03`;
- exhaustive-MaxSim candidate-upper O@100 improves by at least `0.05`;
- direct routed O@100 against exhaustive MaxSim does not decrease;
- direct score retains at least `0.75` of candidate upper;
- document-equivalent reads remain at most `2.0x`.

The selected checkpoint maximizes the minimum normalized gate improvement, not
training loss. If no checkpoint passes, initialization remains selected and
the local witness-training hypothesis stops.
