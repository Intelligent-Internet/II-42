# II-42 M1950-M1951 OpenSearch One-Index Closure

Date: 2026-07-13

## Decision

**Retain the OpenSearch parent swap as lower-cost mechanism evidence, but
close deterministic additive calibration before native or neural promotion.**

## What Was Proved

M1950 replaced M1914 with the frozen mature OpenSearch sparse-v2 parent while
keeping the exact M1930 product shape:

```text
exact BM25 postings + bounded semantic postings
    -> disjoint columns in one sparse vector
    -> one physical inverted index
    -> one additive sparse dot product
```

The fixed `b1` route improved all four primary macro metrics over frozen
OpenSearch. M1951 then transferred the already validated M1931 `rms_m4`
qrels-free query calibration with no sweep and improved the macro result again.

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Frozen OpenSearch | 0.455414 | 0.355934 | 0.716066 | 0.495727 | 0.854005 |
| M1950 global b1 | 0.459558 | 0.358951 | 0.719397 | 0.496245 | 0.852964 |
| **M1951 rms_m4 b1** | **0.464823** | **0.363690** | **0.720414** | **0.501863** | 0.853268 |
| M1933 M1914 b1.125 | 0.460068 | 0.358209 | **0.726169** | 0.495059 | **0.859572** |

M1951 uses 11.11% fewer semantic entries and 5.88% fewer total entries than
M1933 `b1.125`, while improving four-row macro NDCG, MAP, and MRR. This is a
real quality/cost signal.

## Why It Stops

The macro signal is not row-safe. Relative to frozen OpenSearch, M1951 loses
`0.012444` FiQA NDCG, `0.010051` MAP, and `0.022931` MRR. Recall loses only
`0.002962` and CUB gains `0.005555`, proving that relevant candidates remain
visible but lexical evidence disturbs semantic head ordering.

M1950 fails LODO on the same row. M1951 improves FiQA over M1950 but still
fails selection and LODO. This replicated failure under both a global scale
and the strongest previously validated qrels-free query-local scale rules out
ordinary scalar calibration as the missing mechanism.

## Route Consequence

- Do not run native promotion or unseen transfer for M1950/M1951.
- Do not train a residual head on this four-row result.
- Do not search another scalar, calibration statistic, threshold, or selector.
- Keep M1934 `b1`/`b1.125` as the validated learned-sparse product frontier.
- Keep frozen OpenSearch as the mature ranking parent and M1951 as evidence
  that better cost/head-quality points exist if row protection can be made
  intrinsic rather than post-hoc.

The next research stage must alter the learned representation or its training
source so lexical harm separation is encoded before scoring. A post-hoc gate
would repeat the M1939-M1945 complete-neutral-field observability failure.
