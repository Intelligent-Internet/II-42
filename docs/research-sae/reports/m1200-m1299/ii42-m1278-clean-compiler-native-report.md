# M1278 Clean Compiler Native Replay

M1278 replays the M1277 clean-objective compiler selections through the native
retrieval path on hard rows only.

## Run

- Surface: hard-row smoke
- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Output JSON:
  `runs/m1278_clean_compiler_native_smoke_v1/m1278_clean_compiler_native.json`
- Output detail:
  `runs/m1278_clean_compiler_native_smoke_v1/m1278_clean_compiler_native.md`

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1278_source_abs_signed_sum_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1278_model_signed_sum_s1` | 1.309 | 1 | +0.002213 | +0.000631 | +0.000477 | +0.000896 | -0.001606 | -0.026064 |
| `m1278_model_uniform_l1_s1` | 1.309 | 1 | +0.001977 | +0.000862 | +0.000654 | +0.000695 | -0.001606 | -0.026600 |

## Interpretation

This is a useful partial failure.

The clean compiler does exactly what M1277 suggested:

- it selects far fewer atoms: `1.309` vs `4.602`
- it improves Recall/MAP/NDCG/MRR over baseline
- uniform_l1 improves MAP/NDCG relative to signed_sum

But it loses candidate upper bound:

- `dCUB = -0.001606`

This is why M1278 cannot scale.  The sparse clean target is too precision-heavy:
it can move ranking metrics, but it removes support atoms needed to preserve
candidate coverage.

## Decision

Do not scale the clean compiler alone.

Retain the real signal:

1. Clean sparse target prediction is learnable enough to improve ranking
   metrics.
2. Native CUB loss is the blocking failure.
3. The next objective must combine precision target with a support/CUB floor.

The next valid experiment should not be a new classifier or threshold.  It
should test a fixed support-fill construction:

- precision prefix from M1277 clean compiler;
- fill remaining support budget from source_abs signed_sum atoms;
- no learned gate;
- stop if CUB is not restored or if ranking gains disappear.
