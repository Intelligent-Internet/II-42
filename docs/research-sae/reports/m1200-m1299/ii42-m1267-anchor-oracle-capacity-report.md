# M1267 Raw-Uniform Anchor Oracle Capacity

## Question

M1266 stopped manual global interpolation because no fixed raw/uniform mix
passed hard-row query-level stability.  M1267 asks a different question:

> Does the raw/uniform anchor family contain enough oracle capacity to justify
> training a rank-sensitive impact calibrator?

This is explicitly a qrels-oracle capacity audit.  It is not deployable.

## Runs

Smoke:

- Output root: `runs/m1267_anchor_oracle_capacity_smoke_v1/`
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`

Full:

- Script: `scripts/audit_m1267_anchor_oracle_capacity.py`
- Output root: `runs/m1267_anchor_oracle_capacity_v1/`
- JSON: `runs/m1267_anchor_oracle_capacity_v1/m1267_anchor_oracle_capacity.json`
- Markdown: `runs/m1267_anchor_oracle_capacity_v1/m1267_anchor_oracle_capacity.md`
- Surface: full `shared15`
- Query count: `1342`
- Mixes: `0, 0.25, 0.5, 0.75, 1.0`

## Full Shared15 Result

Raw score from M1266/M1267:

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1266_mix0` | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |

Oracle policies:

| Policy | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score | MeanScoreVsRaw | WinsVsRaw | LossesVsRaw |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_any_mix` | 0 | +0.002181 | +0.004075 | +0.004255 | +0.003787 | +0.000549 | +0.039762 | +0.008874 | 207 | 9 |
| `oracle_nonraw_if_better` | 0 | +0.002181 | +0.004075 | +0.004255 | +0.003787 | +0.000549 | +0.039762 | +0.008874 | 207 | 9 |
| `oracle_safe_nonraw_if_better` | 0 | +0.002188 | +0.004083 | +0.004194 | +0.003784 | +0.000436 | +0.039580 | +0.008960 | 203 | 0 |

The important policy is `oracle_safe_nonraw_if_better`: it only switches away
from raw when the candidate mix is better than raw and has no metric-level loss
against raw for that query.

Choice counts:

- `m1266_mix0`: `1139`
- `m1266_mix0p25`: `46`
- `m1266_mix0p5`: `44`
- `m1266_mix0p75`: `40`
- `m1266_mix1`: `73`

Only `203 / 1342` queries need non-raw calibration, but those switches create a
large score lift with zero qrels-oracle losses against raw.

## Interpretation

This is the first strong result after M1265.

The manual policies fail:

- global uniform is macro-positive but query-unstable;
- global interpolation does not fix that;
- qrels-free thresholding over current source geometry does not find a stable
  split.

But the anchor family is not dead:

- a sparse subset of queries benefits from non-raw impact geometry;
- the safe oracle can choose those queries without any metric loss against raw;
- the capacity is large enough to justify a trained impact calibrator.

The bottleneck is now precise:

> not atom selection, not raw/uniform endpoint quality, but learning the
> rank-sensitive conditions under which impact magnitudes should be flattened.

## Decision

Promote the next branch to a small trained calibrator smoke, not another manual
transform or threshold.

Minimum requirements for M1268:

1. Target: imitate `oracle_safe_nonraw_if_better` or its per-query best mix.
2. Input: qrels-free source geometry plus train-fold atom prior features.
3. Model: simple enough to audit first, e.g. multinomial logistic or ordinal
   regression over mix classes.
4. Validation: leave-one-dataset-out, starting with hard-row smoke.
5. Replay gate:
   - compare against raw, uniform, and safe oracle;
   - require no macro metric regression against raw on smoke;
   - require row-level loss analysis before full shared15.

If M1268 cannot recover meaningful safe-oracle lift on hard rows, stop this
anchor-family training branch and return to source/objective redesign.
