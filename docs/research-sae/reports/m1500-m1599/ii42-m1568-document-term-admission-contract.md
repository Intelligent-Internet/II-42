# M1568 Document-Term Admission Contract

## Objective

M1566 proves that all exact lexical occurrences contain enough complementary
dense capacity, while M1567 proves that whole-term DF filtering removes it.
M1568 tests the remaining HI2 hypothesis before neural training:

> Can a dense teacher choose 15 real terms per document such that the bounded
> posting source preserves dense neighbors and transfers to unseen queries?

This is a qrels-free source-transcodability audit, not a deployable model.

## Frozen Teacher

- official FiQA and the exact M1565 route basis;
- full exact lexical document terms with M1566 BM25 impacts;
- dense top256 query-document ranks as the only teacher;
- for each teacher query/document pair, add `1/log2(rank+2)` mass to exact
  terms shared by the query and document;
- select 15 terms per document by teacher mass, then BM25 impact and term ID;
- use BM25-impact fallback where the teacher supplies no shared term;
- route1000 plus at most 256 selected-term candidates;
- no qrels, labels, score interpolation, thresholds, or external index.

## Transfer Surface

1. Transductive ceiling: construct from all 648 queries and evaluate all.
2. Held-out transfer: three deterministic 50/50 splits with seeds
   `1568,2568,3568`; construct from train queries and evaluate only held-out
   queries.
3. Compare every row with unsupervised term15 under the same held-out queries,
   candidates, codec, and read accounting.

All source assignments and candidate surfaces must be persisted before qrels
are loaded.

## Gates

Integrity requires exact route1000 parity and mean combined reads at most 30%
of corpus size.

The transductive dense-teacher oracle must reach O@100 `>=0.98` and O@256
`>=0.95`.

The three-split held-out teacher oracle must have:

- mean O@100 `>=0.95` and O@256 `>=0.90`;
- positive O@100 and O@256 gains over unsupervised term15 on every split;
- mean O@100 gain `>=0.08` and O@256 gain `>=0.10`;
- valid candidate and read costs on every split.

## Decisions

- Both gates pass: authorize cross-corpus frozen-root term-selector training.
- Transductive passes but held-out fails: stop query-derived term admission as
  non-transferable.
- Transductive fails: stop exact-term top15 selector training at source level.

Do not change term budget, splits, teacher depth, rank weight, or thresholds
after observing the result.
