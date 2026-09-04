# M1316 Two-Tier Value Native Replay

## Question

M1315 showed that the M1314 fill source has a CUB/NDCG frontier:

- smaller fill budgets preserve NDCG but lose candidate upper bound;
- fill4 restores CUB but keeps a tiny NDCG regression.

M1316 fixes the fill4 membership and changes only the value pressure of fill
atoms. If lower fill values keep CUB while removing NDCG harm, the current
branch can continue as a single-pass source construction.

## Run

```bash
python3 scripts/replay_m1316_two_tier_value_native.py \
  --datasets cqadupstack,scidocs,webis-touche2020 \
  --fill-to-values 4 \
  --fill-value-scales 0.25,0.5,0.75,1.0 \
  --prefix-geometries uniform_l1,signed_sum \
  --output-root runs/m1316_two_tier_value_native_smoke_v1
```

Artifacts:

- Script: `scripts/replay_m1316_two_tier_value_native.py`
- JSON: `runs/m1316_two_tier_value_native_smoke_v1/m1316_native.json`
- Markdown: `runs/m1316_two_tier_value_native_smoke_v1/m1316_native.md`

Surface:

- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- queries: `249`
- validation: leave-one-dataset-out

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `uniform_l1_fill4_fs1.0` | 3.422 | 1 | +0.002030 | +0.000708 | -0.000054 | +0.001153 | +0.000015 | 0.013944 |
| `signed_sum_fill4_fs1.0` | 3.422 | 1 | +0.002030 | +0.000598 | -0.000121 | +0.001354 | +0.000015 | 0.013220 |
| `source_abs_signed_sum_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | 0.006020 |
| `uniform_l1_fill4_fs0.5` | 3.422 | 1 | +0.001847 | +0.000739 | +0.000395 | +0.001364 | -0.000803 | -0.005911 |
| `signed_sum_fill4_fs0.5` | 3.422 | 1 | +0.001847 | +0.000584 | +0.000309 | +0.001565 | -0.000803 | -0.006148 |
| `uniform_l1_fill4_fs0.25` | 3.422 | 1 | +0.001847 | +0.000569 | +0.000480 | +0.000695 | -0.000788 | -0.007193 |

Against the M1314 reference, lowering fill values produces the expected rank
effect but breaks support:

| Variant | dRecall vs ref | dNDCG vs ref | dCUB vs ref |
| --- | ---: | ---: | ---: |
| `uniform_l1_fill4_fs0.25` | -0.000183 | +0.000534 | -0.000803 |
| `uniform_l1_fill4_fs0.5` | -0.000183 | +0.000449 | -0.000819 |
| `uniform_l1_fill4_fs0.75` | -0.000183 | +0.000014 | -0.000803 |
| `uniform_l1_fill4_fs1.0` | +0.000000 | +0.000000 | +0.000000 |

## Interpretation

M1316 stops the single-pass value-scaling branch.

The failure is useful because it is sharp:

- the same fill4 membership can be rank-safer when fill values are reduced;
- but candidate upper bound immediately drops when fill values are reduced;
- therefore CUB support and NDCG ranking need incompatible value pressure in
  the current single-pass query vector.

This is not another selector failure. It is evidence that the next structure
should decouple candidate generation pressure from ranking pressure while
staying inside the native unified-posting index.

## Decision

Do not scale M1316 to full `shared15`.

Stop these branches:

- fill budget sweeps;
- fill value scaling;
- another post-hoc selector over the same source.

The next valid branch should test a two-stage native replay:

1. candidate stage: use M1314 `uniform_l1_head50_minus_tail_fill4_s1` to
   recover support/CUB;
2. ranking stage: rerank that candidate pool with a rank-safer query, such as
   the lower-value fill variant or clean prefix;
3. compare against single-pass native metrics;
4. require both CUB >= 0 and NDCG >= 0 on the hard-row smoke before broader
   replay.

This is still a unified-posting engineering shape, but it no longer forces a
single query vector to serve candidate generation and final ranking at the same
value scale.
