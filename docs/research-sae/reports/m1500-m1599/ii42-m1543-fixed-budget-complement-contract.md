# M1543 Fixed-Budget Semantic/Lexical Complement Contract

## Question

M1542 route atoms retain 94.3% of exact-BGE Recall@100 at a 15% candidate
union, but fail dense-equivalence. Before discarding them entirely, M1543 asks
one final retrieval-specific capacity question:

> At the same total candidate budget, do frozen semantic routes and frozen
> BM25 rankings contain complementary relevant documents?

This corrects an unfair CUB comparison in M1541/M1542: a 15% route union was
compared with the top1000 dense CUB even though the canary corpora contain only
about 2000 documents. M1543 compares all candidate sources at exactly 15%.

## Frozen Construction

- reuse M1542 dual-assignment spherical routes without changing centroids,
  target cell size, assignment count, or tail scorer;
- semantic route receives exactly 10% unique-document budget;
- reference BM25 adds its highest-ranked unseen documents until the total
  unique union reaches exactly 15%;
- compare against exact BGE top15% and reference BM25 top15%;
- qrels are used only for final capacity and quality metrics;
- no dataset-specific split, threshold, alpha, gate, or model training.

## Ranking Rows

- exact BGE top15%;
- reference BM25 top15%;
- dual semantic route 10% with tail256 INT8;
- semantic 10% + lexical fill-to-15%, reranked by tail256 only;
- the same union with frozen P1 weighted reciprocal-rank fusion:
  semantic weight `0.875`, BM25 weight `0.125`, RRF constant `60`;
- exact-BGE rerank of the union as a scorer ceiling.

The reference BM25 ranking is an existing qrels-free native artifact. M1543
counts the number of BM25 candidates admitted but does not claim to measure
BM25 posting traversal from that artifact.

## Candidate-Capacity Gate

For each dataset, the fixed union passes when its candidate upper bound exceeds
the better equal-budget dense/BM25 CUB by at least `0.005`. The capacity source
is viable only when:

- at least two of three datasets pass;
- no dataset is more than `0.01` below its better equal-budget baseline;
- total unique candidate union is at most `0.151`.

## Retrieval Gate

The frozen P1 rank fusion passes only when:

- macro Recall@100 and MAP@100 both exceed the better macro dense/BM25 row;
- macro NDCG@10 and MRR@20 are no worse than 98% of the better baseline;
- every dataset retains at least 90% of its better-baseline Recall and NDCG.

## Decisions

- **Capacity and retrieval pass:** promote to native-index validation; no
  training is needed yet.
- **Capacity pass, retrieval fail:** the candidate source is valid and one
  global constrained listwise scorer is authorized. Its teacher and stop gate
  must be defined from the fixed union before training.
- **Capacity fail:** stop this route family. Do not train a selector, output
  head, or larger route codebook.

