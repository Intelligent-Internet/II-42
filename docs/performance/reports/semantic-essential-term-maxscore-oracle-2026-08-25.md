# Semantic Essential-Term MaxScore Oracle

## Decision

Stop planner, route-ratio, threshold, cache-window, and block-size tuning. The
only admitted CQ-3 query experiment is an exact essential-term decomposition of
the existing semantic BMP proof. It targets the remaining near-linear semantic
bound work directly and does not create another index, scorer, or lifecycle.

The representation oracle and a deployable-threshold oracle pass. The latter
derives a conservative kth-score lower bound from only the mandatory terms and
an essential semantic prefix. The complete reference pass is used only to
verify that the derived lower bound is safe; it is not an input to the bound or
the projected block set.

This is still not a deployable executor. The audit materializes complete score
and absolute-sum arrays and processes every term so it can compare all prefix
points in one run. Product work must first show that one forced essential
prefix can produce the same proof with bounded query memory and lower physical
I/O in the existing semantic BMP executor.

## Cost Diagnosis

The current query has four physically distinct costs. They must be measured and
reported separately:

1. PostgreSQL filter construction and allowed-document materialization.
2. Planner metadata used to choose direct, transpose, or bounded execution.
3. Exact top-k proof work: semantic superrefs, fine refs, bound pages, and
   posting metadata needed to reject noncompetitive blocks.
4. Exact scoring work for the surviving blocks and forward rows.

Directory v7 reduced planner metadata to a fixed term-work table. Directory v10
reduced competitive-row reads by scoring aligned eight-document ranges. Neither
change removes the mandatory semantic bound stream. On full PubMed, a query can
still expose 25.0--39.7 million semantic postings before exact proof completes.
The addressability audit also rejected b16/b64 coarsening and rank-select-only
membership: dispersed filters still touched almost every physical window.

The remaining bottleneck is therefore proof metadata for sparse and high-DF
semantic terms, not another routing boundary and not forward-row locality.

## Exact Decomposition

For each semantic query term, the audit computes a sign-safe global contribution
cap and sorts terms by descending cap. For the first `N` essential terms it
accumulates block-local bounds. All omitted terms are represented by one
conservative suffix sum, `R_N`, of their global caps.

For every block:

```text
full semantic contribution <= essential_local_bound_N(block) + R_N
```

A lower score is also available without a completed query seed. For every
document, the audit accumulates the mandatory and essential-prefix
contributions, then subtracts a conservative residual floor and a standard
floating-point summation error bound. The kth largest of those document lower
bounds is a lower bound on the final kth score. A block is rejectable only when
its lexical plus essential local bound and the residual global cap are strictly
below that lower bound. Every surviving block is still scored through the
existing exact score authority. This is a MaxScore-style proof decomposition,
not approximate scoring.

`N` is not a product threshold. The audit evaluates projection points to test
whether the representation contains a useful work minimum. A deployable policy
must derive its decision from measured reference and survivor work and must use
the same formula across roots and queries.

## Evidence

The conservative executor-work projection charges:

```text
mandatory postings
+ essential semantic postings
+ all query postings in competitive b16 blocks
```

The last term deliberately double-counts mandatory and essential postings in
competitive blocks, so the reported saving is conservative. `missed` is the
number of blocks competitive under the complete semantic bound but rejected by
the projected bound. It was zero in every row. Every derived kth lower bound was
also no greater than the complete reference kth score.

| Root | Query | Best forced N | Conservative work saved | Derived kth lower bound | Safe | Missed |
| --- | --- | ---: | ---: | ---: | --- | ---: |
| PubMed 500K | machine learning | 48 | 35.04% | 34.6872 | yes | 0 |
| PubMed 500K | cancer immunotherapy | 24 | 41.83% | 78.2765 | yes | 0 |
| PubMed 500K | COVID-19 vaccine efficacy | 24 | 27.63% | 72.4314 | yes | 0 |
| PubMed 500K | CRISPR gene editing | 32 | 48.25% | 66.5996 | yes | 0 |
| PubMed 500K | randomized controlled trial | 56 | 23.42% | 45.4991 | yes | 0 |
| PubMed 500K | protein structure prediction | 32 | 43.08% | 58.1994 | yes | 0 |
| PubMed full | machine learning | 48 | 41.68% | 38.4103 | yes | 0 |
| PubMed full | CRISPR gene editing | 24 | 58.31% | 79.1138 | yes | 0 |
| PubMed full | randomized controlled trial | 48 | 36.73% | 52.4092 | yes | 0 |

Artifacts and SHA-256:

```text
docs/performance/data/raw/semantic-essential-maxscore-2026-08-25/
pubmed-500k-deployable-kth.json
2f1518dc1fc5e3e14d5a0487d1a828d3fd1c11f7ea598fd2f042c172a622e7e1

docs/performance/data/raw/semantic-essential-maxscore-2026-08-25/
pubmed-full-deployable-kth.json
3e3df1ed4ed0d09f2f62fdbb0bfac1e15ae9fff7edf56e0456c7c51e26e60651
```

The full-root audit took approximately 115--144 seconds per query and about
7.6--7.9 GiB backend RSS because it materialized complete reference evidence.
Those numbers are diagnostic costs and explicitly disqualify the audit code
from a product query path.

## Product Gate

The next goal has one bounded implementation target: refactor the existing
exact semantic BMP executor so that a forced essential prefix derives a kth
lower bound, represents omitted terms by the conservative suffix cap, and
exact-scores every surviving block through the unchanged authority.

Required order:

1. Implement the decomposition behind a test-only forced route. Reuse the
   existing BMP directory, liveness, allowed bitmap, score authority, worker,
   WAL, publication, retirement, and reclamation.
2. Keep query memory bounded to a small multiple of corpus block count; never
   materialize all term-reference evidence or index-sized backend state.
3. Run bit-exact forced A/B on current-format small, FIQA, 500K, and full PubMed
   roots. Record decoded superrefs, refs, postings, physical pages and bytes,
   competitive blocks, exact-scored rows, latency, RSS, cancellation, and
   concurrency.
4. Derive any automatic prefix decision from observable reference, posting, and
   survivor cost. The best forced `N` values above are an executor-capability
   experiment, not product constants.
5. Consider automatic admission only if full-root p50/p95 and physical proof
   work both improve materially with zero score-bit difference.

## Stop Conditions

Reject this route without further tuning if any of the following occurs:

- the partial score pass cannot provide a proven kth lower bound;
- the exact implementation needs the audit's complete prepass;
- physical page/reference savings disappear outside the estimator;
- result ordinals or score bits differ;
- statement memory grows with query postings rather than corpus blocks;
- cancellation, concurrency, or repeated-query RSS fails;
- improvement requires query-, corpus-, or dataset-specific `N`, ratios, or
  thresholds.

If rejected, CQ-3 remains at directory v10 and the next research boundary is a
different skip-capable posting layout. It is not another planner constant.
