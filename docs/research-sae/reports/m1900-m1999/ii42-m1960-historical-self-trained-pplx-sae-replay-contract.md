# M1960 Historical Self-Trained PPLX-SAE Replay Contract

## Purpose

M1960 answers one narrow question before any new training:

> Does the strongest reproducible historical self-trained II-42 model remain a
> useful native inverted-index retrieval parent under the current PPLX data,
> PostgreSQL posting engine, and full-corpus evaluation contract?

The primary checkpoint is M190, the repaired and strictly replayed descendant
of M150. M130 and M180 are already dominated on their historical surfaces and
will not consume new GPU time. M160 is retained only as a secondary control if
M190 behaves inconsistently.

This is a representation and product-path replay, not a new loss search.

## Evidence Behind The Contract

### Internal evidence

- M130 improved over still earlier SAE models but remained both low quality and
  expensive. It is not competitive with M150/M190.
- M150 was the first strong self-trained checkpoint, but its original custom
  evaluation data were not sufficiently reproducible.
- M190 rebuilt M150 from the audited PPLX root and fixed duplicate and split
  problems. It is therefore the canonical checkpoint lineage.
- M310 showed that the useful historical surface was not raw SAE dot product.
  It was a latent binary-BM25 interpretation of the M190 support, followed by
  fixed additive lexical rescue.
- M210-M222 already explored scorer, calibration, gate, and tail variants. The
  resulting gains were small or unstable. M1960 must not repeat that search.
- M1903-M1905 later showed why corpus DF must be measured explicitly: low row
  nnz does not prevent universal terms or excessive posting traversal.
- The historical M150 C4 audit found one pre-existing deterministic Pareto
  candidate: query-side `max_df_ratio=0.12` reduced posting work while improving
  three principal metrics. M1961 may replay that frozen policy after M1960, but
  may not reopen a threshold sweep.

### Literature constraints

