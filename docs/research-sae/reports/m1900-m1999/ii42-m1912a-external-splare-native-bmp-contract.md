# M1912A External SPLARE Native BMP Contract

Date: 2026-07-12

Status: **locked before native benchmark**

## Question

M1510 evaluated an independent Gemma-Scope/SPLARE reproduction and found real
retrieval quality, but stopped the deployment route because every query
touched almost every document. M1910 later proved on Latent Terms that raw
posting-union touch can badly overstate cost for an optimized BMP engine.

M1912A asks one narrow question:

> Is the already-frozen M1510 FiQA sparse surface practical in the same exact
> BMP engine, despite its full-corpus raw touch estimate?

This is an engine re-audit. It does not retrain, re-encode, prune, select a
checkpoint, or claim an official SPLARE reproduction.

## Frozen Inputs

- M1510 independent reproduction output:
  `ii42-m1510-external-sparse-v1/results/fiqa`.
- Complete official FiQA: 57,638 documents and 648 qrels queries.
- Frozen sparse shape: 65,536 latent dimensions, document TopK 400, query
  TopK 40, positive sparse dot product.
- Existing float reference: NDCG@10 `0.333815`, MAP@100 `0.275953`,
  Recall@100 `0.636311`, MRR@20 `0.405323`, CUB@1000 `0.836936`.
- Engine: the patched official BMP 0.2 path used by M1660/M1910, block size
  16, global u8 document quantization, per-query f32 scaling, u32 scores,
  source document order, no BP reordering.

The M1510 JSONL files, evaluation JSON, and official qrels are hashed into the
surface manifest. Qrels are unavailable to conversion, quantization, index
construction, and candidate scoring.

M1510 declares exactly 65,536 latent dimensions, while the official BMP 0.2
term namespace requires fewer than 65,536 indexed terms. The frozen FiQA
surface uses only 34,510 dimensions across all document and query postings.
M1912A therefore applies one deterministic, qrels-free namespace remap: sort
the union of all nonzero document and query term IDs, map it contiguously, and
persist the original IDs. This removes only globally empty columns, preserves
every nonzero posting and every query-document dot product, and does not count
as pruning or model adaptation. The failed uncompressed-namespace surface is
retained as an audit artifact.

## Gates

The benchmark must report:

1. strict top-100 score-multiset and strict-boundary exactness for all queries;
2. quantized exhaustive and BMP NDCG@10, MAP@100, Recall@100, MRR@20, and
   CUB@1000;
3. quantized Recall@100 retention against the frozen float reference;
4. index bytes and bytes/document;
5. BMP and exhaustive p50/p95 latency;
6. maximum integer score and whether u16 would overflow.

The engine shape passes only if exactness is 1.0, Recall retention is at least
0.98, all 100 results are returned, and BMP p95 is lower than exhaustive sparse
scoring. A latency-only failure may authorize one fixed BP-ordering test; it
does not authorize model changes.

## Interpretation Boundary

- If native cost passes, revise only the old claim that full raw touch makes
  M1510 undeployable. The adapter still fails the quality/Pareto gate on FiQA
  and has incomplete training provenance.
- If native cost fails, retain the M1510 deployment stop. Do not tune weights,
  TopK, thresholds, or quantization on FiQA.
- This result cannot substitute for M1911 or the exact paper-shaped SPLARE
  training route. It decides whether expensive M1912 training should treat
  corpus fanout as an engine problem or a representation problem.

## Artifacts

- `scripts/prepare_m1912_external_splare_bmp_surface.py`
- `scripts/run_m1912_external_splare_bmp_closure_spark.sh`
