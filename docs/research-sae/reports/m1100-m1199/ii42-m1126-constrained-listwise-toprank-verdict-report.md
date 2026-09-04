# ii42 M1126 Constrained Listwise Top-Rank Verdict

M1126 tests whether the M1125b listwise recall gain can be kept while adding a
direct top-rank preservation constraint.  The run uses the same shared5 surface
as M1118/M1120, M1124, and M1125b:

- datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`
- rank epochs: `4`
- learning rate: `0.0007`
- listwise rank weight: `0.50`
- listwise temperature: `0.07`
- top-rank preserve weight: `0.25`
- top-rank preserve margin: `0.02`
- top-rank preserve k: `4`

## Run Status

Remote run:
`/home/huoju/leask/runs/ii42-m1126-shared5-listwise050-toprank-export-v1`

Local synced run:
`/Volumes/Betty/Tmp/ii42-m1000/m1126-shared5-listwise050-toprank-export-v1`

The run completed normally and wrote final JSON, checkpoint, dataset partials,
and ranking exports.

## Training Signal

| Epoch | Loss | Rank | Listwise | Top-Rank | Fanout | Background |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 3.461770 | 0.673450 | 1.672853 | 0.629432 | 39.425512 | 0.382896 |
| 2 | 2.213940 | 0.581559 | 0.463799 | 0.536775 | 27.844635 | 0.210986 |
| 3 | 1.365527 | 0.457676 | 0.099839 | 0.425423 | 16.251440 | 0.089777 |
| 4 | 0.662402 | 0.303004 | 0.044867 | 0.287686 | 5.277713 | 0.045933 |

The additional top-rank term did not destabilize training.  It decayed more
slowly than listwise loss, which is expected: it is a boundary-preservation
constraint rather than a simple in-batch softmax fit.

## Heldout Fine-Grid Result

Verdict from the fine-grid audit remains `do_not_promote_fixed_alpha`, because
train-selected alpha is still `0.75` while heldout-best alpha is `0.35`.
However, the representation/scorer shape is clearly stronger than previous
shared5 runs.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1120/M1118 best | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| M1124 listwise best | 0.719679 | 0.549041 | 0.496466 | 0.394045 |
| M1125b listwise050 best | 0.715489 | 0.562618 | 0.498005 | 0.391700 |
| M1126 heldout-best alpha=0.35 | 0.717658 | 0.578506 | 0.515893 | 0.411211 |
| M1126 selected alpha=0.75 | 0.740822 | 0.580870 | 0.509204 | 0.405299 |

Delta for M1126 heldout-best alpha=0.35:

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M1120/M1118 best | +0.029401 | +0.012345 | +0.016555 | +0.015846 |
| M1124 listwise best | -0.002021 | +0.029465 | +0.019427 | +0.017166 |
| M1125b listwise050 best | +0.002169 | +0.015888 | +0.017888 | +0.019511 |

## Query Tradeoff

M1126 changes the pressure pattern substantially:

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| 0.00 -> 0.25 | +0.036138 | +0.032332 | +0.027123 | +0.025681 |
| 0.25 -> 0.30 | +0.001537 | +0.006099 | +0.003131 | +0.006660 |
| 0.30 -> 0.50 | +0.010637 | +0.003861 | -0.000631 | -0.001362 |
| 0.50 -> 0.75 | +0.019374 | +0.002895 | -0.000942 | +0.000382 |

The key improvement is that early atom pressure no longer only buys recall by
spending top-rank quality.  The 0.00 -> 0.25 and 0.25 -> 0.30 transitions are
cleanly positive on all four heldout macro metrics.

## Verdict

M1126 is the first M112x shared5 result in this segment that materially
improves the M1118/M1120 best baseline across Recall@100, MRR@20, NDCG@10, and
MAP@100 at the same time.

Keep the M1126 loss shape as the current best training direction:

- retain listwise candidate-set pressure;
- retain top-rank preserve as a constraint;
- do not promote fixed alpha from train selection;
- validate the loss shape on broader surfaces before calling it a default.

## Next Step

Run the same loss shape on a broader surface, preferably shared8 first:

- keep `LISTWISE_RANK_WEIGHT=0.50`;
- keep `TOP_RANK_PRESERVE_WEIGHT=0.25`;
- keep semantic expansion disabled for the first broader validation;
- export ranking JSONL and replay the same fixed-alpha/fine-grid/tradeoff
  audits.

Stop if M1126-style gains collapse outside shared5 or if broader validation
only improves recall while spending MRR/NDCG/MAP.
