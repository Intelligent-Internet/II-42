# M1266 Raw-Uniform Impact Interpolation

## Question

M1264 showed that `uniform_l1` improves macro over raw but loses average
query-level score.  M1265 rejected a deployable uniform/raw query-time gate from
current source-geometry features.

M1266 tests a narrower possibility:

> Is there a single global convex interpolation between raw signed-sum impacts
> and uniform L1 impacts that improves score geometry without needing a query
> selector?

This is still a score-geometry test, not a training run.

## Smoke Run

- Script: `scripts/replay_m1266_raw_uniform_interpolation.py`
- Output root: `runs/m1266_raw_uniform_interpolation_smoke_v1/`
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Mixes: `0, 0.1, 0.25, 0.5, 0.75, 1.0`

These are hard rows that previously exposed rank harm.  Passing this smoke gate
requires more than macro gain: the mix should not lose query-level mean score
against raw.

## Smoke Frontier

| Variant | Mix | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score | MeanScoreVsRaw | WinsVsRaw | LossesVsRaw |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1266_mix0` | 0.000 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 | +0.000000 | 0 | 0 |
| `m1266_mix0p1` | 0.100 | 1 | +0.001331 | +0.000454 | -0.000321 | +0.000678 | +0.000031 | +0.005551 | -0.001753 | 10 | 12 |
| `m1266_mix0p25` | 0.250 | 1 | +0.001133 | +0.000425 | -0.000281 | +0.000879 | +0.000031 | +0.005355 | -0.003571 | 18 | 22 |
| `m1266_mix0p5` | 0.500 | 1 | +0.001133 | +0.000463 | -0.000239 | +0.000678 | +0.000031 | +0.005573 | -0.006433 | 28 | 30 |
| `m1266_mix0p75` | 0.750 | 0 | +0.001133 | +0.000852 | +0.000008 | +0.000812 | +0.000031 | +0.009890 | -0.004313 | 35 | 32 |
| `m1266_mix1` | 1.000 | 0 | +0.001442 | +0.000714 | +0.000313 | +0.000790 | +0.000031 | +0.011588 | -0.005591 | 37 | 41 |

## Interpretation

The macro frontier exists, but it is not query-stable.

The two strongest macro variants are `mix0p75` and `mix1`.  Both make hard-row
macro metrics all-positive.  But both have negative mean query-level score
relative to raw:

- `mix0p75`: `-0.004313`
- `mix1`: `-0.005591`

This matches M1264/M1265:

- equalizing magnitudes helps Recall/CUB and some macro scores;
- it also introduces rank-sensitive query losses;
- the loss is not solved by a single global interpolation.

## Decision

Stop the manual raw-uniform interpolation branch.

Do not run full `shared15` for M1266.  The hard-row smoke already fails the
required stability gate.  Running full would only reconfirm that macro can be
made positive while query-level rank harm remains.

Retain the evidence:

- raw/uniform endpoints are useful anchors;
- score geometry matters independently of atom admission;
- manual transforms are not enough.

Next valid question:

> Does the raw/uniform anchor family contain enough oracle capacity to justify
> training a deeper rank-sensitive calibrator?

If query-oracle choice among raw/uniform/intermediate mixes has only small
additional upside, stop this anchor family.  If oracle upside is large but
currently unobservable, then the next branch must be a trained objective with
rank-sensitive supervision, not another hand transform or selector.
