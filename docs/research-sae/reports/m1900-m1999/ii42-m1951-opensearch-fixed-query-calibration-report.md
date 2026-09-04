# II-42 M1951 OpenSearch Fixed Query Calibration Transfer

Date: 2026-07-13

## Decision

`stop_query_local_calibration`

The frozen M1931 `rms_m4` policy improves every macro metric over M1950 and
all four primary macro metrics over frozen OpenSearch, but it does not remove
the FiQA head inversion. The single fixed configuration fails both selection
and LODO. No native replay, unseen transfer, scalar variant, or training run is
authorized.

## Fixed Configuration

- semantic parent: OpenSearch sparse-v2 revision
  `269e6638b2c4f648996691f6d751495285d8f330`;
- semantic document budget: `1.0x` lexical postings;
- query support: all semantic atoms;
- qrels-free calibration: `rms_m4`;
- global semantic floor: `2.050116`;
- clip interval: `0.512529` to `8.200465`;
- candidate depth: 1,000;
- one additive sparse dot product, with no ANN, RRF, reranker, or second
  engine.

There was no mode, multiplier, budget, or threshold sweep.

## Result

| Dataset | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA | 0.357800 | 0.300809 | 0.653080 | 0.433075 | 0.855192 |
| ArguAna | 0.414513 | 0.288609 | 0.991435 | 0.286809 | 0.999286 |
| NFCorpus | 0.352173 | 0.163862 | 0.286143 | 0.575941 | 0.565261 |
| SciFact | 0.734807 | 0.701481 | 0.951000 | 0.711625 | 0.993333 |
| **macro** | **0.464823** | **0.363690** | **0.720414** | **0.501863** | **0.853268** |

Relative to frozen OpenSearch, M1951 has a strong macro result but one unsafe
row:

| Dataset | Delta NDCG | Delta MAP | Delta R | Delta MRR | Delta CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA | -0.012444 | -0.010051 | -0.002962 | -0.022931 | +0.005555 |
| ArguAna | +0.016975 | +0.011187 | +0.002855 | +0.011727 | 0.000000 |
| NFCorpus | +0.007252 | +0.003601 | +0.000832 | +0.009533 | -0.008501 |
| SciFact | +0.025854 | +0.026288 | +0.016667 | +0.026213 | 0.000000 |
| **macro** | **+0.009409** | **+0.007756** | **+0.004348** | **+0.006136** | **-0.000736** |

FiQA NDCG and MRR are respectively `3.36%` and `5.03%` below the frozen
OpenSearch parent. This violates the declared 1% row floor even though FiQA
CUB improves. The failure is head ordering, not candidate visibility.

## M1950 And Existing Frontier

The fixed query-local transfer improves all five macro metrics over M1950:

| Comparison | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1951 - M1950 | +0.005265 | +0.004739 | +0.001017 | +0.005617 | +0.000304 |
| M1951 - M1933 M1914 b1 | +0.006239 | +0.006845 | -0.002185 | +0.007871 | -0.004335 |
| M1951 - M1933 M1914 b1.125 | +0.004755 | +0.005481 | -0.005755 | +0.006804 | -0.006304 |

At the lower `b1` posting budget, M1951 has the strongest four-row macro head
metrics among these additive candidates. It still cannot replace M1934
`b1.125`, because M1934 passed LODO, unseen transfer, and exact native replay,
while M1951 fails row safety before either broader gate.

## Query-Scale Diagnosis

| Dataset | Min scale | Median scale | Max scale |
| --- | ---: | ---: | ---: |
| FiQA | 0.512529 | 1.192556 | 3.731243 |
| ArguAna | 2.344772 | 8.200465 | 8.200465 |
| NFCorpus | 0.512529 | 0.534448 | 8.200465 |
| SciFact | 0.512529 | 0.808023 | 3.160332 |

The qrels-free RMS policy correctly increases semantic dominance for ArguAna
and improves that row substantially. It also improves all FiQA head metrics
over M1950, but not enough to recover the frozen semantic parent. More global
semantic weight would asymptotically suppress lexical evidence and return to
OpenSearch; it would not solve when lexical evidence is useful.

The remaining problem is therefore not another scalar value. It is the lack
of a qrels-free, row-safe signal for semantic-head protection versus lexical
admission. M1939-M1945 already showed that post-hoc selectors can separate
sampled target/harm atoms but fail on the complete neutral field. Reopening a
mode, multiplier, threshold, or classifier sweep would repeat that closed
failure family.

## Gate Audit

| Gate | Result | Evidence |
| --- | --- | --- |
| Artifact identity and exact ID alignment | pass | Model/revision/manifests pinned; FiQA exact permutation recorded |
| Native BM25 replay | pass | Reused M1950 validated lexical surface |
| Unchanged M1931 selection | fail | `rms_m4` is not row-safe on FiQA |
| Leave-one-dataset-out | fail | FiQA heldout fails; folds containing FiQA cannot select the fixed route |
| FiQA improves over M1950 | pass | NDCG +0.002595, MAP +0.002861, MRR +0.003441 |
| 1% row head floor vs parent | fail | FiQA NDCG -3.36%, MRR -5.03% |
| Macro primary metrics exceed parent | pass | All four are positive |
| Macro CUB relative floor | pass | CUB loss is 0.086%, below 0.5% limit |

## Conclusion

M1951 establishes a useful but non-deployable lower-cost macro frontier. It
confirms that mature learned-sparse semantic evidence and exact lexical
evidence are complementary inside one physical inverted index. It also
confirms that deterministic query-local scalar calibration cannot make that
combination row-safe.

The OpenSearch additive-calibration branch is closed under the M1951 contract.
M1934 `b1`/`b1.125` remains the validated learned-sparse product frontier;
frozen OpenSearch remains the strongest mature head-quality control. Future
work must change the representation or make head protection intrinsic to the
training objective/source construction. It must not add another post-hoc
scalar or selector.
