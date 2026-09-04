# ii42 M1128 Shared8 Listwise-Only Control Verdict

M1128 is the same-surface control for M1127.

It uses the same shared8 data, seed, ranking export, and replay/audit stack as
M1127, but disables top-rank preservation:

- `LISTWISE_RANK_WEIGHT=0.50`
- `LISTWISE_TEMPERATURE=0.07`
- `TOP_RANK_PRESERVE_WEIGHT=0`

## Run Status

Remote run:
`/home/huoju/leask/runs/ii42-m1128-shared8-listwise050-control-export-v1`

Local synced run:
`/Volumes/Betty/Tmp/ii42-m1000/m1128-shared8-listwise050-control-export-v1`

The run completed normally and wrote final JSON, checkpoint, dataset partials,
and ranking exports for all 8 datasets.

## Training Shape

| Epoch | Loss | Rank | Listwise | Top-Rank | Fanout | Background |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 2.795446 | 0.663033 | 1.340430 | 0.000000 | 31.638393 | 0.391813 |
| 2 | 1.711930 | 0.557330 | 0.290287 | 0.000000 | 21.923908 | 0.197833 |
| 3 | 0.912734 | 0.403579 | 0.075576 | 0.000000 | 9.968959 | 0.072165 |
| 4 | 0.449703 | 0.258682 | 0.048300 | 0.000000 | 3.275076 | 0.039430 |

The control is stable and directly comparable to M1127.

## Heldout Macro

| Row | Alpha | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M1127 top-rank preserve best | 0.60 | 0.665976 | 0.649756 | 0.558740 | 0.426991 |
| M1128 listwise-only best | 0.50 | 0.658735 | 0.673472 | 0.578677 | 0.441150 |
| M1127 top-rank preserve selected | 0.75 | 0.668550 | 0.641893 | 0.554247 | 0.424338 |
| M1128 listwise-only selected | 0.75 | 0.674255 | 0.665149 | 0.575095 | 0.440310 |

M1128 best versus M1127 best:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| -0.007241 | +0.023716 | +0.019937 | +0.014158 |

M1128 selected alpha=0.75 versus M1127 selected alpha=0.75:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.005705 | +0.023255 | +0.020849 | +0.015972 |

## Interpretation

This control rejects the broad claim that top-rank preservation is required for
the current listwise route.

M1126 showed top-rank preserve could repair shared5 MAP/NDCG, but M1128 shows
that on shared8 the same constraint suppresses MRR/NDCG/MAP and is not worth
the small recall benefit seen at the respective best-alpha rows.

The retained signal is therefore simpler:

- listwise candidate-set pressure is real;
- top-rank preserve is not the current broader default;
- alpha selection remains unstable and should not be tuned per dataset.

## Query Tradeoff

M1128 improves the early pressure region more cleanly than M1127:

| Transition | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| 0.00 -> 0.25 | +0.014367 | +0.035037 | +0.034860 | +0.030603 |
| 0.25 -> 0.30 | +0.000371 | +0.004476 | +0.002049 | +0.005162 |
| 0.30 -> 0.50 | +0.011664 | +0.010342 | +0.002734 | +0.007301 |
| 0.50 -> 0.75 | +0.015038 | -0.011100 | -0.006942 | -0.005447 |

The useful region is around alpha `0.30` to `0.50`.  Pushing to `0.75`
continues to spend top-rank quality.

## Verdict

Promote M1128 listwise-only as the current best broader training signal.

Do not keep top-rank preserve as the default for broader validation.  It should
remain an ablation result, not the main route.

## Next Step

Run the M1128 loss shape on the next broader surface:

- same listwise-only objective;
- same seed and export stack;
- no semantic expansion yet;
- no per-dataset alpha tuning;
- evaluate with the same fixed-alpha, fine-grid, and query-tradeoff audits.

The acceptance condition for the next surface is not just macro Recall.  It
must preserve the M1128 pattern: MRR/NDCG/MAP should remain positive in the
useful alpha region, and later alpha pressure should be explicitly bounded.
