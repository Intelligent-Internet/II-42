# M1542 Corpus-Route Posting Capacity Contract

## Motivation

M1541 proved that pooled-BGE signed coordinates and a 256-dimensional INT8
tail sketch can reproduce dense ranking after a good 8% candidate set exists.
It also proved that selecting posting entries by individual coordinate impact
cannot produce that set: dense neighbors require accumulated evidence from
many moderate coordinates.

M1542 changes only candidate access. It tests document-only spherical route
atoms, analogous to a coarse inverted-file or clustered sparse index, before
authorizing any learned compiler.

## Frozen Components

- frozen `BAAI/bge-base-en-v1.5` document and query embeddings;
- M1541 document-only PCA active coordinates and tail256 INT8 scorer;
- exact M1520 NFCorpus, SciFact, and FiQA canaries;
- no BM25 fusion and no trainable output head;
- qrels used only after all rankings and costs are fixed.

## Route Construction

- fit spherical k-means on document embeddings only;
- use the global rule `ceil(document_count / 16)` centroids;
- use deterministic seed `1542` and at most 20 Lloyd iterations;
- publish either one route atom or the top two route atoms per document;
- sort each route posting list by document-to-centroid affinity;
- score query route atoms by query-to-centroid affinity;
- multi-probe lists in route-score order until the exact candidate union
  reaches 8% or 15% of the corpus;
- count every posting entry read, including duplicate entries in the dual
  assignment surface.

The codebook, assignments, probe order, and budget are qrels-free. Corpus
adaptation is allowed because the route codebook is an index structure, not a
dataset-specific ranking policy.

## Required Rows

- exact frozen-BGE dense;
- reference BM25;
- single-assignment route score, exact-dense upper, and tail256 INT8;
- dual-assignment route score, exact-dense upper, and tail256 INT8;
- both 8% and 15% exact-union budgets.

Every row reports quality, candidate upper bound, dense overlap, route posting
reads, raw union, rerank fraction, probes, and logical bytes per document.

## Predeclared Gate

For each assignment count, its 15% tail row passes a dataset only when:

1. Recall@100 is at least 95% of exact BGE;
2. NDCG@10 is at least 95% of exact BGE;
3. candidate upper bound is at least 95% of exact BGE;
4. exact-dense candidate upper overlap@100 is at least 0.90;
5. raw candidate union is at most 0.151;
6. route reads are at most 0.151 for single assignment or 0.301 for dual.

A route is scalable only with at least two of three dataset passes and no row
below 90% of exact BGE Recall@100 or NDCG@10. Single assignment has priority;
dual assignment is considered only if single assignment fails its global gate.

## Decisions

- **Single pass:** proceed to one native unified lexical+semantic index test.
- **Dual-only pass:** retain dual routes and account for the extra route-edge
  storage/read cost before native integration.
- **Admission pass, tail fail:** permit one deterministic sketch-capacity
  check; no output-head training.
- **Both routes fail admission:** stop pooled-BGE posting routing. Use a proper
  ANN/vector index for dense geometry rather than disguising a full scan as a
  posting route.

