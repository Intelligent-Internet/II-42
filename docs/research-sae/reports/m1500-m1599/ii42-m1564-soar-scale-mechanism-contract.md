# M1564 SOAR Scale-Mechanism Contract

## Objective

M1563 confirmed SOAR residual decorrelation on NFCorpus but produced only
`+0.0200` O@100 and `+0.015273` O@256.  The SOAR paper reports stronger gains
for larger corpora and higher recall targets.  M1564 tests that mechanism once
on official full FiQA without changing the construction.

This is not an override of the failed M1563 canary and cannot authorize model
training by itself.

## Frozen Configuration

- official clean FiQA root with 57,638 documents and 648 qrel queries;
- frozen `BAAI/bge-base-en-v1.5` embeddings;
- spherical k-means with `ceil(document_count / 16)` routes;
- naive centroid dual versus SOAR dual, both two edges per document;
- fixed SOAR `lambda=1`;
- 15% exact unique candidate union and at most 30.1% reads;
- exact dense reranking over touched candidates;
- qrels loaded only after source construction and diagnostics.

Do not tune lambda, fanout, route count, budget, query policy, or thresholds.

## Scale Gate

Relative to the equal-cost naive dual, official FiQA SOAR must have:

- score-error correlation ratio at most `0.80`;
- mean best route-rank ratio at most `0.95`;
- centroid-routed O@100 gain at least `0.04`;
- centroid-routed O@256 gain at least `0.03`;
- route-cover oracle O@100 gain at least `0.03`;
- no O@10 regression larger than `0.01`;
- valid union and read costs;
- O@100 gain at least twice the NFCorpus gain.

## Decision

- Pass: run the unchanged source on official SciFact as a replication row.
- Fail: stop SOAR after documenting its useful but insufficient geometry.
- Do not train a posting compiler or add lexical residuals from this audit.
