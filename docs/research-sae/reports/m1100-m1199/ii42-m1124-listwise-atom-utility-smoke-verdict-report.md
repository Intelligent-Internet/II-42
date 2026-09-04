# ii42 M1124 Listwise Atom Utility Smoke Verdict

## Result

M1124 added a small listwise batch loss to the existing atom posting utility
training:

- `LISTWISE_RANK_WEIGHT=0.25`
- `LISTWISE_TEMPERATURE=0.07`
- `RANK_EPOCHS=4`
- `LEARNING_RATE=0.0007`
- semantic hard negatives disabled

The run completed successfully on spark-1 and was replayed locally on shared5.

## Comparison

| Run | Heldout-best alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1118 | 0.25 | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| M1121 | 0.30 | 0.703565 | 0.548662 | 0.494046 | 0.391612 |
| M1124 | 0.55 | 0.719679 | 0.549041 | 0.496466 | 0.394045 |

M1124 does not beat M1118 on top-rank metrics, but it is the best
Recall/MAP balance among the low-pressure variants. Compared with M1121, it
improves Recall, MRR, NDCG, and MAP at the heldout-best operating point.

## Fixed Alpha Replay

M1124 heldout profiles:

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| additive atom 0.25 | 0.703053 | 0.553144 | 0.496855 | 0.392528 |
| additive atom 0.50 | 0.712690 | 0.543900 | 0.494837 | 0.391132 |
| additive atom 0.75 | 0.725942 | 0.546375 | 0.491363 | 0.389887 |

Unlike M1118/M1121, increasing alpha no longer destroys MAP as sharply.

## Query-Level Tradeoff

M1124 increases the number of queries whose best alpha is non-zero:

| Run | best alpha 0.00 | non-zero best alpha |
| --- | ---: | ---: |
| M1118 | 106 | 44 |
| M1121 | 109 | 41 |
| M1124 | 89 | 61 |

For the important `0.30 -> 0.50` transition:

| Run | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | clean gain | pure damage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1118 | +0.011444 | -0.020333 | -0.012483 | -0.012221 | 4 | 42 |
| M1121 | +0.012000 | -0.008456 | -0.008438 | -0.005428 | 4 | 41 |
| M1124 | +0.009637 | -0.009577 | -0.002560 | -0.001996 | 8 | 36 |

This is the strongest positive sign from M1124: listwise loss reduced
top-rank damage and increased clean gain count for mid-alpha pressure.

## Verdict

M1124 is not a default promotion, but it is a route-level positive signal.

Keep:

1. Listwise batch loss directly attacks the observed pure-top-damage failure.
2. It shifts useful operating pressure upward without the same MAP collapse.
3. It improves M1121 on the best Recall/MAP tradeoff.

Do not yet promote:

1. M1118 still has the best MRR/NDCG/MAP point.
2. Train-selected alpha remains `0.75`; train selection is still unreliable.
3. The result is shared5-only and needs a small controlled sweep before any
   broader validation.

## Next Step

Run a narrow listwise-weight sweep, not a broad hyperparameter search:

- M1125a: weight `0.10`
- M1125b: weight `0.50`

Keep all other M1124 settings fixed. Promote only if one variant improves over
M1124 and closes the gap to M1118 top-rank metrics while preserving M1124
Recall gains.
