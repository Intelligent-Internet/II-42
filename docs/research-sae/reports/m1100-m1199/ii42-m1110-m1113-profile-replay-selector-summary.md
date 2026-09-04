# ii42 M1110-M1113 Profile Replay and Selector Summary

Date: 2026-07-08

## Status

This stage followed the M1100/M1101 semantic-neighbor stop signal.

M1100 showed semantic-neighbor supervision was real but spent top-rank quality.
M1101 lowered semantic pressure and became safer versus lexical BM25, but lost
all four unified metrics versus M1050. Therefore semantic-neighbor pair
construction is stopped as a standalone route.

## M1110 Fixed Profile Replay

M1110 exported per-query lexical/atom candidate score rows from a M1050-style
run and replayed fixed profiles.

Best heldout MAP profile:

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `lexical` | 0.486992 | 0.392967 | 0.325332 | 0.258925 |
| `additive_atom_0.5` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `lex_residual_atom_0.75` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |

Conclusion:

- `additive_atom_0.5` is the best MAP profile and matches the current
  M1050-style unified scoring shape.
- `lex_residual_atom_0.75` is a real alternate tradeoff with higher Recall/MRR,
  but lower NDCG/MAP.
- Fixed profile search validates the current profile; it does not create a new
  breakthrough.

## M1111 Profile Oracle

M1111 tested query-level choice between:

- base: `additive_atom_0.5`
- alternate: `lex_residual_atom_0.75`

Heldout oracle:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `base` | 0.512545 | 0.401204 | 0.335614 | 0.266129 |
| `alternate` | 0.520115 | 0.402480 | 0.333091 | 0.265431 |
| `oracle` | 0.522862 | 0.423594 | 0.352961 | 0.284616 |

Oracle delta over base:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.010316 | +0.022390 | +0.017346 | +0.018486 |

There is real profile-choice headroom.

## M1112/M1113 Selector Smoke

M1112 one-feature threshold failed. It gave tiny MAP/MRR/NDCG gains but lost
Recall:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| -0.000119 | +0.001515 | +0.000580 | +0.001645 |

M1113 logistic selector also failed:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.006397 | +0.004663 | -0.000915 | +0.000741 |

It captures some Recall/MRR but misses the oracle top-rank improvement and
regresses NDCG.

## Diagnosis

The latest evidence points to a precise bottleneck:

1. Atom signal is useful when combined with lexical evidence.
2. The current fixed profile is already close to the best fixed MAP profile.
3. Query-level profile-choice oracle is large enough to matter.
4. Simple query-side score-distribution features cannot recover that oracle.

This reproduces the older M160 lesson in the current M1050-style surface:
profile choice has headroom, but runtime-safe selector features are too weak.

## Next

Do not continue selector micro-tuning with the same features.

The next useful route should change the observable interface, not the selector
model:

- export richer per-query boundary traces from candidate rows;
- include top-positive/bottom-negative score margins and rank movement features;
- train or calibrate query-local score geometry only after those features show
  heldout separability;
- otherwise keep `additive_atom_0.5` as the frozen M1050-style scoring baseline.
