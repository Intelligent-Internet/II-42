# M1569 Exact Impact-Ordered Access Contract

## Objective

M1568 proves that dense-teacher document-level term admission has stable
held-out capacity, but its full posting scan exceeds the fixed read budget.
M1569 isolates execution from representation:

> Can an exact impact-ordered traversal return the identical top256 lexical
> candidate set while decoding at most 30% of the corpus postings per query?

This is an inverted-index access audit. It does not alter the M1568 source,
train a selector, approximate candidate scores, or use qrels.

## Frozen Source

- official FiQA and the exact M1565 route basis;
- M1568 transductive source and the three fixed 50/50 held-out sources;
- 15 selected exact terms per document;
- at most 32 exact query terms;
- route1000 plus 256 lexical candidates;
- identical BM25 term impacts, dense teacher depth, splits, and fallback;
- M1568 candidate surface SHA-256
  `960f53e488444704dc6c7f54921354ef4f3606571529224b192a60ab131dad20`.

## Exact Access

For each selected term, store postings in descending impact order. At query
time, decode the globally largest remaining term impact first and maintain a
lower score and a safe unseen-impact upper bound for every observed document.
After every 256 decoded postings, stop only when the current top256 lower-score
set cannot be displaced by any observed or unseen document.

The 256-posting check stride may over-read but cannot change the result. It is
fixed before evaluation and is part of the measured implementation.

## Gates

Integrity requires, on all 1,620 evaluated query/source pairs:

- exact lexical top256 set parity with the full scan;
- exact combined route-plus-lexical set parity with the persisted M1568
  candidate surface;
- no early stop justified by an unsafe or negative bound.

Cost requires the mean combined route and decoded lexical reads to be at most
`0.30x` corpus size for the transductive source and every held-out split.
Report p95 cost and read reduction, but do not add a post-result p95 threshold.

## Decisions

- Parity and cost pass: authorize a separate fixed document-term budget
  frontier to close M1568's held-out O@100 gap.
- Parity fails: stop for implementation-integrity failure.
- Parity passes but cost fails: stop this exact-term admission family; do not
  increase term budget or train a selector.

Even if M1569 passes, neural selector training remains blocked until a later
source frontier passes both held-out capacity and access cost.
