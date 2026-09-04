# M1255 Corrected Low-Reserve Native Replay

## Goal

M1250 reported `top8_low0_s1` as the best safe native replay, but a later code
review found that `low_reserve=0` still added one low-tail atom: the helper
appended an atom before checking the `top_n` limit.

M1255 fixes that off-by-one and reruns the low-reserve frontier:

- true `reserve0`
- `reserve1`
- `reserve2`
- `reserve4`
- `reserve8`

This is a source-construction check, not another threshold or guard.

## Runs

Smoke:

- `runs/m1255_corrected_low_reserve_native_smoke_v1/`
- Datasets:
  `cqadupstack`, `scidocs`, `webis-touche2020`

Full `shared15`:

- JSON:
  `runs/m1255_corrected_low_reserve_native_v1/m1255_corrected_low_reserve_native.json`
- Markdown:
  `runs/m1255_corrected_low_reserve_native_v1/m1255_corrected_low_reserve_native.md`
- Query count: `1342`

## Full Shared15 Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1255_reserve1_s1` | 8.958 | 0 | +0.001562 | +0.003536 | +0.003268 | +0.002995 | +0.000092 | +0.031034 |
| `m1255_reserve0_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |
| `m1255_reserve2_s1` | 9.928 | 1 | +0.001448 | +0.003477 | +0.003271 | +0.002472 | -0.000156 | +0.025088 |
| `m1255_reserve4_s1` | 11.806 | 1 | +0.001164 | +0.003140 | +0.003094 | +0.002636 | -0.000132 | +0.023259 |
| `m1255_reserve8_s1` | 15.013 | 1 | +0.000211 | +0.003365 | +0.003384 | +0.003660 | -0.000125 | +0.021994 |
| `m1255_reserve0_s0.5` | 7.975 | 0 | +0.001448 | +0.001783 | +0.001633 | +0.001728 | +0.000236 | +0.019547 |
| `m1255_reserve8_s0.5` | 15.013 | 0 | +0.001557 | +0.002029 | +0.001422 | +0.001025 | +0.000124 | +0.018888 |
| `m1255_reserve2_s0.5` | 9.928 | 0 | +0.000736 | +0.002339 | +0.001981 | +0.001152 | +0.000199 | +0.017163 |
| `m1255_reserve1_s0.5` | 8.958 | 0 | +0.000460 | +0.002249 | +0.001935 | +0.001634 | +0.000036 | +0.016222 |
| `m1255_reserve4_s0.5` | 11.806 | 0 | +0.000642 | +0.002116 | +0.001711 | +0.000963 | +0.000201 | +0.015111 |

## Interpretation

The old M1250 best result was not invalid, but it was mislabeled.

After fixing the off-by-one:

- true `reserve0_s1` matches the pure all-actions signed-sum route from M1251
- `reserve1_s1` reproduces the old M1250 best row
- `reserve1_s1` is the best safe macro frontier
- larger reserves at scale `1.0` introduce CUB loss
- scale `0.5` remains safe but weaker

So the source-construction lesson is precise:

> exactly one low-tail reserve atom is useful; larger reserves spend support
> budget and create CUB risk.

## Decision

Keep `reserve1_s1` as the best current macro-safe native policy candidate, but
do not promote it as default yet.

The gain over pure signed-sum is real but small:

| Comparison | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `reserve1_s1 - reserve0_s1` | +0.000163 | +0.000200 | -0.000357 | -0.000132 | -0.000022 | +0.000414 |

This is not enough to call solved, but it is a real source-construction
improvement and should be preserved.

## Next Step

Do not add another output guard.

The next useful work is:

1. Audit `reserve1_s1` row and per-dataset stability against `reserve0_s1`.
2. Identify when the single low-tail atom helps versus hurts.
3. If the row anatomy is observable, convert that into a source/objective
   feature.
4. If it is not observable, keep `reserve1_s1` as a macro-positive candidate
   and move to training objective design using the low-tail reserve as a
   supervised target.

This follows the broader M1000+ conclusion: useful added atoms exist, but
qrels-free safe selection remains the bottleneck.
