# M795 Context-Bundle Failure Audit

M795 audits the M794 generated bundle pool before selector training.
It separates proposal-ceiling failure from threshold-selection
failure by evaluating oracle modes over the same generated rows.

## Test Oracle Means

| Score | Mode | Gate | Clean | Applied | dMAP | dNDCG | dRecall | dMRR | dCUB | dO@100 | Utility |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| score_mean_margin | best_utility | 0 | 0 | 202.0 | +0.000255 | +0.000543 | +0.000585 | -0.000101 | -0.000134 | -0.001095 | +0.000711 |
| score_mean_margin | utility_positive | 0 | 0 | 43.3 | +0.000392 | +0.000812 | +0.000585 | +0.000104 | +0.000115 | -0.000209 | +0.001283 |
| score_mean_margin | no_dense_spend_best | 0 | 0 | 191.7 | +0.000203 | +0.000512 | +0.000000 | -0.000101 | +0.000108 | +0.000000 | +0.000748 |
| score_mean_margin | strict_positive | 1 | 1 | 39.3 | +0.000342 | +0.000812 | +0.000000 | +0.000104 | +0.000107 | +0.000000 | +0.001229 |
| margin_bundle | best_utility | 0 | 0 | 202.0 | +0.000273 | +0.000589 | +0.000273 | +0.000099 | -0.000143 | -0.000775 | +0.000811 |
| margin_bundle | utility_positive | 0 | 0 | 42.7 | +0.000357 | +0.000791 | +0.000273 | +0.000099 | +0.000101 | -0.000172 | +0.001218 |
| margin_bundle | no_dense_spend_best | 0 | 0 | 192.0 | +0.000241 | +0.000558 | +0.000000 | +0.000099 | +0.000102 | +0.000000 | +0.000869 |
| margin_bundle | strict_positive | 1 | 1 | 39.7 | +0.000330 | +0.000791 | +0.000000 | +0.000099 | +0.000090 | +0.000000 | +0.001185 |
| conservative_margin | best_utility | 0 | 0 | 202.0 | +0.000260 | +0.000538 | +0.000586 | -0.000101 | -0.000140 | -0.001058 | +0.000709 |
| conservative_margin | utility_positive | 0 | 0 | 43.3 | +0.000397 | +0.000807 | +0.000586 | +0.000104 | +0.000109 | -0.000209 | +0.001280 |
| conservative_margin | no_dense_spend_best | 0 | 0 | 191.7 | +0.000210 | +0.000507 | +0.000000 | -0.000101 | +0.000101 | +0.000000 | +0.000747 |
| conservative_margin | strict_positive | 1 | 1 | 40.0 | +0.000349 | +0.000807 | +0.000000 | +0.000104 | +0.000105 | +0.000000 | +0.001230 |

## Proposal Pool Summary

| Score | Surface | Split | Bundles | Rows | Queries | Strict+ | Utility+ | Dense Spend | Utility p90 | Utility p99 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| score_mean_margin | original | dev | 1089 | 574 | 205 | 52 | 64 | 74 | +0.000083 | +0.005114 |
| score_mean_margin | original | test | 1089 | 515 | 199 | 64 | 75 | 73 | +0.000493 | +0.037741 |
| score_mean_margin | seed7642 | dev | 1109 | 531 | 201 | 51 | 65 | 94 | +0.000205 | +0.006942 |
| score_mean_margin | seed7642 | test | 1109 | 578 | 202 | 72 | 88 | 65 | +0.000657 | +0.013574 |
| score_mean_margin | seed7643 | dev | 1122 | 557 | 206 | 63 | 80 | 69 | +0.000379 | +0.021184 |
| score_mean_margin | seed7643 | test | 1122 | 565 | 205 | 67 | 78 | 74 | +0.000413 | +0.012461 |
| margin_bundle | original | dev | 1112 | 584 | 205 | 65 | 75 | 60 | +0.000187 | +0.008814 |
| margin_bundle | original | test | 1112 | 528 | 199 | 62 | 70 | 70 | +0.000338 | +0.034439 |
| margin_bundle | seed7642 | dev | 1109 | 552 | 201 | 59 | 68 | 79 | +0.000145 | +0.009331 |
| margin_bundle | seed7642 | test | 1109 | 557 | 202 | 69 | 79 | 55 | +0.000349 | +0.012187 |
| margin_bundle | seed7643 | dev | 1141 | 561 | 206 | 64 | 80 | 59 | +0.000337 | +0.024018 |
| margin_bundle | seed7643 | test | 1141 | 580 | 205 | 66 | 77 | 69 | +0.000338 | +0.009937 |
| conservative_margin | original | dev | 1087 | 587 | 205 | 60 | 73 | 75 | +0.000193 | +0.006692 |
| conservative_margin | original | test | 1087 | 500 | 199 | 61 | 70 | 69 | +0.000377 | +0.035364 |
| conservative_margin | seed7642 | dev | 1111 | 537 | 201 | 55 | 67 | 90 | +0.000196 | +0.007645 |
| conservative_margin | seed7642 | test | 1111 | 574 | 202 | 67 | 81 | 67 | +0.000396 | +0.011995 |
| conservative_margin | seed7643 | dev | 1127 | 562 | 206 | 64 | 80 | 67 | +0.000336 | +0.020931 |
| conservative_margin | seed7643 | test | 1127 | 565 | 205 | 57 | 68 | 75 | +0.000213 | +0.012461 |

## Decision

M795 found a clean strict-positive oracle in the generated bundle pool. This route deserves a selector redesign.
