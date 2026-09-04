# M1566 HI2 Unified Source Capacity Contract

## Objective

M1565 proved that nearest-two corpus routes do not preserve dense retrieval at
a practical fixed budget. M1566 starts a new source inspired by HI2
(`arXiv:2210.05521`): semantic cluster entries and salient lexical entries are
stored in one inverted index and jointly generate one candidate union.

This is not ANN plus BM25 fusion. There is no vector index, external candidate
engine, score interpolation, qrels-trained gate, or dataset-specific policy.
Dense vectors are an offline teacher and an exact diagnostic codec only.

## Frozen Source

- official FiQA and the qrels-free M1565 route basis;
- nearest-two semantic route postings with a 1,000-candidate union;
- one lexical namespace using production-compatible lower-cased alphanumeric
  terms and BM25 impacts;
- fixed lexical reserve of 256 unique documents;
- fixed BM25 `k1=0.82`, `b=0.68` from the HI2 unsupervised configuration;
- two lexical document sources:
  - `full`: every unique document term;
  - `term15`: the 15 highest-impact terms per document;
- at most 32 unique query terms, selected by corpus-average term impact only
  when a query is longer;
- exact dense reranking over the reached union for source-capacity diagnosis.

The combined source may contain at most 1,256 documents per query. All route
and lexical posting reads are counted. Candidate surfaces must be persisted
before qrels are loaded.

## Diagnostics

For `full` and `term15`, compare:

1. deployable lexical order by BM25 impact;
2. a qrels-free dense-teacher oracle that chooses, from the same touched
   lexical posting lists, the documents that best cover dense top256.

The oracle changes no document postings and sees no qrels. It separates
source membership from lexical scoring/selection.

## Gates

Integrity requires exact M1565 route1000 parity, matching basis/root IDs, at
most 1,256 combined candidates, and mean posting reads at most 30% of the
corpus.

A deployable combined source passes when it has:

- O@10 at least `0.99`;
- O@100 at least `0.95`;
- O@256 at least `0.90`;
- Recall@100 at least 98% of exact dense;
- NDCG@10, MAP@100, and MRR@20 at least 99.5% of exact dense.

An oracle source requires O@100 at least `0.98` and O@256 at least `0.95`.

## Decisions

1. `term15` deployable pass: authorize protected-tail replay with the compact
   source.
2. Only `full` deployable pass: authorize replay with exact lexical terms;
   keep `term15` as a rejected compression.
3. Only an oracle passes: authorize one selector-distillation stage for that
   fixed posting source.
4. No oracle passes: stop cluster-plus-term source training.

Do not tune term counts, query terms, route/lexical budgets, BM25 parameters,
or thresholds after observing the result.
