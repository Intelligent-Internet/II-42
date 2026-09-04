# M1315 Fill-Budget Frontier

## Question

M1314 found a near-pass source construction:

`uniform_l1_head50_minus_tail_fill4_s1`

It improves Recall/MAP/MRR/CUB on the hard-row smoke, but keeps a tiny negative
NDCG delta. M1315 asks whether a smaller or larger fill budget fixes the NDCG
harm without giving back the useful movement.

This is a bounded frontier over the M1314 source. It is not a selector or a
gate sweep.

## Run

```bash
python3 scripts/replay_m1314_head_support_fill_native.py \
  --datasets cqadupstack,scidocs,webis-touche2020 \
  --fill-rules source_abs,head50_minus_tail \
  --prefix-geometries signed_sum,uniform_l1 \
  --fill-to-values 1,2,3,4,5 \
  --output-root runs/m1315_fill_budget_frontier_smoke_v1
```

Artifacts:

- Script: `scripts/replay_m1314_head_support_fill_native.py`
- JSON: `runs/m1315_fill_budget_frontier_smoke_v1/m1314_native.json`
- Markdown: `runs/m1315_fill_budget_frontier_smoke_v1/m1314_native.md`

Surface:

- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- queries: `249`
- validation: leave-one-dataset-out

## Result

Best rows:

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `uniform_l1_head50_minus_tail_fill4_s1` | 3.422 | 1 | +0.002030 | +0.000708 | -0.000054 | +0.001153 | +0.000015 | 0.013944 |
| `signed_sum_head50_minus_tail_fill4_s1` | 3.422 | 1 | +0.002030 | +0.000598 | -0.000121 | +0.001354 | +0.000015 | 0.013220 |
| `uniform_l1_source_abs_fill4_s1` | 3.422 | 1 | +0.001938 | +0.000554 | -0.000195 | +0.000807 | +0.000031 | 0.010654 |
| `m1314_source_abs_signed_sum_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | 0.006020 |

The smaller budgets expose the tradeoff:

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `signed_sum_head50_minus_tail_fill3_s1` | 2.751 | +0.002266 | +0.000901 | +0.000115 | +0.001425 | -0.000773 | -0.002972 |
| `signed_sum_head50_minus_tail_fill2_s1` | 2.072 | +0.002266 | +0.000745 | +0.000505 | +0.000885 | -0.000773 | -0.003741 |
| `uniform_l1_head50_minus_tail_fill3_s1` | 2.751 | +0.002030 | +0.000989 | +0.000181 | +0.001224 | -0.000773 | -0.004159 |
| `uniform_l1_head50_minus_tail_fill2_s1` | 2.072 | +0.002030 | +0.000976 | +0.000681 | +0.000684 | -0.000773 | -0.004277 |

## Interpretation

M1315 does not find a macro-safe budget.

The frontier is informative:

- lower fill budgets (`fill1` to `fill3`) preserve NDCG and ranking metrics but
  lose CUB;
- `fill4` restores CUB but introduces a small NDCG regression;
- `fill5` keeps CUB but gives back too much Recall/MAP/NDCG gain;
- `head50_minus_tail` remains better than plain `source_abs` at the same
  budget.

This means the current bottleneck is not "choose the right budget." It is a
boundary conflict between:

1. support atoms needed to preserve candidate upper bound;
2. head-rank atoms needed to avoid NDCG harm.

## Decision

Stop the simple fill-budget frontier.

Do not scale M1315 to full `shared15`.

The retained signal is still useful:

> `head50_minus_tail` plus uniform-L1 is the best current fill source, but CUB
> restoration and NDCG preservation need separate value treatment.

The next valid branch must decouple membership support from rank pressure. A
minimal follow-up would test two-tier value construction:

1. keep clean prefix and `head50_minus_tail` fill4 membership;
2. assign lower weight to fill atoms than prefix atoms;
3. require NDCG >= 0 and CUB >= 0 on the same smoke;
4. stop if this only recreates the fill3 CUB loss.
