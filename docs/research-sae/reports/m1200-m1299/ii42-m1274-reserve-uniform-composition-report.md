# M1274 Reserve/Uniform Composition

M1274 tests one structural combination instead of another gate:

- source construction from M1255: corrected `reserve1`
- score geometry from M1263/M1264: `uniform_l1`

The question is whether the low-tail reserve and uniform impact geometry
compose into a better hard-row native policy.

## Run

- Surface: hard-row smoke
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`
- Output JSON:
  `runs/m1274_reserve_uniform_composition_smoke_v1/m1274_reserve_uniform_composition.json`
- Output detail:
  `runs/m1274_reserve_uniform_composition_smoke_v1/m1274_reserve_uniform_composition.md`

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1274_reserve0_uniform_l1_s1` | 4.602 | 0 | +0.001442 | +0.000714 | +0.000313 | +0.000790 | +0.000031 | +0.011588 |
| `m1274_reserve1_uniform_l1_s1` | 4.683 | 0 | +0.001143 | +0.000607 | +0.000313 | +0.000790 | +0.000031 | +0.009773 |
| `m1274_reserve0_raw_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1274_reserve1_raw_s1` | 4.683 | 1 | +0.001331 | +0.000487 | -0.000286 | +0.000607 | +0.000031 | +0.005928 |

## Interpretation

The result is negative for composition:

- `uniform_l1` fixes the hard-row NDCG regression in both reserve settings.
- `reserve1` does not improve the uniform shape.
- `reserve1_uniform_l1` is safe, but weaker than `reserve0_uniform_l1`.
- `reserve1_raw` is also weaker than `reserve0_raw`.

So the two positive signals do not combine directly.  The low-tail reserve is
not a general-purpose companion to uniform impact geometry.

## Decision

Do not scale M1274 to full `shared15`; M1263 already covers the stronger
component (`reserve0_uniform_l1`) on the full surface, and M1264 already shows
its row-level instability.

This reinforces the current route decision:

1. fixed transform composition is not enough;
2. post-hoc query selection is not enough;
3. the next useful move must change the training/source objective so that
   target/harm separation is produced earlier.
