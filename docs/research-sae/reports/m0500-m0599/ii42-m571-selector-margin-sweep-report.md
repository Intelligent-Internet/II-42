# M571 Selector Margin Sweep

M571 reviews whether M570 can be repaired by increasing the qrels-free
selector-overlap margin before accepting the learned M551 source.

## Evidence

M570 seed551 official-1024 BEIR7 exposed a mixed result:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned M551 | +0.006360 | +0.003820 | -0.000630 | +0.010660 | +0.001330 |
| M570 guarded, margin 0.000 | +0.005710 | +0.003470 | -0.000630 | +0.010330 | +0.001170 |

The default selector gate fixed neither the learned Recall regression nor the
task-level Recall losses.

Selector-margin sweep on seed551:

| Selector dOverlap Margin | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 | Accepted Tasks |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 0.000 | +0.005706 | +0.003467 | -0.000636 | +0.010326 | +0.001170 | nfcorpus, fiqa, scidocs, scifact, trec-covid, webis-touche2020 |
| 0.001 | +0.003346 | +0.002911 | -0.001306 | +0.007521 | -0.000544 | nfcorpus, fiqa, scidocs, scifact, trec-covid |
| 0.002 | +0.003280 | +0.002199 | +0.000221 | +0.008289 | -0.000523 | nfcorpus, scifact, trec-covid |
| 0.005 | +0.003280 | +0.002199 | +0.000221 | +0.008289 | -0.000523 | nfcorpus, scifact, trec-covid |
| 0.008 | +0.000159 | -0.000170 | -0.000186 | +0.004761 | -0.000476 | trec-covid |

## Interpretation

Increasing selector margin helps Recall at `0.002-0.006`, but it makes
heldout dense-overlap negative.  This is not a clean promotion gate.  The
selector surface is detecting part of the risk, but it is not stable enough to
serve as the only source-selection criterion.

The route is not dead: NDCG and MRR gains are strong and repeatable enough to
justify one more gate design.  But M570 should not proceed to a three-seed
official run as-is.

## Next Step

M572 should add a second qrels-free selector signal before source selection.
Candidates:

- selector dense overlap at multiple cutoffs, not only top100;
- selector score-correlation or rank-correlation against exact dense scores;
- selector candidate coverage of dense top-k sets at `k=20,100,200`;
- task-level fallback when selector query count is too small for a stable
  estimate.

The acceptance rule should require both:

- early-rank preservation: selector overlap/rank signal does not regress;
- coverage preservation: selector dense top-k coverage is not materially lower.

Only after this dual gate passes seed551 should we spend GPU time on seed552 and
seed553.
