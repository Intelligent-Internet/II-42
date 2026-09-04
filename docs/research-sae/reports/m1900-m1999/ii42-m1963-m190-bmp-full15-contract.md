# M1963 Raw M190 Exact-BMP Full15 Contract

## Purpose

M1963 completes the large-corpus quality decision for the only product-shaped
historical self-trained candidate: raw M190 learned impacts in one inverted
index. It follows two completed proofs:

1. M1960 broad9 verified the frozen checkpoint through the current PostgreSQL
   native posting path and exposed real row-level quality losses.
2. M1962 verified exact patched-BMP score/boundary parity, quantized quality,
   1,200 bytes/document, and 7.203 ms FiQA p95 without changing M190 support.

Publishing the remaining 2.6–5.4M-document corpora as billions of PostgreSQL
rows would primarily remeasure the known executor overhead on the local Mac.
M1963 instead performs the large evaluation on spark-1 through the validated
exact BMP implementation. This is a native inverted-index evaluation, not an
offline dense scan.

## Frozen Representation

- checkpoint SHA-256:
  `49dfd1e8de9cbbe270c1f98793157101f1892cf3dd32d10033ead66d47e2360b`;
- source trunk: `perplexity-ai/pplx-embed-v1-0.6B`;
- raw normalized learned impacts, no IDF replacement;
- document TopK-64, query TopK-80, 16,384 atoms;
- no BM25 fusion, gate, dataset-specific parameter, pruning, or retraining;
- official qrels used only for final quality metrics.

The five immediately available expansion rows are `nq`, `dbpedia-entity`,
`hotpotqa`, `fever`, and `climate-fever`. MSMARCO enters only after the
recovered historical M150 PPLX artifact passes the separate local and remote
identity contract. Cross-hardware rematerialization remains a fallback.

The final matrix requires a M1963 exact-BMP summary for every BEIR15 row.
After the large expansion rows complete, the eight remaining broad9 rows are
rerun through the same quantized engine; FiQA is already complete through
M1962/M1963. M1960 PostgreSQL results remain a cross-engine diagnostic and do
not fill missing exact-BMP rows in the promotion matrix.

MSMARCO uses a one-shot dependency handoff rather than a polling/restart loop.
The primary path transfers a checksum-bound zstd archive to an independent
recovered root. `scripts/run_m1960_msmarco_recovered_export_spark.sh` then
recomputes the 66 GB document SHA, all identity gates, and M190 TopK64/TopK80
surface before the exact-BMP runner starts. Schema, configuration, output
sizes, and every output SHA-256 must match. Upstream failure or a partial
surface blocks the replay. `scripts/run_m1963_msmarco_handoff_spark.sh`
remains the raw-text rematerialization fallback.

The MSMARCO handoff also requires at least 104 GiB of `MemAvailable` before
starting exact BMP. This is a systems safety gate, not a quality threshold:
the 5.42M-document FEVER and Climate-FEVER rows peaked near 59 GiB, which
linearly projects the 8.84M-document MSMARCO row near 96 GiB before headroom.
If spark-2 is below the gate because another high-priority job owns unified
memory, the replay must fail closed or move only the verified M190 surface to
an idle node. It must not stop the other job or attempt an OOM-prone build.

## Evaluation Design

All official queries are executed twice by BMP:

- top-100 for timed product latency;
- top-1000 for NDCG@10, MAP@100, Recall@100, MRR@20, and CUB@1000.

The exact M1962 binding has already been audited on every FiQA query. M1963
adds a fixed, evenly spaced 16-query sample per dataset for:

- exhaustive u32 score and exact top-100 boundary parity;
- float-M190 versus u8/u32 Recall retention.

Sampling is a quantization/engine diagnostic only. Every query still
contributes to reported retrieval quality and latency.

## Mathematical And Literature Basis

Raw M190 has the same additive impact form used by learned sparse retrieval:

```math
s(q,d)=\sum_{a\in\operatorname{supp}(q)\cap\operatorname{supp}(d)}
q_a d_a.
```

