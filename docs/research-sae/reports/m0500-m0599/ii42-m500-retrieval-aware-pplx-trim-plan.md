# II-42 M500 Retrieval-Aware PPLX Trim Plan

## Goal

M500 starts a new route: instead of training a small encoder from scratch to
imitate PPLX, test whether PPLX itself can be compressed with retrieval-aware
signals.

The working hypothesis is:

- M396 proved that dense-derived posting structure is expressive enough.
- M411-M417 show that small students still struggle to learn the PPLX semantic
  space.
- Therefore, compressing or trimming PPLX may be a better route than asking a
  much smaller random student to rediscover that space.

## Stage A: Non-Training Trim Probe

The first step must not interfere with current training. It is a read-only
probe:

- load PPLX teacher on an idle machine;
- sample documents and queries from materialized retrieval tasks;
- collect official SentenceTransformer embeddings;
- collect hidden-layer pooled embeddings from the same PPLX backbone;
- compare each layer/pool candidate against materialized dense teacher and
  against the deterministic posting target.

This is a proxy for layer trimming: if an intermediate layer already preserves
dense retrieval and posting geometry, later work can try actual truncation.

## Metrics

For each candidate representation:

- dense cosine and MSE versus materialized PPLX dense vectors;
- score Pearson versus materialized dense scores on sampled query-doc pairs;
- TopK overlap against materialized dense retrieval on sampled candidate docs;
- deterministic posting coordinate cosine/MSE;
- active posting Jaccard;
- tail sketch cosine.

## Promotion Rule

Promote to actual structured trimming only if an intermediate representation
keeps most of the teacher geometry:

- document/query dense cosine stays high;
- Top100 overlap is close to the full teacher;
- posting active Jaccard does not collapse;
- query side does not regress more than document side.

If every intermediate layer collapses, move to retrieval-aware distillation or
QAT from full PPLX initialization rather than blind layer truncation.

## Resource Rule

Do not use `spark-2` while the existing Scale-RAE job is active. Prefer ASA if
reachable; otherwise use `spark-1` only when M417 is idle or complete.
