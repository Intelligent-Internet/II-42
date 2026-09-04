# ii42 M1125 Listwise Weight Sweep Verdict

## Scope

M1125 tested a narrow listwise-weight sweep after M1124 showed a positive
signal:

- M1125a: `LISTWISE_RANK_WEIGHT=0.10`
- M1124: `LISTWISE_RANK_WEIGHT=0.25`
- M1125b: `LISTWISE_RANK_WEIGHT=0.50`

All other settings were fixed:

- shared5 datasets
- semantic hard negatives disabled
- `RANK_EPOCHS=4`
- `LEARNING_RATE=0.0007`
- `LISTWISE_TEMPERATURE=0.07`

## Heldout Best Points

| Run | Weight | Best alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1118 baseline | 0.00 | 0.25 | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| M1125a | 0.10 | 0.15 | 0.684173 | 0.556086 | 0.494349 | 0.388732 |
| M1124 | 0.25 | 0.55 | 0.719679 | 0.549041 | 0.496466 | 0.394045 |
| M1125b | 0.50 | 0.50 | 0.715489 | 0.562618 | 0.498005 | 0.391700 |

## Interpretation

M1125a is too weak. It underperforms the baseline and should be rejected.

M1124 and M1125b show that listwise loss is real:

- M1124 gives the best Recall/MAP balance.
- M1125b gives the best top-rank balance among listwise runs.
- Both retain much more Recall than M1118 while staying close on MRR/NDCG.

However, neither beats M1118 on MAP. This means listwise batch selection helps
reduce pure top damage, but it does not directly optimize average precision.

## Query Tradeoff Evidence

For `0.30 -> 0.50`:

| Run | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Clean gain | Pure damage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1118 | +0.011444 | -0.020333 | -0.012483 | -0.012221 | 4 | 42 |
| M1124 | +0.009637 | -0.009577 | -0.002560 | -0.001996 | 8 | 36 |
| M1125b | +0.017174 | +0.002951 | -0.003505 | +0.003671 | 6 | 36 |

M1125b is the strongest evidence that listwise training changes the atom
pressure shape in the right direction: mid-alpha pressure can now gain Recall
and MAP without the severe MRR collapse seen in M1118.

## Verdict

Keep the listwise objective as a real positive route.

Do not continue blind weight sweeps. The curve is informative enough:

- `0.10` is too weak.
- `0.25` improves Recall/MAP balance.
- `0.50` improves top-rank balance.

The missing piece is not another weight value. The missing piece is a
top-rank/MAP preservation term or checkpoint selection surface that directly
penalizes precision loss.

## Next Step

Design M1126 as a constrained listwise objective:

1. Keep batch listwise loss.
2. Add a top-rank preservation penalty against the lexical baseline on train
   candidates.
3. Select checkpoints using heldout-like MAP/NDCG/MRR, not train rank loss.
4. Keep the first gate on shared5.

Promote only if it beats M1118 MAP/NDCG while preserving most of the M1124 or
M1125b Recall gain.
