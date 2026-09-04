# M813 Generated-Bundle Oracle Ceiling

M813 measures whether the current generated-bundle pool contains a
deployable safe oracle.  If the safe oracle is weak or absent, more
selector tuning cannot be the main breakthrough path.

## Results

| Surface | Split | Mode | Gate | Clean | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Utility | Pos Rows | Risky Pos Rows |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| original | dev | safe_positive | 1 | 1 | 42 | +0.000319 | +0.000217 | +0.000281 | +0.000054 | +0.000000 | +0.000620 | 178 | 6 |
| original | dev | strict_positive | 1 | 1 | 41 | +0.000319 | +0.000217 | +0.000281 | +0.000050 | +0.000000 | +0.000618 | 178 | 6 |
| original | dev | unrestricted | 1 | 0 | 200 | +0.000310 | +0.000121 | +0.000281 | +0.000074 | +0.000000 | +0.000523 | 178 | 6 |
| original | test | safe_positive | 1 | 1 | 41 | +0.000382 | +0.001376 | +0.000107 | +0.000033 | +0.000000 | +0.001795 | 187 | 4 |
| original | test | strict_positive | 1 | 1 | 41 | +0.000382 | +0.001376 | +0.000107 | +0.000033 | +0.000000 | +0.001795 | 187 | 4 |
| original | test | unrestricted | 1 | 0 | 190 | +0.000297 | +0.001108 | +0.000107 | +0.000041 | +0.000000 | +0.001447 | 187 | 4 |
| seed7642 | dev | safe_positive | 1 | 1 | 36 | +0.000233 | +0.000144 | +0.000186 | +0.000117 | +0.000000 | +0.000472 | 166 | 14 |
| seed7642 | dev | strict_positive | 1 | 1 | 36 | +0.000233 | +0.000144 | +0.000186 | +0.000117 | +0.000000 | +0.000472 | 166 | 14 |
| seed7642 | dev | unrestricted | 1 | 0 | 193 | +0.000219 | +0.000139 | +0.000186 | +0.000129 | +0.000000 | +0.000460 | 166 | 14 |
| seed7642 | test | safe_positive | 1 | 1 | 49 | +0.000358 | +0.000714 | +0.000141 | +0.000087 | +0.000000 | +0.001143 | 210 | 4 |
| seed7642 | test | strict_positive | 1 | 1 | 49 | +0.000358 | +0.000714 | +0.000141 | +0.000087 | +0.000000 | +0.001143 | 210 | 4 |
| seed7642 | test | unrestricted | 1 | 0 | 190 | +0.000293 | +0.000681 | +0.000141 | +0.000095 | +0.000000 | +0.001049 | 210 | 4 |
| seed7643 | dev | safe_positive | 1 | 1 | 44 | +0.000745 | +0.000689 | +0.000401 | +0.000032 | +0.000000 | +0.001530 | 191 | 12 |
| seed7643 | dev | strict_positive | 1 | 1 | 44 | +0.000745 | +0.000689 | +0.000401 | +0.000032 | +0.000000 | +0.001530 | 191 | 12 |
| seed7643 | dev | unrestricted | 1 | 0 | 198 | +0.000719 | +0.000742 | +0.000401 | +0.000027 | +0.000000 | +0.001555 | 191 | 12 |
| seed7643 | test | safe_positive | 1 | 1 | 40 | +0.000340 | +0.000348 | +0.000066 | +0.000201 | +0.000000 | +0.000801 | 190 | 9 |
| seed7643 | test | strict_positive | 1 | 1 | 40 | +0.000340 | +0.000348 | +0.000066 | +0.000201 | +0.000000 | +0.000801 | 190 | 9 |
| seed7643 | test | unrestricted | 0 | 0 | 196 | +0.000240 | -0.000046 | +0.000066 | +0.000217 | +0.000000 | +0.000316 | 190 | 9 |

## Decision

M813 found a clean useful safe-oracle ceiling. The proposal pool has enough signal; M812 failed because supervision/modeling is not extracting the safe subset.
