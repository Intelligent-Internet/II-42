# M1630 Exact Signed Block Engine Report

## Decision

**Close signed PCA postings plus global exact block64 traversal.**

The score representation is useful and the engine is exact, but the safe
membership-aware bounds cannot skip any document block.  M1630B training is
not authorized because it would optimize a representation against an engine
surface that already failed its predeclared cost gate.

## Fixed Surface

- frozen `BAAI/bge-base-en-v1.5`;
- document-only PCA rotation;
- 128 active signed document coordinates;
- exact same-sign posting upper bounds with cross-sign contributions deferred
  to exact forward scoring;
- one 64-bit membership word per term/block;
- qrels used only after rankings were fixed;
- public literature reviewed through the 2026-07-10 arXiv cutoff.

## FiQA Canary

| Query shape | Document order | NDCG@10 | Recall@100 | O@100 | Parity | Decoded/full | Word/full | Blocks opened |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| full rotated query | source | 0.686645 | 0.920044 | 0.903800 | 1.000000 | 1.000000 | 0.103530 | 32/32 |
| full rotated query | balanced tree | 0.686645 | 0.920044 | 0.903800 | 1.000000 | 1.000000 | 0.102786 | 32/32 |
| top128 query | source | 0.679056 | 0.918794 | 0.859800 | 1.000000 | 1.000000 | 0.072632 | 32/32 |
| top128 query | balanced tree | 0.679056 | 0.918794 | 0.859800 | 1.000000 | 1.000000 | 0.071584 | 32/32 |

Exact BGE gives NDCG@10 `0.694897` and Recall@100 `0.934806` on the
same 2,000-document, 100-query surface.  Both sparse query shapes pass the
95% representation gate.  Every engine row fails the cost gate.

## Mechanism

The failure is not an implementation or ranking error:

1. Exact top100 parity is 100/100 for every run.
2. The negative-score and final-partial-block cases are covered by focused
   tests.
3. Query top128 reduces active terms and bitset work, but not block opening.
4. Deterministic qrels-free active-vector ordering changes metadata locality
   slightly but still opens all blocks.

Dense-root signed coordinates distribute a useful score over many moderate
same-sign contributions.  Each 64-document block retains at least one lane
whose safe sum remains above the top100 threshold.  A global block maximum is
therefore too loose even after membership masking.

This differs from M1015.  Its lexical plus learned-sparse surface had about 91
active query terms, selective non-negative postings, and heterogeneous impact
weights.  On full 57,638-document FiQA it skipped 72.5% of blocks.  M1630
shows that the M1015 engine result does not transfer automatically to signed
PCA dense coordinates.

## Literature Boundary

- [Block-Max Pruning](https://arxiv.org/abs/2405.01117) supports safe block
  termination but does not guarantee tight bounds for an arbitrary weight
  distribution.
- [Seismic](https://arxiv.org/abs/2404.18812) relies on geometrically cohesive
  per-list blocks and approximate summaries, not one global document order.
- [Wacky Weights](https://arxiv.org/abs/2110.11540) predicts that learned
  sparse score distributions can weaken standard dynamic pruning.
- [Approximate Cluster-Based Sparse Retrieval](https://arxiv.org/abs/2404.08896)
  supports clustering only with representation-aware segmented bounds; M1544
  already rejected approximate coordinate admission on this source.

No paper justifies relaxing exactness or adding another ordering/grid after
the measured failure.

## Next Route

M1640 starts from an established tokenizer-vocabulary learned-sparse model
rather than another latent/PCA projection.  Its first step is an external
SPLADE-v3 control with the same exact block engine.  Training is allowed only
if that known sparse output shape demonstrates both retrieval signal and a
materially better bound/cost distribution.

## Artifacts

- local implementation:
  `scripts/research_sae_m1630_exact_signed_block_engine.py`;
- remote root:
  `/home/huoju/leask/runs/ii42-m1630-exact-sparse-engine-v1`;
- ClearML tasks:
  `608d9ff9d594473488a4733900a0d57a`,
  `eeb0961fc5e64a77973aed0c2d861104`;
- focused tests: 8 passed with the M1541 compatibility tests included.
