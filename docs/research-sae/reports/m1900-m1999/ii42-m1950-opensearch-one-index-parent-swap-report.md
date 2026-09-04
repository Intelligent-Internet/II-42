# II-42 M1950 OpenSearch One-Index Parent Swap

Date: 2026-07-13

## Decision

`stop_before_residual_training`

Lexical and semantic dimensions are disjoint columns of one sparse vector. Every evaluated score is one additive sparse dot product; there is no ANN, RRF, post-hoc reranker, or two-engine merge.

## Native BM25 Replay Gate

| Dataset | Pass | Top100 set min | Max metric delta |
| --- | ---: | ---: | ---: |
| fiqa | true | 0.980198 | 0.000000 |
| arguana | true | 0.980198 | 0.000009 |
| nfcorpus | true | 1.000000 | 0.000008 |
| scifact | true | 1.000000 | 0.000000 |

## Selected Global Configuration

`b1_qall_s0.5`

Budget ratio: `1.0`; query K: `0`; scale multiplier: `0.5`.

| Dataset | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| fiqa | 0.355205 | 0.297948 | 0.655998 | 0.429635 | 0.852518 |
| arguana | 0.401241 | 0.279592 | 0.986438 | 0.277506 | 1.000000 |
| nfcorpus | 0.350528 | 0.163735 | 0.284153 | 0.572313 | 0.566006 |
| scifact | 0.731259 | 0.694529 | 0.951000 | 0.705527 | 0.993333 |
| **macro** | **0.459558** | **0.358951** | **0.719397** | **0.496245** | **0.852964** |

## Parent Delta

The selected one-index result improves all four primary macro ranking metrics
over frozen OpenSearch. The small CUB loss means the result is a ranking
improvement, not a candidate-coverage expansion.

| Comparison | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1950 - frozen OpenSearch | +0.004144 | +0.003017 | +0.003331 | +0.000518 | -0.001040 |

The gain is not row-safe. FiQA loses substantial head quality while retaining
essentially the same Recall and gaining CUB. SciFact supplies the largest
positive movement.

| Dataset | Delta NDCG | Delta MAP | Delta R | Delta MRR | Delta CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| FiQA | -0.015039 | -0.012912 | -0.000044 | -0.026372 | +0.002881 |
| ArguAna | +0.003702 | +0.002169 | -0.002141 | +0.002423 | +0.000714 |
| NFCorpus | +0.005608 | +0.003473 | -0.001157 | +0.005906 | -0.007756 |
| SciFact | +0.022306 | +0.019336 | +0.016667 | +0.020115 | 0.000000 |

## Existing Frontier Comparison

All values below use the same four selection corpora. M1930B is the exact
same global deterministic publisher applied to M1914. M1933 adds the frozen
M1931 qrels-free query-local calibration and then compares the `b1` and
`b1.125` posting budgets.

| Route | Semantic budget | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1930B M1914 | 1.000x | 0.453310 | 0.352296 | 0.721610 | 0.489030 | 0.857232 |
| M1933 M1914 | 1.000x | 0.458584 | 0.356845 | 0.722599 | 0.493992 | 0.857603 |
| M1933 M1914 | 1.125x | 0.460068 | 0.358209 | 0.726169 | 0.495059 | 0.859572 |
| **M1950 OpenSearch** | **1.000x** | **0.459558** | **0.358951** | **0.719397** | **0.496245** | **0.852964** |

Against the exact M1930B mechanism, the parent swap adds `0.006248` NDCG,
`0.006655` MAP and `0.007215` MRR, but loses `0.002213` Recall and `0.004268`
CUB. Against the current M1933 `b1.125` selection frontier, M1950 is within
`0.000510` NDCG, improves MAP by `0.000742` and MRR by `0.001186`, but loses
`0.006772` Recall and `0.006608` CUB.

M1950 uses `11.11%` fewer semantic posting entries and `5.88%` fewer total
lexical-plus-semantic entries than `b1.125`. This identifies a potentially
useful lower-cost head-quality operating point. It does not replace M1934:
M1934's fixed `b1.125` route passed LODO, unseen-corpus transfer, and exact
native replay, while M1950 has not passed the first of those gates.

## Leave-One-Dataset-Out Gate

Pass: `false`

| Held out | Train-selected configuration | Pass | Delta NDCG | Delta MAP | Delta R | Delta MRR | Delta CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| FiQA | b1_qall_s0.5 | false | -0.015039 | -0.012912 | -0.000044 | -0.026372 | +0.002881 |
| ArguAna | b1_qall_s0.5 | true | +0.003702 | +0.002169 | -0.002141 | +0.002423 | +0.000714 |
| NFCorpus | b1_qall_s0.5 | true | +0.005608 | +0.003473 | -0.001157 | +0.005906 | -0.007756 |
| SciFact | b1_qall_s2 | true | +0.006542 | +0.008849 | 0.000000 | +0.009332 | 0.000000 |

FiQA is the sole LODO blocker. Its failure is specifically a head-ranking
inversion: Recall is effectively unchanged and CUB improves, while NDCG, MAP,
and MRR regress. This is consistent with a query-source calibration mismatch;
it is not evidence that the OpenSearch parent lacks lexical complementarity.

## Artifact Identity

The OpenSearch parent is pinned to model revision
`269e6638b2c4f648996691f6d751495285d8f330`. Every surface file was bound by
manifest identity, byte size, and SHA-256 before evaluation. The FiQA semantic
cache contained the exact same 57,638 document IDs in a different row order;
56,897 rows were deterministically permuted to the frozen reference order.
Missing, duplicate, or unequal IDs were fatal. No scores or supports were
changed by this alignment.

## Interpretation

M1950 is a positive structural parent-swap result, but not a promotable
product result. It shows that the mature OpenSearch semantic parent can reach
near-`b1.125` head quality at the lower `b1` posting budget inside one exact
inverted index. It does not establish row-safe generalization, and therefore
does not authorize residual training or native-canary promotion.

M1951 completed the only authorized continuation by transferring the frozen
M1931 `rms_m4` policy without a method or threshold sweep. It improved every
macro metric over M1950 but still lost 3.36% FiQA NDCG and 5.03% FiQA MRR
against frozen OpenSearch. The OpenSearch additive-calibration branch is
therefore closed, and M1934 `b1`/`b1.125` remains the validated learned-sparse
product frontier.
