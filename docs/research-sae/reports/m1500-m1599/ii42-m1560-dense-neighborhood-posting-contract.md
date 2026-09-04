# M1560 Dense-Neighborhood Posting Contract

## Objective

M1560 starts a new route whose product target is one text encoder, one unified
posting map, and one II42 inverted index.  Dense and lexical engines may be
used offline as teachers, but neither VectorChord nor a second BM25 retrieval
path is allowed in the final query path.

The first question is candidate access, not score fitting:

> Can a bounded corpus-global posting vocabulary preserve the dense
> neighborhood graph well enough to recover dense top100/top256 candidates?

No trainable text head is authorized until this oracle-capacity question is
answered.

## Prior Boundary

M1542 is the frozen baseline.  At a 15% unique candidate union, dual spherical
route atoms reached macro dense O@100 `0.712067`, Recall@100 `0.723467`, and
NDCG@10 `0.650569`.  It was a structural improvement over independent dense
coordinates, but it failed dense equivalence.

M1560 must not repeat centroid-count, budget, route-count, or loss sweeps.  It
changes route construction using the information M1542 discarded: the dense
document-neighborhood graph.

## M1560A Construction

Use the exact M1542 canaries, BGE root, seed, spherical k-means cells, 15%
candidate budget, and two posting edges per document.

Compare two equal-fanout document posting sources:

1. `centroid_dual`: each document publishes its two nearest centroid atoms;
2. `graph_dual`: each document publishes its primary centroid plus the
   non-primary cell receiving the largest cosine-weighted mass from its exact
   dense top32 document neighbors.

The second assignment is therefore chosen to reduce dense-neighborhood edge
cuts, not to increase local centroid fit.  Posting lists remain ordinary
inverted lists.  Every duplicate read is counted.

## Query-Side Decomposition

Evaluate each document posting source with two query policies:

1. `centroid_query`: order routes by query-to-centroid cosine; this is the
   deterministic deployable baseline used by M1542;
2. `dense_neighbor_oracle`: greedily select route lists that cover the frozen
   dense top100 with the fewest newly touched documents.  This is qrels-free
   but not deployable; it measures whether the document posting codebook has
   enough capacity independently of query routing.

This decomposition produces a precise next decision:

- oracle fails: document route vocabulary/capacity is wrong;
- oracle passes and centroid query fails: train a text-to-route compiler;
- both pass: move directly to unified semantic plus lexical postings.

## Frozen Costs And Metrics

- corpora: NFCorpus, SciFact, FiQA shared canaries;
- candidate union: at most 15.1% of documents;
- document route fanout: exactly 2;
- posting reads: at most 30.1% of corpus size;
- dense teacher depths: top100 and top256;
- qrels: evaluation only, after route construction and rankings are frozen;
- scoring upper bound: exact dense rerank over touched documents;
- required reporting: O@10/O@100/O@256, CUB, Recall@100, NDCG@10,
  MAP@100, MRR@20, union, reads, probes, edge coverage, and storage.

The M1542 centroid-dual row must reproduce the frozen M1542 summary before any
new result is interpreted.

## Gates

### Codebook Capacity

`graph_dual + dense_neighbor_oracle` passes only when:

- macro O@100 is at least `0.95`;
- every row O@100 is at least `0.90`;
- macro O@256 is at least `0.90`;
- union is at most `0.151`;
- reads are at most `0.301`.

### Deterministic Query Routing

`graph_dual + centroid_query` passes only when:

- macro O@100 is at least `0.90`;
- every row O@100 is at least `0.85`;
- every row Recall@100 and NDCG@10 is at least 95% of exact dense;
- the same union/read gates pass.

### One Authorized Capacity Extension

Graph fanout four is authorized only if graph-dual oracle O@100 is at least
`0.90`, improves centroid-dual oracle by at least `0.03`, but remains below
the `0.95` codebook gate.  This is a predeclared structural extension, not a
fanout grid.

## Stop Conditions

Stop this source without training when:

- M1542 parity fails;
- graph-dual oracle O@100 is below `0.90`;
- graph construction improves neither dense edge coverage nor oracle O@100;
- gains require exceeding the fixed union/read budget;
- only qrels metrics improve while dense-neighborhood coverage does not.

Do not add BM25, M1549 tail labels, a scorer, LoRA, or a text compiler before
the codebook-capacity gate permits it.

## Later Stages

If M1560A/B passes:

1. M1561 trains a qrels-free text-to-route compiler against dense-neighborhood
   route labels with corpus-heldout validation.
2. M1562 adds M1549 lexical-residual atoms into the same posting map and tests
   protected boundary behavior.
3. M1563 publishes the unified map through the existing II42 model lifecycle
   and runs heldout native index matrices and cost benchmarks.
