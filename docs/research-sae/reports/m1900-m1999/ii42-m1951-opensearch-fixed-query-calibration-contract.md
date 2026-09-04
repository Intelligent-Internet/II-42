# M1951 OpenSearch Fixed Query Calibration Transfer Contract

Date: 2026-07-13

## Question

M1950 showed that exact BM25 plus bounded OpenSearch semantic postings can
improve four-row macro head quality at the `b1` posting budget, but FiQA lost
NDCG, MAP, and MRR despite stable Recall and higher CUB. M1931 previously
removed the analogous fixed-scale inversion for M1914 with a qrels-free
query-local calibration that later passed LODO, unseen transfer, and native
replay.

M1951 asks one controlled question:

> Does the already frozen M1931 `rms_m4` query calibration transfer to the
> OpenSearch parent and remove the M1950 FiQA head inversion?

This is not a new scalar search, model training run, reranker, or second
engine.

## Frozen Inputs

- semantic parent, revision, surfaces, identity checks, and exact FiQA row
  alignment: M1950;
- exact lexical substrate and BM25 replay gate: M1930/M1950;
- semantic document budget: `1.0x` lexical postings;
- semantic query support: all parent atoms;
- calibration mode: `rms`;
- calibration multiplier: `4`;
- clip interval: `0.25x` to `4.0x` the qrels-free global semantic floor;
- candidate depth: 1,000;
- score: one additive sparse dot product over disjoint columns.

No mode, multiplier, budget, threshold, or dataset-specific value may be
searched. Qrels are evaluation-only.

For query `q`, the semantic scale is computed before retrieval from lexical
and semantic query postings plus corpus column RMS statistics:

```text
raw(q) = lexical_query_proxy_rms(q) / semantic_query_proxy_rms(q)
scale(q) = clip(4 * raw(q), 0.25 * global_floor, 4 * global_floor)
```

## Gates

1. All M1950 artifact-identity and native-BM25 replay checks must pass.
2. The single `rms_m4` configuration must pass the unchanged M1931 selection
   and leave-one-dataset-out gates.
3. FiQA NDCG, MAP, and MRR must each improve over M1950; stable Recall alone
   is insufficient.
4. Relative to frozen OpenSearch, no row may lose more than 1% NDCG or MRR,
   and no row may lose more than 0.01 absolute Recall.
5. Macro NDCG, MAP, Recall, and MRR must all exceed frozen OpenSearch. CUB may
   not fall by more than 0.5% relative.

## Stop Rules

- If any gate fails, close the OpenSearch additive-calibration branch. Do not
  add another calibration mode, multiplier, threshold, gate, or neural loss.
- If all gates pass, authorize one exact native replay on NFCorpus and SciFact
  before unseen-corpus transfer.
- Neural continuation remains unauthorized. It requires an independent,
  corpus-disjoint training and native-cost contract.
