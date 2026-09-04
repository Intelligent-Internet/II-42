# M1710 Score-Decomposable Semantic Posting Contract

## Objective

M1691-M1701 close another loss-search family. Different teachers, objectives,
and schedules repeatedly improve a local retrieval surface by opening more
high-DF vocabulary terms, then lose either dense ordering or native cost.
M1710 does not train a text encoder. It asks the prior structural question
before any model is allowed to learn:

> Can a deterministic dense-to-posting compiler preserve dense top-k through
> one additive inverted score at a bounded posting cost?

The product boundary remains one encoder output, one physical inverted index,
and one query-time posting accumulator. Semantic code keys and lexical token
keys may occupy separate namespaces inside that index. ANN search, exact-dense
reranking, forward-vector residual scoring, qrels-selected routing, and a
second external index are not product candidates.

## Evidence Boundary

M1710 does not repeat closed mechanisms:

- M1520C already rejects an unsupervised token-state latent vocabulary.
- M1541/M1630 show that signed PCA scoring is expressive but requires nearly
  global posting traversal for safe top-k.
- M1570 rejects one composite product-cell posting because deployable cell
  ordering cannot expose its oracle capacity.
- M1571/M1572 reject random-hyperplane first-hit and multi-hit sources.
- M1600 proves that a balanced 4,096-key source contains dense neighbors at
  low oracle cost, but nearest-key union plus exact-dense reranking and a
  learned query router do not expose that capacity.
- M1640-M1701 establish the current vocabulary-sparse native control and its
  stable quality/cost Pareto conflict.

The missing experiment is narrower. M1600 treated code keys as binary
candidate admission, then used exact dense scores. M1710 instead makes the
code posting itself carry an additive approximation of the dense inner
product. This is a different scorer and can be falsified without training.

The mechanism is grounded in product/residual quantization and MIPS work:

- [Distill-VQ](https://arxiv.org/abs/2204.00185) distinguishes retrieval-order
  preservation from reconstruction-only quantization.
- [RepCONC](https://arxiv.org/abs/2110.05789) makes balanced code assignment a
  first-class constraint.
- [Anisotropic Vector Quantization](https://arxiv.org/abs/1908.10396) explains
  why inner-product error, not isotropic reconstruction error, is the relevant
  quantity.
- [Searching Dense Representations with Inverted Indexes](https://arxiv.org/abs/2312.01556)
  shows that dense scoring can be represented in an inverted index but warns
  that traversal can make it impractical.
- [ColBERTv2](https://arxiv.org/abs/2112.01488) and
  [PLAID](https://arxiv.org/abs/2205.09707) retain multi-vector structure,
  centroid pruning, and residual interaction. They motivate residual codes but
  are not evidence that a single-vector posting dot product is lossless.

## Frozen Surface

The first gate reuses the M1600A S2 qrels-free surface:

- dense root: `BAAI/bge-base-en-v1.5`;
- codebook fit cache: 10,000 MS MARCO rows and 88,992 unique documents;
- heldout cache: 1,000 queries and 8,988 disjoint documents;
- frozen M1600 initialization: 8 groups x 512 spherical keys;
- exact dense top256 already materialized before this experiment;
- no qrels, BM25, dataset identity, cross encoder, or retrieval labels.

The checkpoint, train cache, and heldout cache hashes are recorded before the
run. M1710 may not modify the dense vectors, M1600 rotation, or first-level
codebook.

## Posting Compilers

### First-Order Balanced Code Posting

For document group `g`, assign the nearest frozen unit codeword `c[g,k]` and
publish one posting with impact:

```text
a[d,g] = dot(d[g], c[g,k])
```

The query weight for the same key is `dot(q[g], c[g,k])`. A posting match
therefore contributes `a[d,g] * dot(q[g], c[g,k])`, an explicit rank-one
approximation to the group inner product.

### Residual Code Posting

Fit one deterministic 256-key Euclidean codebook per group to the residual:

```text
r[d,g] = d[g] - a[d,g] * c[g,k]
```

Each document publishes one additional residual key per group. Its query
weight is the query/residual-centroid dot product. The final score is the sum
of first-order and residual posting contributions. No forward vector is kept.

The fixed bounded policies are:

- first-order: 4, 8, and 16 query keys per group;
- residual: 2+2, 4+4, and 8+8 first/residual keys per group;
- ceiling: all keys, used only to measure representation capacity.

There is no probe grid after observing results. Residual fitting uses 20,000
training documents, 256 keys per group, 10 iterations, and seed 1710.

## Measurements

Every bounded row records:

- dense overlap at 10, 100, and 256;
- an exact-dense rerank upper over the same touched-document set;
- mean and p95 posting reads divided by corpus documents;
- touched-document ratio;
- maximum and p99 semantic-key DF;
- document postings per document;
- reconstruction cosine and full-code score ceiling.

An unquantized rotated-dense control must reproduce cached dense overlap at
10/100/256 at `>=0.999`. Failure is an evaluator-integrity stop and overrides
every representation result.

The exact-dense rerank upper is diagnostic only. It separates source admission
from posting-score approximation and cannot authorize a product run.

## Gates

A bounded row passes only when all conditions hold:

- direct posting-score O@100 `>= 0.95`;
- direct posting-score O@256 `>= 0.90`;
- exact-dense rerank upper meets the same floors;
- mean reads `<= 0.30N`;
- maximum semantic-key DF `<= 0.02`;
- every query returns at least 256 touched documents.

The residual representation ceiling must independently reach O@100 `>=0.98`
and O@256 `>=0.95`. A passing bounded row authorizes one unchanged official
FiQA replication. Selection is by the smallest mean read ratio, then O@256,
then O@100. Qrels cannot participate.

## Decisions And Stop Conditions

- If the full-code ceiling fails, stop: the code representation lacks dense
  capacity and no encoder training is authorized.
- If the ceiling passes but no bounded row passes, stop: capacity exists only
  behind excessive traversal, so the product access structure fails.
- If the bounded source upper passes but direct posting score fails, stop:
  the additive score decomposition is insufficient; do not attach a reranker.
- Only a conjunctive bounded pass may run official FiQA and a native index
  prototype.
- No code count, group count, seed, rotation, probe allocation, loss, or
  teacher sweep follows a failure.

The final report must synthesize M1520C, M1541, M1570-M1572, M1600, M1630,
M1640-M1701, and M1710 into one representation-capacity verdict. It must not
call an oracle, full-code scan, or exact-dense rerank a unified-posting product.
