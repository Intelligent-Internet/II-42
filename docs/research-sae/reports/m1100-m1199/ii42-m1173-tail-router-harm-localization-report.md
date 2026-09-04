# M1173 Tail Router Harm Localization

M1173 localizes the row-floor harm from the best M1172 oracle class tail router:
switch `clean_relevant_entry`, `rank_gain_no_top100_recall`, and
`relevant_swap` rows from direct_protect to tail_full.

## Inputs

- Policy: M1172 best-any oracle class router.
- Tail classes:
  `clean_relevant_entry`, `rank_gain_no_top100_recall`, `relevant_swap`.
- Accepted selected rows: 99.
- Output:
  `runs/m1173_tail_router_harm_localization_v1/tail_router_harm_localization.json`.

## Dataset Summary

| Dataset | Count | Harmed | Classes | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 1 | 0 | `{'rank_gain_no_top100_recall': 1}` | +0.000000 | +0.000000 | +0.087963 | +0.315465 | +0.125000 |
| `dbpedia-entity` | 36 | 14 | `{'clean_relevant_entry': 13, 'rank_gain_no_top100_recall': 9, 'relevant_swap': 14}` | +0.000000 | +0.000000 | +0.012795 | +0.032441 | +0.043651 |
| `msmarco` | 26 | 18 | `{'clean_relevant_entry': 10, 'rank_gain_no_top100_recall': 6, 'relevant_swap': 10}` | +0.000000 | +0.000000 | +0.006260 | -0.032988 | +0.000000 |
| `nfcorpus` | 4 | 2 | `{'clean_relevant_entry': 2, 'relevant_swap': 2}` | +0.000000 | +0.000000 | -0.007345 | -0.018341 | +0.000000 |
| `trec-covid` | 32 | 16 | `{'clean_relevant_entry': 1, 'relevant_swap': 31}` | +0.000049 | +0.000047 | +0.001598 | +0.038950 | +0.059896 |

## Interpretation

The oracle class tail-router harm is not uniformly distributed.  It is mainly
dataset-local:

- `msmarco` is the dominant NDCG harm source despite positive MAP.
- `nfcorpus` is a small but clean MAP/NDCG harm source.
- `dbpedia-entity`, `trec-covid`, and `arguana` are net positive on the same
  oracle class policy.

This means tail_full is not a globally stable runtime route, but it is also
not useless.  It carries useful ranking movement on several surfaces and fails
because score/rank geometry changes differently by dataset.

## Decision

Do not deploy raw tail_full routing globally or via M1166 class alone.

The useful next direction is not another blind router.  It is a training or
calibration branch that uses M1137 tail_full as a teacher/reference while
penalizing dataset-local NDCG/MAP harm:

1. Treat tail_full gains as candidate positive supervision.
2. Add per-query rank-preservation/harm penalties for msmarco-like failures.
3. Evaluate with row-floor gates before any runtime route.
4. If training cannot internalize tail gains without msmarco/nfcorpus harm,
   keep M1137 as analysis-only and return to unified-surface training.
