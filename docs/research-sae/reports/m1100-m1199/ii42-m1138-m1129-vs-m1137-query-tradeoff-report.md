# ii42 M1138 M1129-vs-M1137 Query Tradeoff Audit

## Purpose

M1137 improved Recall@100 over M1129 on shared15, but regressed MRR@20,
NDCG@10, and MAP@100. M1138 checks whether that tradeoff is separable at query
level. If clean gains can be selected with inference-safe features, M1137 could
be distilled into a constrained policy. If not, the route should stop.

Compared rows:

- Baseline: M1129 fixed `alpha=0.60`, `gamma=1.00`
- Candidate: M1137 heldout-best score shape `alpha=0.50`, `gamma=0.50`
- Split: `heldout`
- Surface: shared15 ranking exports

## Query-Level Outcome Classes

Across 403 heldout queries:

| Class | Count |
| --- | ---: |
| unchanged | 177 |
| pure damage | 96 |
| clean gain, no Recall gain | 69 |
| clean Recall gain | 36 |
| mixed non-Recall tradeoff | 15 |
| Recall gain with rank damage | 10 |

M1137 is not uniformly bad. It has 105 clean-gain queries, including 36 clean
Recall gains. The problem is that these gains are mixed with 121 damage or
tradeoff queries.

## Dataset Mean Delta

M1137 best-shape minus M1129:

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Read |
| --- | ---: | ---: | ---: | ---: | --- |
| arguana | +0.000000 | -0.006845 | +0.022412 | -0.006896 | mixed |
| climate-fever | -0.008333 | -0.004578 | -0.009738 | -0.018145 | damage |
| cqadupstack | +0.048937 | +0.042407 | +0.029110 | +0.043465 | real gain |
| dbpedia-entity | +0.014774 | +0.016667 | +0.036070 | +0.032195 | real gain |
| fever | +0.000000 | -0.038889 | -0.028969 | -0.038889 | damage |
| fiqa | +0.058333 | -0.141931 | -0.101456 | -0.117497 | severe tradeoff |
| hotpotqa | +0.000000 | +0.016667 | -0.014713 | -0.014385 | mixed |
| msmarco | -0.055517 | +0.000000 | -0.004894 | -0.046478 | damage |
| nfcorpus | +0.065328 | -0.037222 | +0.027223 | +0.014308 | mixed gain |
| nq | +0.000000 | +0.001667 | +0.008138 | +0.015184 | small gain |
| quora | +0.000000 | +0.011905 | +0.012172 | +0.016916 | small gain |
| scidocs | -0.001667 | -0.039199 | +0.000736 | +0.002778 | mixed |
| scifact | -0.006667 | -0.040356 | -0.028925 | -0.036749 | damage |
| trec-covid | +0.012161 | -0.011111 | +0.049541 | +0.047857 | mixed gain |
| webis-touche2020 | +0.021935 | -0.033333 | -0.019842 | -0.005453 | tradeoff |

The gain is concentrated in a few datasets. FiQA is the clearest warning:
M1137 buys substantial Recall but severely damages top-rank quality.

## Separability Check

The first selector smoke accidentally included qrels-derived features
(`positive_candidate_count`, `best_pos_*`, and top1 label features). That result
is invalid and was discarded.

The valid test used only inference-safe score-geometry features:

- candidate count
- lexical/atom score correlation
- lexical/atom overlap at 10/50/100
- cross-ranks of each top1
- lexical and atom top1-to-top10 score gaps

LODO results:

| Metric | Macro |
| --- | ---: |
| AUC | 0.544497 |
| Average precision | 0.584551 |
| Precision@0.5 | 0.490021 |
| Recall@0.5 | 0.365934 |
| F1@0.5 | 0.368282 |

This is not enough separability for a robust global policy.

## Safe-Feature Selector Replay

The selector chooses between M1129 and M1137 for each query. All rows below are
leave-one-dataset-out.

| Threshold | M1137 Queries | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.30 | 375 | 0.781898 | 0.765366 | 0.699427 | 0.600921 |
| 0.50 | 182 | 0.782102 | 0.770511 | 0.698660 | 0.605527 |
| 0.70 | 45 | 0.773904 | 0.778360 | 0.700130 | 0.607879 |
| 0.85 | 8 | 0.773108 | 0.779772 | 0.701562 | 0.606606 |
| M1129 baseline | 0 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |

A very conservative threshold can produce tiny NDCG/MAP gains, but the effect is
small and accepts only 8 to 45 queries. It does not recover the main Recall gain
without spending MRR or MAP.

## Oracle Ceiling

The failure is not that M1137 has no useful signal. The failure is that the
useful signal is not separable by the current inference-safe query-level
features.

| Policy | M1137 Queries | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1129 baseline | 0 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| M1137 global | 403 | 0.782350 | 0.762162 | 0.698423 | 0.598732 |
| safe all-metric oracle | 105 | 0.790654 | 0.802855 | 0.729743 | 0.632544 |
| Recall no-rank-loss oracle | 36 | 0.790654 | 0.785624 | 0.709370 | 0.613910 |
| MAP oracle | 121 | 0.790907 | 0.797710 | 0.728524 | 0.634644 |
| NDCG oracle | 83 | 0.776754 | 0.803048 | 0.733721 | 0.630075 |

This ceiling is large enough to justify one more route, but not another
query-scalar gate. The next route needs richer candidate/doc-pair evidence,
because the current query-level geometry cannot identify the safe subset.

## Verdict

Do not continue the M1137 selector route.

The route produced useful diagnostic evidence:

1. Dual-gamma can create clean per-query gains.
2. Those gains are not reliably separable using inference-safe score geometry.
3. Broad application damages top-rank quality.
4. Conservative selection yields only tiny improvements.

This means the next useful work is not more gamma/alpha/gate tuning. The
M1129 fixed-alpha shared15 line remains the current baseline.

The one remaining valuable direction is to convert the safe oracle rows into a
candidate/doc-pair teacher:

1. protect M1129 top-rank positives and top lexical evidence;
2. add M1137-only clean-gain positives as expansion targets;
3. train at candidate/doc-pair level rather than query-scalar level;
4. reject the route if leave-dataset-out replay cannot recover a material
   fraction of the safe-oracle ceiling.

This changes the evidence and supervision source. It is not another
gamma/alpha selector loop.
