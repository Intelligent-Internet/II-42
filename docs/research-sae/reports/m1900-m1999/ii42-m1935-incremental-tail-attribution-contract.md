# II-42 M1935 Incremental-Tail Attribution Contract

## Question

M1934 proved that expanding the semantic posting budget from `1.0x` to
`1.125x` lexical postings improves all unseen macro quality metrics and
reproduces through the native one-index path. It also raises mean posting
touches by 7.61% and SciDocs native p95 latency by 8.75%.

M1935 asks whether the **added tail only** contains a qrels-free, index-build
observable subset that retains most of the quality gain at materially lower
traversal cost.

This is not M1932 repeated. M1932 used DF/IDF to replace postings inside the
protected `b1` support and lost quality. M1935 freezes every `b1` posting and
only filters the additional postings between `b1` and `b1.125`.

## Frozen Surface

- datasets: SciDocs, Quora, and TREC-COVID;
- semantic parent, support, impacts, and query calibration: frozen M1934;
- lexical postings: exact M1930 cache;
- baseline: exact `b1` support;
- reference expansion: the exact M1934 `b1.125` support;
- attribution expansion: a canonical nested `b1.125` support that preserves
  every `b1` posting at the same exact posting count;
- score: one additive sparse dot product;
- candidate depth: 1,000.

No impact change, query gate, qrel-derived posting selection, row-specific
threshold, or model training is allowed.

## Predeclared Attribution Bins

For each target corpus, measure each semantic atom's document-frequency ratio
on the complete `b1.125` semantic surface. Evaluate cumulative added-tail
routes:

```text
b1
b1 + added postings whose atom DF <= 0.01
b1 + added postings whose atom DF <= 0.10
b1 + added postings whose atom DF <= 0.50
b1.125 (all additions, DF <= 1.00)
```

The DF ratio is available while building the target index and uses no qrels.
The M1934 publisher is first audited for non-nested tie swaps. M1935 then
constructs the attribution endpoint by retaining all `b1` postings and adding
the highest-impact unused postings under the same balanced document policy.
Every attribution route must be an exact subset of this nested endpoint and
preserve original M1914 weights. If the nested endpoint does not preserve the
M1934 macro gain and row floors, attribution stops because the apparent gain
was not a pure capacity effect.

## Diagnostic Gate

A partial route may authorize broader leave-one-dataset-out validation only
when it satisfies all of the following on the three unseen rows:

1. retains at least 70% of the full M1934 macro gain in NDCG@10, MAP@100,
   and MRR@20;
2. does not reduce macro Recall@100 below `b1`;
3. retains at least 50% of the full candidate-upper-bound gain;
4. uses at most 75% of the extra postings and at most 75% of the extra mean
   posting touches consumed by `b1.125`;
5. keeps every row above the M1934 Recall, NDCG, and MRR safety floors.

This gate is diagnostic because qrels are used to measure gain retention. A
passing cutoff is not deployable until it succeeds under seven-row nested
leave-one-dataset-out selection across the four M1933 rows and the three M1934
rows.

## Stop Rules

- If no partial route passes, close DF-only filtering of the incremental tail.
- Do not respond with a finer cutoff grid; the predeclared bins answer whether
  DF contains a useful first-order separation.
- Do not train a selector on a failed attribution source.
- If a route passes, validate the same cutoff family under seven-row LODO
  before native replay or ClearML training.

## Training Boundary

Training is authorized only after a deterministic source passes broader LODO
and native quality-cost gates. The training objective must then reproduce the
accepted added-tail source while preserving the complete `b1` support. Loss
improvement without retrieval and traversal improvement is not a scale signal.
