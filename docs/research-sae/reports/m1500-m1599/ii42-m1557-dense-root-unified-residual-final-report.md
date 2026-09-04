# M1557 Dense-Root Unified Residual Final Report

## Executive Decision

The M1550-M1556 program found a real and reusable lexical-residual mechanism,
but it did **not** produce a unified posting encoder that replaces full-corpus
dense retrieval.

Keep these results:

- a frozen semantic head plus token-aware lexical head is the correct internal
  decomposition;
- dense ranks 1-99 must be protected structurally rather than by a soft loss;
- exact lexical terms in the same posting lifecycle add substantial candidate
  capacity and Recall;
- the deterministic residual/controller is much stronger and more stable than
  reconstructing lexical identity from a pooled dense vector.

Stop these routes:

- blind whole-ranking residual scoring;
- pooled-dense lexical reconstruction, deeper MLPs, or LoRA on the same input;
- unconditional or lexical-unique rank-100 replacement;
- qrels-labeled or threshold-tuned boundary guards;
- controlled joint tuning before semantic candidate access is dense-equivalent.

M1549 remains the recall product baseline: native ANN/dense access plus native
lexical postings under one auditable retrieval lifecycle.  The P1 residual
route remains useful as a compact native supplement/fallback, not as the
default dense replacement.

## Stage Results

| Stage | Question | Result | Decision |
| --- | --- | --- | --- |
| M1550 | Can one score reproduce M1549? | Context model keeps 87.6% of gain but harms the dense head | Require hard top99 controller |
| M1551 | Is protected tail selection learnable? | Deterministic lexical parity; learned context keeps 88.1% gain | Authorize frozen-head capacity probe |
| M1552 | Can pooled dense embeddings generate lexical residuals? | Held-out target 0.75%; only 0.33% gain retained | Stop pooled head/MLP/LoRA |
| M1553 | Does token-aware residual work in one P1 index? | Official9 Recall +0.0676, but arguana regresses | Keep architecture, reject default policy |
| M1554 | Does the mechanism transfer to giant corpora? | Giant3 Recall +0.0217, all rows positive | Promote residual mechanism evidence |
| M1555 | Does lexical-unique admission fix arguana? | Recall -0.00143, worse than unrestricted | Stop source restriction |
| M1556 | Is action harm observable qrels-free at query time? | AUC near 0.80, but 46-75% of harms survive; arguana fails | Stop learned guard line |

## Comparable Official12 Matrix

The following matrix uses the 12 official rows available in both the external
M1549 native evaluation and the P1 native/generation-shard evaluations.  It is
an equal-row macro, not a query-weighted aggregate.

| Variant | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| VectorChord dense | 0.499243 | 0.363149 | 0.657241 | 0.591910 | 0.722712 |
| M1549 dense + lexical | 0.499243 | 0.363455 | 0.669238 | 0.591910 | 0.754437 |
| M549U/P1 semantic posting | 0.367833 | 0.245101 | 0.485054 | 0.466554 | 0.525526 |
| M549U/P1 + protected lexical residual | 0.367833 | 0.245896 | 0.541173 | 0.466554 | 0.648028 |

The residual is materially useful relative to P1:

- Recall@100: `+0.056119`;
- CUB: `+0.122502`;
- MAP@100: `+0.000795`;
- NDCG@10 and MRR@20: unchanged by construction.

But the absolute first-stage gap remains decisive:

- Recall versus dense: `-0.116067`;
- CUB versus dense: `-0.074684`;
- NDCG versus dense: `-0.131410`;
- MRR versus dense: `-0.125356`;
- M549U native dense-overlap@100 on the same 12 rows: `0.372110`.

The high M549U score/overlap results on capped or full-scan surfaces therefore
do not imply dense-equivalent inverted-index candidate access.  This is the
same representation-versus-access distinction isolated by M1540-M1545.

## What The New Architecture Proved

The proposed two-head shape is useful, with one correction:

```text
text
  -> dense-root semantic compiler
  -> exact/token-aware lexical residual publisher
  -> one logical II42 lifecycle
  -> protected-tail policy
```

It is not necessary or beneficial to make the lexical head reconstruct exact
terms from the pooled semantic embedding.  The text already exposes those
terms exactly.  A deterministic tokenizer/impact publisher is smaller, more
auditable, and transfers better than the failed M1552 neural head.

The protected-tail controller is also not optional.  M1550 proves that a
whole-ranking score changes the protected dense head; M1555 proves that source
restriction alone does not make a forced swap safe; M1556 proves that the
remaining harm is not robustly observable across corpora.

## Why Joint Tuning Was Not Run

The predeclared controlled-joint gate required the frozen residual head to
retain at least 50% of M1549 gain per nontrivial held-out row and 80% in macro.
M1552 retained only 0.33% in macro and collapsed near random across held-out
corpora.  Unfreezing the root after that failure would add capacity around an
unmet information/access contract and would repeat the historical geometry
damage seen in M636 and later output-compiler work.

This is a stop condition, not an incomplete experiment or a claim that more
epochs were unavailable.  Training loss converged; transfer failed.

## Literature And Prior-Evidence Alignment

- SPLADE v2 supports retrieval-oriented distillation and exact lexical
  evidence, not reconstruction-only SAE loss.
- LED supports lexical supervision with explicit dense-rank consistency.
- sparse latent dense-retrieval work shows that reconstruction plus KL can
  improve fit without guaranteeing dense retrieval preservation.
- scaling and hard-negative work supports candidate-set structure, but does
  not provide missing candidate access from a pooled vector.
- M1540-M1545 already showed that the honest bottleneck is neighborhood access
  geometry, while exact scoring over a full union can be near dense only at
  prohibitive touch.

The new experiments agree with those results rather than overturning them.

## Product And Research Recommendation

1. Keep M1549 as the quality baseline and implement ANN plus lexical postings
   under one logical II42 index lifecycle/API if product unification is the
   requirement.
2. Keep the deterministic token-aware residual and top99 controller as a P1
   compact-path feature; do not call it dense-equivalent.
3. Do not reopen lexical residual training until a new semantic candidate
   source passes an oracle capacity gate at bounded touch on held-out full
   corpora.
4. If pure sparse replacement remains a research goal, the next probe must
   change candidate-access geometry, such as learned corpus routes or
   multi-probe block access.  It must first beat the M1542 capacity frontier;
   it should not begin with another loss, selector, or LoRA run.

## Final Status

**Route closed as a unified dense replacement; mechanism retained.**  The
program generated new information and a useful product component, but its own
hard gates reject deeper training and default promotion.