- [DeepImpact](https://arxiv.org/abs/2104.12016) and
  [uniCOIL](https://arxiv.org/abs/2106.14807) establish learned, contextual
  impact values as a first-class inverted-index representation. Therefore,
  replacing M190 document impacts with corpus IDF is an ablation, not an
  automatic modernization. The raw learned-impact surface remains the primary
  representation test whenever it ranks better.
- [SPLADE-v3](https://arxiv.org/abs/2403.06789) establishes that learned sparse
  retrieval can generalize, but its successful recipe uses mature pretraining,
  hard negatives, and multi-teacher supervision. A short local loss sweep is
  not an equivalent reproduction.
- [Two-Step SPLADE](https://arxiv.org/abs/2404.13357) shows that sparse quality
  and sparse traversal cost are separate engineering surfaces. Native latency
  and posting traversal therefore remain mandatory M1960 outputs.
- [SPLATE](https://arxiv.org/abs/2404.13950) supports testing a frozen dense
  representation through a learned sparse interface before changing the
  backbone. M1960 follows the same diagnostic separation.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) shows that ordinary within-row
  sparsity does not control corpus-wide document frequency. M1960 records
  maxDF, posting counts, index bytes, and native latency rather than accepting
  average nnz alone.

## Frozen Data And Model Lineage

Primary checkpoint:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1/
    bm25sae_stageb_best.pt
```

Expected checkpoint SHA-256:

```text
49dfd1e8de9cbbe270c1f98793157101f1892cf3dd32d10033ead66d47e2360b
```

Expected model shape:

- dense input: 1,024 dimensions;
- latent vocabulary: 16,384 atoms;
- trained support: hard TopK 96;
- replay support: document TopK 64, query TopK 80;
- shared query/document SAE;
- dense source: `perplexity-ai/pplx-embed-v1-0.6B`, no prefix.

### Checkpoint training provenance

The checkpoint payload, rather than a mutable later audit file, is the source
of truth for the executed training configuration.

Stage A (`ii42-m190-m150a1-replay-stagea-v1`) used:

- the frozen PPLX 1,024-dimensional embedding surface;
- 16,384 non-negative features with trained hard TopK 96;
- three epochs over complete BEIR15 corpus embeddings;
- `query_repeat=2` and `expected_records=35,902,812` per epoch;
- reconstruction weight `1.0`, cosine weight `0.25`, neighborhood KL `0.5`,
  neighborhood MSE `0.1`, and fanout weight `0.05`;
- no coverage auxiliary and no candidate-evaluation auxiliary.

Stage B (`ii42-m190-m150-fusionnorm-strictscale-v1`) initialized from the
Stage-A best checkpoint and ran 5,000 ranking-refinement steps. Its principal
loss weights were recall `1.0`, multi-candidate CE `0.8`, single-candidate CE
`0.2`, teacher KL `0.25`, complement `0.35`, reconstruction `0.02`, and
K-budget `0.001`.

The training root contained complete BEIR15 documents, safe train/dev queries,
and corpus-derived synthetic queries. Official test-query overlap was audited
as zero and test qrel labels were excluded. This is therefore a heldout-query
and heldout-label evaluation, but not a heldout-corpus evaluation. M1960 must
not describe BEIR15 replay as proof of unseen-corpus generalization.

The final Stage-B checkpoint is 134,306,453 bytes. It is a trained sparse head
over PPLX embeddings, not a standalone text encoder: product inference still
requires the PPLX trunk before sparse compilation.

The official M310 roots are accepted only when their document/query JSONL and
qrels signatures are recorded in the output manifest. The current local
VectorChord PPLX rows have already been spot-checked against those JSONL roots;
the exporter makes the full replay identity auditable rather than relying on
that spot check.

## Mathematical Surfaces

Let the trained SAE produce non-negative hard sparse codes
`z_d, z_q in R^16384`. Encoding applies the historical normalized sparse-dot
geometry and then clips without re-normalizing:

```text
D(d) = TopK_64(normalize(z_d))
Q(q) = TopK_80(normalize(z_q))
```

### Surface A: raw SAE dot

```text
s_raw(q, d) = sum_a Q(q)_a D(d)_a
```

This surface isolates the quality of the self-trained representation. The
historical score scale of 20 is irrelevant to ranking and min-max fusion.

### Surface B: latent binary BM25

Document magnitude is replaced by corpus IDF while query magnitude is retained
through a saturating query-TF transform:

```text
df(a) = number of documents whose clipped support contains atom a

idf(a) = log(1 + (N - df(a) + 0.5) / (df(a) + 0.5))

qtf(x; k3) = x (k3 + 1) / (x + k3),  k3 = 8

s_latent(q, d) = sum_{a in support(q) intersect support(d)}
                 idf(a) qtf(Q(q)_a; 8)
```

This has an exact single-posting representation:

- document impact: `idf(a)`;
- query impact: `qtf(Q(q)_a; 8)`.

It does not require materializing a dense similarity matrix.

### Fixed lexical hybrid

The fixed historical calibration surface is chosen before observing new
results:

```text
s_hybrid(q, d) = minmax(s_latent(q, d))
                 + 0.5 minmax(s_BM25(q, d))
```

The candidate set is the union of the top native semantic and lexical
candidates. Weight `0.5` is the global full-15 macro winner from the old M310
surface. Per-dataset weight selection is prohibited.

This hybrid is not the single-index product result: the current evaluator uses
separate semantic and lexical candidate sources. It is retained as a fixed
quality ceiling and complementarity diagnostic. Raw M190 is the product-shaped
self-trained semantic posting backup. Final promotion therefore also requires
raw M190 to retain at least 90% of dense Recall@100 and MAP@100 on the complete
matrix.

## Execution Ladder

1. Export `nfcorpus` from the frozen M190 checkpoint on spark-1.
2. Verify checkpoint, input, ID, shape, support, and output signatures.
3. Publish raw and latent surfaces into separate local PostgreSQL schemas.
4. Evaluate raw SAE, latent SAE, and fixed latent+BM25 through the same native
   query path used by current product evaluations.
5. If implementation parity and quality gates pass, expand to `scifact`,
   `arguana`, and `fiqa`.
6. Expand beyond common4 only if common4 gives a credible backup-model case.
7. Train nothing until replay identifies one specific, observable bottleneck.

## Gates

### Identity gate

- checkpoint SHA and shape match this contract;
- document/query IDs are unique;
- evaluable query IDs exactly match positive native qrels;
- all embeddings are finite 1,024-dimensional vectors;
- all qrel-positive documents are present;
- raw and latent surfaces have exactly the same support;
- every output is signature-bound in the manifest.

### Implementation parity gate

- document nnz is at most 64 and query nnz at most 80;
- latent DF and IDF are computed from the complete dataset corpus;
- no qrels influence encoding, weighting, candidate generation, or scoring;
- a native/offline discrepancy greater than 0.02 absolute on any principal
  metric is treated as an implementation defect, not model failure.

### Common4 quality gate

The fixed global M190 latent+BM25 route must:

- beat native BM25 on at least three of four macro principal metrics;
- retain at least 90% of native PPLX dense Recall@100 and MAP@100;
- avoid a dataset-specific parameter or threshold;
- have no unexplained row with missing queries or documents.

Raw SAE is diagnostic and is not required to pass the hybrid gate.

### Cost gate

Report, by dataset and macro:

- document and query nnz;
- maxDF and maxDF ratio;
- posting count and normalized index bytes;
- postings touched, candidate count, p50/p95 query latency;
- quality per unit of posting and storage cost.

M1960 is a backup candidate, so cost does not need to beat P2.1 immediately.
However, a universal atom (`maxDF` near 1.0) or unbounded traversal must be
reported as a structural risk, not hidden by average nnz.

## Stop Conditions

Stop M190 expansion and do not optimize it when any of the following holds:

- native common4 Recall@100 or MAP@100 is below 90% of PPLX dense;
- fixed global fusion does not beat BM25 on at least three principal metrics;
- apparent gains require per-dataset weights or qrels-derived selection;
- the surface has materially worse cost than P2.1 without compensating
  retrieval quality;
- a proposed optimization duplicates M210-M222 or a later failed selector,
  gate, threshold, or tail search.

If M190 passes, the next step is not arbitrary fine-tuning. The report must
identify whether the remaining gap is representation quality, corpus DF,
lexical complementarity, or engine scoring. Only that isolated component may
be changed.

### Optimization admissibility

Any continuation after full-15 follows this decision order:

1. If raw M190 fails the dense-retention gate, stop the route. A cost objective
   cannot repair missing representation quality.
2. If raw M190 passes quality and native traversal is acceptable, preserve the
   checkpoint. Do not optimize a bottleneck that is not measured.
3. If raw M190 passes quality but traversal fails, first construct a frozen,
   deterministic pruning or reweighting frontier. For each candidate policy
   `p`, measure both retrieval quality `Q(p)` and exact posting work
   `T_p(q) = sum_{a in A_q(p)} df(a)` on held-out rows.
4. Training is admissible only if that oracle contains a non-trivial Pareto
   point: lower mean and p95 `T_p` while retaining row-safe Recall, MAP, NDCG,
   and MRR. The training target must approximate that demonstrated policy,
   rather than introducing another ungrounded sparsity loss.

This ordering separates representation capacity, corpus-wide cost, and
learnability. It also prevents repeating the M210-M222 calibration loop or the
later failure mode where average nnz improved while high-DF traversal did not.

## MSMARCO Historical Artifact Recovery

The active M310 MSMARCO embedding symlink is broken, but a complete physical
copy of its historical M150 PPLX target was recovered from Betty. The recovered
artifact is the primary source only after
`scripts/verify_m1960_historical_embedding_artifact.py` confirms its full
SHA-256, 8,841,823 rows, fixed anchors, sampled and qrel-referenced embedding
dimensions, 43-query identity, 4,102 qrel pairs, and M150 preparation summary.
The accepted signatures are recorded in
`ii42-m1960-pplx-rematerialization-parity.md`.

The recovered file must be copied to a separate Spark root, verified again
after transfer, and exported directly through the frozen M190 checkpoint. The
ongoing spark-2 rematerialization remains intact until the remote recovered
artifact and exported surface both pass. It is then a fallback only.

Raw-text rematerialization is admissible only if the recovered physical
artifact fails transfer or verification, and only under the already completed
PPLX parity contract. PostgreSQL `halfvec(1024)`, a new embedding model,
altered prefix, changed normalization, or dataset-specific scoring are not
authorized.

## Required Outputs

- signed export manifests and sparse NPZ surfaces;
- native JSON rows for raw SAE, latent SAE, and fixed latent+BM25;
- common4 per-dataset and macro matrix against BM25, PPLX dense, and P2.1;
- cost/latency matrix;
- a final M1960 report with one of three decisions:
  `promote backup`, `continue isolated repair`, or `stop historical route`.