[DeepImpact](https://arxiv.org/abs/2104.12016) and
[uniCOIL](https://arxiv.org/abs/2106.14807) establish contextual learned
impacts as legitimate inverted-index weights. Their evidence is why M1963
preserves M190 magnitudes instead of replacing them with lexical IDF.

Before pruning, the exhaustive posting work of a query is proportional to:

```math
W(q)=\sum_{a\in\operatorname{supp}(q)}df(a).
```

Average document nnz alone cannot bound this quantity: a small number of
high-DF atoms can dominate every query. This is the failure mode isolated by
[DF-FLOPS](https://arxiv.org/abs/2505.15070), and it is why M1960 records full
corpus DF while M1963 measures actual index bytes and query latency.

M1963 then quantizes the frozen document impacts to u8 and accumulates u32
scores. It does not assert that quantization preserves all float rankings.
Instead it measures float/u8 Recall retention on a deterministic sample and
proves that patched BMP returns the exact top-k of the complete quantized
score surface. [Block-max learned-sparse retrieval](https://arxiv.org/abs/2405.01117)
supports treating exact candidate execution as a separate systems layer from
representation learning. [Two-Step SPLADE](https://arxiv.org/abs/2404.13357)
likewise motivates separating retrieval quality from traversal optimization.

Therefore a row can fail for exactly one observable reason:

1. float-to-u8 representation loss;
2. exact-engine identity failure;
3. unacceptable native bytes/latency;
4. genuine retrieval-quality loss of the frozen M190 representation.

No new loss, pruning rule, or scorer is introduced until this attribution is
complete.

## Per-Dataset Gates

- M1960 manifest, checkpoint, documents, queries, IDs, and qrels signatures
  match exactly;
- all query IDs equal the official qrel query set and every positive document
  exists;
- sampled known-ID, returned-score, score-multiset, and strict-boundary parity
  equal 1.0;
- sampled quantized/float Recall retention is at least 0.95;
- BMP p95 is at most 50 ms;
- index bytes/document are at most 2,000;
- costs are finite and positive.

The 50 ms ceiling is deliberately looser than the 7.203–17.905 ms validated
FiQA controls because corpus sizes grow by two orders of magnitude. It is a
predeclared product diagnostic, not a tuned search parameter.

## Matrix Decision

The final matrix compares raw M190 with the existing native BM25, PPLX
VectorChord dense, and P2.1 rows on identical official query identities.

The report emits separate quality and product decisions. Raw M190 remains a
quality-valid self-trained backup only if the complete matrix:

- retains at least 90% of dense Recall@100 and MAP@100;
- has no unexplained missing row or identity mismatch;
- is reported honestly as heldout-query/label but seen-corpus validation.

It remains a product-valid native backup only if every per-dataset engine gate
also passes. A latency or storage failure blocks product promotion without
being relabelled as representation failure.

The fixed M190+BM25 hybrid remains a broad9 complementarity ceiling. It is not
included as a full15 product route because it retrieves from separate semantic
and lexical candidate sources. If raw M190 loses to P2.1 on all principal
macro metrics after full15, stop historical model optimization and retain M190
only as training evidence. Do not reopen M210-M222 scorer/gate searches.

### Frozen MSMARCO decision boundaries

The first 14 exact-BMP rows were complete before canonical MSMARCO input
recovery finished. To prevent post-result gate movement, let `x_m` be
the raw M190 MSMARCO value for metric `m`. Its final macro is fixed by:

```math
\operatorname{macro}_{15}(m)
= \frac{14\operatorname{macro}_{14}(m)+x_m}{15}.
```

The existing full15 BM25, PPLX dense, and P2.1 matrix therefore fixes the
following MSMARCO boundaries before observing the M190 result:

| Metric | M190 MSMARCO needed for 90% dense retention | M190 MSMARCO needed to beat P2.1 macro |
| --- | ---: | ---: |
| NDCG@10 | >= 0.599170 | > 0.605511 |
| MAP@100 | >= 0.399958 | > 0.409295 |
| Recall@100 | already guaranteed by 14 rows | > 0.773850 |
| MRR@20 | >= 0.743963 | > 0.898610 |

These are diagnostic consequences of the frozen gates, not new per-dataset
targets. MSMARCO is still evaluated once with the unchanged representation and
exact engine. No threshold is used for training, scoring, or checkpoint
selection.
