# M1314 Head-Support Fill Native Replay

## Question

M1279 showed that filling the clean compiler prefix with `source_abs` support
atoms can restore candidate upper bound, but still leaves NDCG harm. M1314
keeps the M1277 clean prefix and changes only the fill source:

- prefer atoms supported by native head documents (`head10`, `head50`);
- penalize tail-only support (`head50_minus_tail`);
- compare against the source-abs repair anchor;
- test both signed-sum geometry and uniform-L1 geometry.

This is source construction, not another query selector or post-hoc gate.

## Run

```bash
python3 scripts/replay_m1314_head_support_fill_native.py \
  --datasets cqadupstack,scidocs,webis-touche2020 \
  --output-root runs/m1314_head_support_fill_native_smoke_v1
```

Artifacts:

- Script: `scripts/replay_m1314_head_support_fill_native.py`
- JSON: `runs/m1314_head_support_fill_native_smoke_v1/m1314_native.json`
- Markdown: `runs/m1314_head_support_fill_native_smoke_v1/m1314_native.md`

Surface:

- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- queries: `249`
- validation: leave-one-dataset-out

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1314_prefix_uniform_l1_head50_minus_tail_fill4_s1` | 3.422 | 1 | +0.002030 | +0.000708 | -0.000054 | +0.001153 | +0.000015 | 0.013944 |
| `m1314_prefix_signed_sum_head50_minus_tail_fill4_s1` | 3.422 | 1 | +0.002030 | +0.000598 | -0.000121 | +0.001354 | +0.000015 | 0.013220 |
| `m1314_prefix_uniform_l1_head10_fill4_s1` | 3.422 | 1 | +0.002121 | +0.000772 | -0.000203 | +0.000607 | +0.000031 | 0.011730 |
| `m1314_prefix_uniform_l1_source_abs_fill4_s1` | 3.422 | 1 | +0.001938 | +0.000554 | -0.000195 | +0.000807 | +0.000031 | 0.010654 |
| `m1314_source_abs_signed_sum_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | 0.006020 |
| `m1314_prefix_uniform_l1_head50_fill4_s1` | 3.422 | 2 | +0.001938 | +0.000719 | -0.000584 | +0.000807 | -0.000773 | -0.013628 |

The best variant is `uniform_l1_head50_minus_tail_fill4_s1`. It is not
macro-safe because NDCG is still slightly negative, but it improves strongly
over the source-abs anchor:

| Comparison vs `m1314_source_abs_signed_sum_s1` | Delta |
| --- | ---: |
| selected atoms/query | -1.181 |
| Recall@100 | +0.000699 |
| MAP@100 | +0.000191 |
| NDCG@10 | +0.000231 |
| MRR@20 | +0.000546 |
| Candidate upper bound | -0.000015 |

## Interpretation

M1314 is a narrow positive source-construction signal, not a deployable result.

The useful part is specific:

- head support alone (`head50`) hurts CUB too much;
- multiplying source score by head support (`source_x_head50`) also hurts CUB;
- `head50_minus_tail` is better, which says the source should prefer atoms that
  explain head documents while explicitly discounting tail-only support;
- `uniform_l1` is better than raw signed-sum geometry for this fill path.

The remaining harm is very small and macro-level NDCG-only. That is a different
failure mode from the broader selector failures in M1311-M1313.

## Decision

Do not scale M1314 to full `shared15` yet because it is not macro-safe.

Do keep the signal:

> head-support fill needs a tail penalty, and uniform-L1 geometry is currently
> the best value shape for the clean-prefix fill branch.

The next valid probe is a bounded fill-budget frontier over this exact source:

1. keep the source construction fixed to `head50_minus_tail`;
2. test only small fill budgets around the current `fill4`;
3. require NDCG >= 0 before any broader replay;
4. stop if the NDCG fix simply gives back Recall/MAP/MRR.

Do not turn this into another selector/gate/classifier line.
