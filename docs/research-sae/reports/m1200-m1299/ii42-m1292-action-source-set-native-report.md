# M1292 Action-Source Set Native Replay

## Question

M1291 showed that action-aware atoms are not safe as independent single-atom
labels.  M1292 tests the next logical claim: the useful signal may be
set-level and coordinated, matching the earlier M1251 `signed_sum_s1` result.

This is a bounded replay, not a training run.

## Setup

- Datasets: `arguana,cqadupstack,fiqa,scidocs`
- Limit: `25` queries per dataset
- Split: leave-one-dataset-out source construction
- Sources: `signed_sum`, `high`, `mid`, `low`, `cub_source_abs`
- Source top-n: `8`
- Scales: `0.5`, `1.0`
- Output:
  `runs/m1292_action_source_set_native_limit25_v1/m1292_native.json`

## Result

| Variant | Selected | NegMetrics | Top95 | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `low_top8_s0.5` | 1.180 | 1 | 0.991684 | +0.000000 | -0.000576 | +0.000712 | +0.000000 | +0.000000 |
| `signed_sum_top8_s0.5` | 1.840 | 1 | 0.989368 | +0.000000 | -0.001338 | +0.000227 | +0.000000 | +0.000000 |
| `cub_source_abs_top8_s0.5` | 1.840 | 1 | 0.991158 | +0.000000 | -0.001426 | +0.000201 | +0.000000 | +0.000000 |
| `low_top8_s1` | 1.180 | 2 | 0.986737 | +0.000000 | -0.000787 | +0.000584 | -0.000026 | +0.000000 |
| `cub_source_abs_top8_s1` | 1.840 | 2 | 0.985368 | +0.000000 | -0.001100 | +0.000350 | -0.000026 | +0.000000 |
| `high_top8_s1` | 0.710 | 2 | 0.994421 | +0.000000 | -0.000588 | -0.000370 | +0.000000 | +0.000000 |
| `mid_top8_s1` | 0.710 | 2 | 0.995895 | +0.000000 | -0.000716 | -0.000441 | +0.000000 | +0.000000 |
| `mid_top8_s0.5` | 0.710 | 2 | 0.998316 | +0.000000 | -0.000854 | -0.000511 | +0.000000 | +0.000000 |
| `high_top8_s0.5` | 0.710 | 2 | 0.997263 | +0.000000 | -0.000890 | -0.000511 | +0.000000 | +0.000000 |
| `signed_sum_top8_s1` | 1.840 | 3 | 0.981474 | +0.000000 | -0.008294 | -0.004987 | -0.007026 | +0.000000 |

No variant is safe-positive on this small LODO surface.

## Interpretation

M1292 is a local negative result, not a global refutation of M1251.

The important diagnostic is `Selected`: this small four-dataset limit surface
only admits roughly `0.7-1.8` atoms per query after LODO CUB/action filtering.
That is much narrower than the earlier full shared15 M1251 surface, where
`signed_sum_s1` selected about `8` atoms per query and was macro positive.

So the conclusion is not "set-level source is dead".  The conclusion is:

- this small smoke is too sparse to validate the M1251 set effect;
- isolated atom labels remain unsupported;
- source capacity and heldout-stable allowed atoms are now a first-class
  bottleneck.

## Decision

Do not train from M1291/M1292 rows.

Do not keep expanding small selector/gate variants on the four-dataset limit
surface.  The next valid step is a direct stability audit of the already-known
M1251 full shared15 candidate, focused on row-level harm and source capacity.

## Next Valid Step

M1293 should not invent a new selector.  It should audit M1251 on the real
surface:

1. Load or rerun the full shared15 M1251 `signed_sum_s1` result.
2. Report per-dataset deltas, selected count, allowed atom count, and top95.
3. Identify whether failures correlate with low selected count, low allowed
   count, or specific metric trade-offs.
4. Only if M1251 remains stable should a constrained source objective be built
   around it.

This keeps the route evidence-driven and avoids another classifier loop.
