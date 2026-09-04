# M645 / P1.3 Replacement-Target Ranker Report

## Status

M645 is an informative negative result.  It converts M644 query-local
replacement supervision into graded row targets and trains a nonlinear
`HistGradientBoostingRegressor`.  The intent was to combine M641's nonlinear
capacity with M644's protected-positive behavior.

The result is not promotion-ready.  It improves over the frozen P1.3-a010 proxy,
but it does not recover the M641 breakthrough level and it still leaves a major
dataset regression.

## Compared Runs

| Run | alpha | preserve | window | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | promoted | displaced | net | status |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M641 validation | 0.10 | 95 | 300 | +0.004468 | +0.000131 | +0.000000 | +0.000000 | 57 | 49 | +8 | breakthrough candidate |
| M643 conservative | 0.10 | 95 | 300 | +0.003487 | +0.000088 | +0.000000 | +0.000000 | 20 | 13 | +7 | dominance limited |
| M644 query-local | 0.15 | 95 | 350 | +0.000552 | +0.000088 | +0.000000 | +0.000000 | 7 | 2 | +5 | not promoted |
| M645 base | 0.05 | 98 | 250 | +0.001706 | +0.000086 | +0.000000 | +0.000000 | 10 | 4 | +6 | not promoted |
| M645 aggressive | 0.05 | 99 | 300 | +0.001092 | +0.000103 | +0.000000 | +0.000000 | 8 | 2 | +6 | not promoted |

## M645 Base Details

| Metric | Value |
| --- | ---: |
| Baseline Recall@100 | 0.889652 |
| Candidate Recall@100 | 0.891358 |
| Candidate upper bound | 0.993377 |
| Net top100 positives | +6 |
| Positive gain efficiency | 0.600 |
| Max positive dataset share | 0.546 |

Role distribution:

| Role | Count |
| --- | ---: |
| boundary_negative | 33190 |
| protected_positive | 4889 |
| safe_negative | 16213 |
| tail_negative | 33280 |
| tail_positive | 336 |
| under_positive | 1438 |

Dataset recall deltas for the base run:

| Dataset | dRecall@100 |
| --- | ---: |
| arguana | +0.000000 |
| climate-fever | +0.010101 |
| cqadupstack | +0.000000 |
| dbpedia-entity | +0.000896 |
| fever | +0.000000 |
| fiqa | +0.000000 |
| hotpotqa | +0.000000 |
| msmarco | -0.002041 |
| nfcorpus | +0.005197 |
| nq | +0.000000 |
| quora | +0.000000 |
| scidocs | +0.001724 |
| scifact | +0.000000 |
| trec-covid | +0.000587 |
| webis-touche2020 | +0.000000 |

## What This Proves

M645 answers a narrow question: whether M641's promotion capacity can be kept
while replacing row-level labels with M644-style protected replacement targets.
The answer is no in this form.

The aggressive target replay is important.  Increasing under-positive targets
and reducing negative penalties did not move M645 toward M641.  It reduced
Recall@100 from +0.001706 to +0.001092.  That means the block is unlikely to be
simple target conservatism.

The current bottleneck is the selection interface.  M645 still trains rows as
mostly independent graded examples.  The actual ranking decision is query-local:
which positive should displace which negative inside a specific top100 boundary.
M644 captured this locality but was linear and weak.  M645 added nonlinear
capacity but lost too much replacement specificity.

## Current Limits And Blocks

- Candidate supply is not the primary block on this audit surface.  The
  candidate upper bound remains high at 0.993377, while Recall@100 remains
  around 0.89.
- Fixed scorer capacity is partly sufficient.  M641 can promote 57 positives,
  but it also displaces 49 positives and fails dominance.
- Conservative protection is not enough.  M644 and M645 reduce displacement, but
  they do not promote enough under-ranked positives.
- Dataset-wide dominance remains fragile.  M645 base has a major MSMARCO
  regression of -0.002041 Recall@100.
- Row-regression target shaping is now a low-priority path.  The aggressive
  replay made it worse, so further hand-tuning targets is likely to loop.

## Next Breakthrough Point

The next useful experiment should be M646: a query-local replacement policy,
not another row regressor.

Recommended shape:

1. Build explicit per-query boundary slots from native rows:
   positives ranked 101-1000 compete only against negatives or low-value rows
   currently occupying ranks 80-140.
2. Train a pair/listwise scorer on replacement decisions:
   for each query, score `promote_positive - displaced_candidate`, not isolated
   row quality.
3. Keep M641 nonlinear features, but optimize a query-local objective:
   RankNet/LambdaRank-style pair loss or a small GBDT pair classifier over
   difference features.
4. Gate by native replay:
   Recall@100 must beat M641 validation or at least exceed +0.005, while
   NDCG@10/MRR@20 stay flat and no dataset regression exceeds 0.001.
5. Stop if M646 cannot exceed M641's promoted-positive count without matching
   M644/M645's displacement control.

In short: M641 found capacity, M644 found locality, M645 showed that target
regression does not combine them.  The next route is a nonlinear query-local
replacement model that directly learns the boundary swap.
