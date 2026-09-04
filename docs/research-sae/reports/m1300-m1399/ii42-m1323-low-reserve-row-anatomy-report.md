# M1323 Low-Reserve Row Anatomy

## Question

M1255 found that `reserve1_s1` was the best macro-positive low-reserve source
on full shared15. M1323 checks whether that source is row-safe on the current
risk7 surface before using it as a new objective target.

## Command

```bash
python3 scripts/audit_m1323_low_reserve_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --reserves 0,1,2,4,8 \
  --scales 0.5,1.0 \
  --output-root runs/m1323_low_reserve_row_anatomy_risk7_v1
```

Elapsed: `127.17s`.

## Result

| Variant | Selected | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1323_reserve0_s1` | 7.816 | 1 | 10 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | +0.005682 |
| `m1323_reserve1_s1` | 8.668 | 1 | 8 | +0.001960 | +0.001352 | +0.001683 | -0.001034 | +0.000071 | +0.004886 |
| `m1323_reserve4_s1` | 10.673 | 1 | 10 | +0.001020 | +0.001141 | +0.001594 | -0.001037 | +0.000614 | -0.000121 |
| `m1323_reserve8_s1` | 11.850 | 1 | 10 | +0.001076 | +0.001087 | +0.001466 | -0.001085 | +0.000141 | -0.001304 |
| `m1323_reserve0_s0.5` | 7.816 | 1 | 9 | +0.000904 | +0.000604 | +0.000745 | -0.001142 | +0.000483 | -0.005400 |

## Interpretation

Low-reserve is not a standalone row-safe source on risk7.

The useful detail is narrower:

- `reserve1_s1` improves candidate support and reduces dataset negatives
  compared with `reserve0_s1` (`10 -> 8`).
- It helps some `cqadupstack` ranking symptoms.
- It does not fix the macro MRR loss.
- Larger reserves spend support budget and do not solve ranking harm.

This means the full shared15 macro-positive M1255 result should be preserved as
source-construction evidence, but not promoted as a deployable policy.

## Decision

Do not promote `reserve1_s1` by itself.

Use it only as a candidate-support component or objective target. The next
bounded check is to combine `low_reserve1` as candidate stage with the M1321
lower-pressure ranking stage.
