# ii42 M1121-M1123 Alpha Pressure and Gate Verdict

## Question

M1118/M1120 showed a real atom signal, but also showed a bad selection
surface: train split prefers high atom alpha (`0.75`), while heldout top-rank
quality prefers low alpha (`0.25`). M1121-M1123 tested whether this could be
fixed by lower training pressure or a deployable query-level alpha gate.

## Evidence

### M1121 Low-Pressure Training

M1121 used fewer rank epochs and a lower learning rate:

- `RANK_EPOCHS=4`
- `LEARNING_RATE=0.0007`
- semantic hard negatives disabled

It completed on spark-1 and replayed cleanly on shared5.

| Run | Train-selected alpha | Heldout-best alpha | Verdict |
| --- | ---: | ---: | --- |
| M1118 | 0.75 | 0.25 | `do_not_promote_fixed_alpha` |
| M1121 | 0.75 | 0.30 | `do_not_promote_fixed_alpha` |

M1121 improves Recall at atom-weighted alphas, but does not beat M1118 on the
best top-rank operating point.

| Run | Best alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1118 | 0.25 | 0.688257 | 0.566161 | 0.499339 | 0.395365 |
| M1121 | 0.30 | 0.703565 | 0.548662 | 0.494046 | 0.391612 |

Conclusion: lower pressure changes the Recall/top-rank tradeoff, but it does
not solve alpha selection reliability.

### M1122 Query Tradeoff Audit

Most alpha increases do not create clean gains. They mostly create top-rank
damage without Recall gains.

M1121 heldout:

| Transition | Clean recall gain | Mixed gain/damage | Pure top damage | Top gain no recall |
| --- | ---: | ---: | ---: | ---: |
| 0.00 -> 0.25 | 8 | 1 | 37 | 24 |
| 0.25 -> 0.30 | 1 | 0 | 24 | 18 |
| 0.30 -> 0.50 | 4 | 0 | 41 | 18 |
| 0.50 -> 0.75 | 2 | 1 | 53 | 16 |

The useful movements are sparse. This explains why fixed alpha is unstable:
the same pressure that helps a small group of queries damages a larger group.

### M1123 Deployable Gate Separability

M1123 trained a logistic gate using only inference-time geometry features:

- lex/atom correlation
- lex/atom topK overlap
- cross-ranks of top lexical/atom documents
- top score gaps
- candidate count

It evaluated leave-dataset-out and cross-run transfer.

| Train -> Test | Transition | LODO AUC | Cross AUC | Verdict |
| --- | --- | ---: | ---: | --- |
| M1118 -> M1121 | 0.00 -> 0.25 | 0.562593 | 0.558388 | fail |
| M1118 -> M1121 | 0.30 -> 0.50 | 0.584880 | 0.453437 | fail |
| M1118 -> M1121 | 0.50 -> 0.75 | 0.370833 | 0.473251 | fail |
| M1121 -> M1118 | 0.00 -> 0.25 | 0.594643 | 0.552432 | fail |
| M1121 -> M1118 | 0.30 -> 0.50 | 0.569001 | 0.517857 | fail |
| M1121 -> M1118 | 0.50 -> 0.75 | 0.613001 | 0.466667 | fail |

Conclusion: a deployable alpha gate is not supported by current feature
geometry. It should not become the next branch.

## Verdict

Stop the fixed-alpha and alpha-gate sub-route for now.

Kept facts:

1. Atom signal is real: low/mid alpha consistently beats lexical on at least
   some macro metrics.
2. Atom pressure is not globally safe: most pressure increments create pure
   top-rank damage.
3. Lower pressure is a tradeoff, not a solution.
4. Query-level safe movement is not separable with current deployable geometry
   features.

Rejected next steps:

1. Do not run more alpha grids.
2. Do not train a geometry-only alpha gate.
3. Do not promote M1121 as a new default.
4. Do not use train split to select atom alpha.

## Next Useful Direction

The next target is the atom training objective, not the scorer gate.

The current evidence says the atom head is learning useful evidence but with
poor calibration against lexical/top-rank preservation. A meaningful next
experiment should train the atom signal under explicit top-rank preservation
constraints:

1. Reward atom evidence only when it moves positives without displacing
   already-good lexical top ranks.
2. Penalize pure top-damage transitions directly.
3. Select checkpoints by heldout-like top-rank metrics, not train rank loss.
4. Keep shared5 as the first expansion gate before any broader run.

The next branch should therefore be a constrained atom utility objective, not
another alpha/pressure selector.
