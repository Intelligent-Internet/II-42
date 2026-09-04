# M1572 Bounded Multi-Hit Controller Contract

## Objective

M1571's first-hit LSH source fails, but its frozen radius-2 buckets cover
`99.66%` of dense top100 and `99.49%` of dense top256. M1572 tests the only
remaining causal explanation:

> Does bounded cross-table collision accumulation expose that source capacity
> before the 30% posting-read budget is exhausted?

This is an index-internal controller audit over the exact M1571 source. It
does not change bucket membership, hash functions, probe order, or storage.

## Frozen Inputs

- M1571 basis SHA-256
  `f595bee2391268741a189e8cd924c3b558c8d62b33e72e2110daceedb23db340`;
- official FiQA M1565 dense basis;
- 16 tables, 12 bits, fixed seed 1571, radius-2 margin probes;
- exactly 16 document postings and the same sorted posting rows;
- fixed read budget `floor(0.30 * 57638) = 17,291` decoded postings;
- exactly 1,256 output candidates;
- no qrels, BM25, thresholds, learned scores, or fallback candidates.

For each decoded posting, accumulate one independent-table collision and its
query probe-margin cost. Rank observed documents lexicographically by:

1. descending number of table collisions;
2. ascending mean probe-margin cost;
3. ascending first probe rank;
4. document ID.

The candidate surface is persisted before qrels are loaded. Exact dense is
used only to rerank the frozen 1,256 candidates.

## Gates

Integrity requires exact M1571 basis lineage, exactly 17,291 or fewer reads,
exactly 1,256 unique candidates per query, and no document receiving more than
16 collision votes.

Capacity requires O@10 `>=0.98`, O@100 `>=0.95`, O@256 `>=0.90`,
Recall@100 `>=0.72`, and candidate upper bound `>=0.89`. It must also beat
M1565 route1000 by at least `+0.05` O@100 and `+0.08` O@256.

## Decisions

- All gates pass: replicate unchanged on NFCorpus and SciFact as a native
  controller candidate.
- Any gate fails: stop LSH and all single-hop semantic posting source work.

No alternative collision weight, per-table quota, block size, conjunction,
or read budget is authorized after the result.
