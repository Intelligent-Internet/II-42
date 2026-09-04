# ii42 M1127 Shared8 Constrained Listwise Top-Rank Verdict

M1127 replays the M1126 loss shape on a broader shared8 surface.

The goal is not to tune alpha.  The goal is to test whether the structural
signal from M1126 survives when adding `trec-covid`, `cqadupstack`, and
`webis-touche2020`.

## Run Status

Remote run:
`/home/huoju/leask/runs/ii42-m1127-shared8-listwise050-toprank-export-v1`

Local synced run:
`/Volumes/Betty/Tmp/ii42-m1000/m1127-shared8-listwise050-toprank-export-v1`

The run completed normally and wrote final JSON, checkpoint, dataset partials,
and ranking exports for all 8 datasets.

## Training Shape

| Epoch | Loss | Rank | Listwise | Top-Rank | Fanout | Background |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 2.958060 | 0.662464 | 1.342095 | 0.617822 | 31.826106 | 0.391751 |
| 2 | 1.832729 | 0.554383 | 0.289242 | 0.515560 | 21.804969 | 0.190580 |
| 3 | 0.935075 | 0.386849 | 0.066464 | 0.362312 | 8.891197 | 0.066704 |
| 4 | 0.474937 | 0.241487 | 0.054621 | 0.232028 | 2.836308 | 0.036078 |

The loss remains stable on shared8.  The top-rank term remains active through
the final epoch and does not produce NaN or collapse.

## Heldout Macro

Fine-grid verdict remains `do_not_promote_fixed_alpha`, because train-selected
alpha is `0.75` while heldout-best alpha is `0.60`.

| Alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 0.00 | 0.624251 | 0.623738 | 0.532395 | 0.388653 |
| 0.25 | 0.651693 | 0.652791 | 0.558317 | 0.412922 |
| 0.50 | 0.664167 | 0.654200 | 0.562397 | 0.422482 |
| 0.60 | 0.665976 | 0.649756 | 0.558740 | 0.426991 |
| 0.75 | 0.668550 | 0.641893 | 0.554247 | 0.424338 |

Best alpha=0.60 improves over lexical alpha=0.00:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.041725 | +0.026019 | +0.026344 | +0.038338 |

## Row-Level Risk

M1127 is macro-positive, but not row-clean.  At alpha=0.60 versus lexical:

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | -0.008519 | -0.039267 | -0.005848 |
| `cqadupstack` | -0.003167 | -0.010588 | -0.010379 | -0.006105 |
| `fiqa` | +0.098413 | +0.077571 | +0.054620 | +0.063475 |
| `nfcorpus` | +0.020568 | +0.072792 | +0.062721 | +0.029641 |
| `scidocs` | +0.033333 | -0.012036 | -0.025756 | -0.003107 |
| `scifact` | +0.056667 | +0.007817 | +0.000507 | -0.007835 |
| `trec-covid` | +0.006347 | -0.038889 | +0.054919 | +0.070522 |
| `webis-touche2020` | +0.121639 | +0.120000 | +0.113388 | +0.165962 |

The route is positive on macro and very strong on `fiqa`, `nfcorpus`, and
`webis-touche2020`, but it still spends row-level top-rank quality on
`arguana`, `cqadupstack`, and `scidocs`.

## Query Tradeoff

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| 0.00 -> 0.25 | +0.026147 | +0.027013 | +0.020748 | +0.019590 |
| 0.25 -> 0.30 | +0.000496 | +0.001915 | -0.001329 | -0.000454 |
| 0.30 -> 0.50 | +0.010264 | -0.003479 | +0.002853 | +0.004687 |
| 0.50 -> 0.75 | +0.002938 | -0.010493 | -0.009733 | -0.002471 |

The first pressure step remains clean.  Later pressure is still too expensive.

## Verdict

M1127 preserves the M1126 structural signal at shared8 macro level.  It is a
real positive route, not just a shared5 artifact.

It is not yet a deployable/default route:

- fixed alpha selection remains unstable;
- alpha=0.75 increases recall but spends MRR/NDCG;
- several rows regress versus lexical at the selected best macro alpha.

## Next Step

Run a shared8 same-surface control without top-rank preserve:

- `LISTWISE_RANK_WEIGHT=0.50`
- `TOP_RANK_PRESERVE_WEIGHT=0.0`
- same shared8 data, seed, ranking export, and audit stack.

This is the minimal scientific control needed before deeper changes.  If the
control is worse on MAP/NDCG/MRR, M1126/M1127 top-rank preserve should stay as
the current best loss shape.  If the control is equal or better, the top-rank
term is only a shared5 artifact and should be redesigned.
