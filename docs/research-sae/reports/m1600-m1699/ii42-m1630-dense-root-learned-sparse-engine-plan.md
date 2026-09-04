# M1630 Dense-Root Learned-Sparse Engine Plan

## Decision

M1630 changes the causal structure of the program. It does not train another
query-key router and does not approximate M1549 with a post-hoc selector.

The new hypothesis is:

> Preserve a wider dense-faithful learned-sparse score, then obtain efficiency
> from a retrieval engine designed for learned sparse vectors instead of
> forcing the encoder to predict a tiny access set before seeing the index.

This remains one encoder and one unified posting index. A forward payload used
to finalize sparse scores is part of the same index and is not an ANN field.

## Why This Is Not A Repeated Route

The project has tested both halves separately, but never combined them:

- M1541 proved that accumulated signed-coordinate postings plus a compact tail
  score can reproduce dense quality. Its failure was reading almost the whole
  coordinate index.
- M1544 tested approximate centroid summaries inside coordinate postings. It
  lost candidates and required about 12,000 dense summary scans per query.
- M1015 tested a different mechanism: membership-aware block64 upper bounds.
  It preserved exact top-k and reduced decoded/full to `0.320` on the high-
  fanout FiQA canary, with bitset word/full `0.176`.

M1630 combines M1541's stronger dense-root score surface with M1015's safe
engine. Unlike M1544, a safe upper bound cannot prune a true top-k block.

The literature supports this separation:

- Seismic (`arXiv:2404.18812`) organizes learned-sparse inverted lists into
  geometric blocks and finalizes selected documents from a forward index.
- Block-Max Pruning (`arXiv:2405.01117`) uses block upper bounds and a safe
  early-termination condition over a block-oriented sparse payload.
- Dynamic Superblock Pruning (`arXiv:2504.17045`) reduces bound-computation
  work before visiting child blocks.
- Li-LSR (`arXiv:2505.01452`) finds that relaxed sparsity improves both
  in-domain and out-of-domain quality and that modern sparse engines can handle
  the wider representations efficiently.

## Target Architecture

```text
text
  -> frozen dense-root backbone
  -> moderately sparse semantic impact head
  -> signed impacts split into non-negative posting channels
  -> one unified posting index
       term -> block64 membership + block max + compressed impacts
       doc  -> compact forward sparse payload
  -> membership-aware block/superblock upper bounds
  -> exact score finalization for opened blocks
```

For a signed dimension, same-sign query/document channels provide positive
upper-bound contributions. Cross-sign contributions can only reduce a score,
so they are omitted from the safe upper bound and included during exact forward
finalization. This preserves correctness without requiring non-negative dense
coordinates.

## Stage 0: Reconstruct The Proof Surface

1. Rebuild the frozen M1541 three-row canary from the committed script because
   the old remote run directory has been archived.
2. Export a strict sparse score surface and a tail-sketch diagnostic surface.
3. Adapt M1015 metadata and traversal to signed semantic postings.
4. Use fixed-K overlap and report underfilled rankings correctly.

No model training, qrels-based selection, lexical rescue, or approximation is
allowed in Stage 0.

### Stage 0 Gate

- exact block traversal matches full sparse top100 for every query;
- sparse representation reaches at least 95% of dense Recall/NDCG on at least
  two of NFCorpus, SciFact, and FiQA;
- all posting reads, block-bound operations, finalized documents, metadata
  bytes, and wall time are reported separately.

If representation quality fails before access, stop and move to Stage 2's
proper learned-sparse head only as a new model family. If exactness fails, fix
the engine before any quality experiment.

## Stage 1: Engine Feasibility

Run one fixed `block64` layout first. Promotion requires:

- exact top100 parity on every query;
- macro decoded/full <= `0.60` and no row above `0.85`;
- bitset word/full <= `0.30`;
- metadata <= 1.5x compressed posting bytes;
- wall time better than full sparse accumulation.

If bounds are too loose, one structural repair is allowed: qrels-free BP-like
document ordering plus two-level superblocks. Do not sweep block sizes,
thresholds, or approximate summary mass. If the repaired exact engine still
fails, close this representation/engine combination.

## Stage 2: Dense-Root Learned-Sparse Head

Only after the engine surface is trustworthy, train a proper overcomplete
sparse impact head. Do not constrain the query to 4-8 routing keys. Start with
roughly 64/128/256 query nonzeros and 128/256/512 document nonzeros as three
capacity points, not a hyperparameter grid.

Training is staged:

```text
L = L_dense_score_distillation
  + lambda_rank * L_topK_listwise
  + lambda_support * L_dense_support
  + lambda_df * L_document_frequency
  + lambda_bound * L_block_bound_slack
```

- freeze the dense backbone initially;
- use broad qrels-free dense teacher neighbourhoods and hard negatives;
- select checkpoints by fixed-K O@10/O@100/O@256 and exact-engine cost;
- only unfreeze the last backbone layers after two seeds pass;
- use BEIR qrels only after checkpoint and engine policy lock.

The block-bound term is a log-sum-exp surrogate for the gap between safe block
upper bounds and exact document maxima. It trains an index-friendly score
distribution, not a post-hoc query selector.

## Stage 3: Unified Retrieval Residual

Only after dense-faithful semantic retrieval passes broader rows, add lexical
or generated residual terms into the same posting namespace. Train document
expansion/impact first; the LSR literature and local M1553/M1554 evidence both
indicate that document-side expansion is more reliable than query-time gates.

There remains one physical index and one scalar score. No VectorChord or
external BM25 candidate union is permitted in this research branch.

## Stop Conditions

Stop M1630 when any of these holds:

- exact safe traversal must decode more than 85% on every canary row;
- metadata or forward-payload cost is comparable to adding a separate ANN
  index without providing dense-quality parity;
- wider sparse heads improve training loss but not fixed-K heldout overlap;
- cost only falls by returning underfilled candidate lists;
- gains require approximate centroid admission, qrels-time controls,
  per-dataset settings, or a second physical retrieval source;
- the next proposal is another router, threshold, block-size, or loss-weight
  sweep rather than a measured representation/bound repair.

## Expected Value And Risk

This route has a stronger mathematical basis than M1600-M1620 because safe
upper bounds preserve the score surface by construction. It directly removes
the query-action observability requirement.

The main risk is efficiency, not correctness: dense-faithful sparse impacts
may produce loose bounds and force most blocks open. Stage 0/1 answer that
question cheaply before a new training program begins. If they fail, the
strict single-index dense-replacement objective has a much stronger final stop
signal than another failed encoder experiment.

## Execution Goal

Build and evaluate an exact membership-aware block64 engine over the frozen
M1541/P1 dense-root posting surface. Establish exact top100 parity, fixed-K
dense overlap, decoded/full, bitset work, metadata cost, and wall time on the
three canary rows. Authorize learned-sparse head training only when the engine
passes the predeclared cost gate; otherwise close the strict route without
further router or selector experiments.
