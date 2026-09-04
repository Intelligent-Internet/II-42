# M1279 Clean Compiler Support-Fill Replay

## Question

M1278 showed that the M1277 clean-objective compiler can improve ranking
metrics, but loses candidate upper bound.  M1279 tests a fixed support repair:
keep the M1277 precision prefix, then fill the remaining support budget with
`source_abs` signed-sum atoms.

This is a construction test, not a learned gate:

- prefix: M1277 clean compiler
- fill source: `source_abs` signed-sum atoms
- fill budgets: 4 and 8
- prefix geometries: `signed_sum`, `uniform_l1`
- surface: hard rows, `cqadupstack`, `scidocs`, `webis-touche2020`

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1279_prefix_uniform_l1_fill4_s1` | 3.422 | 1 | +0.001938 | +0.000554 | -0.000195 | +0.000807 | +0.000031 | +0.010654 |
| `m1279_prefix_signed_sum_fill4_s1` | 3.422 | 1 | +0.001938 | +0.000432 | -0.000262 | +0.001008 | +0.000031 | +0.009894 |
| `m1279_source_abs_signed_sum_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1279_prefix_signed_sum_fill8_s1` | 4.602 | 1 | +0.001331 | +0.000517 | -0.000286 | +0.000607 | +0.000031 | +0.006020 |
| `m1279_prefix_uniform_l1_fill8_s1` | 4.602 | 1 | +0.001331 | +0.000274 | -0.000425 | +0.000406 | +0.000031 | +0.003220 |

## Interpretation

Support-fill restores CUB, but does not produce a macro-safe policy.  Every
variant keeps one negative metric, and the negative metric is NDCG@10.  The
best fill variant improves Recall, MAP, MRR, and CUB, but the NDCG regression
means this cannot be promoted or scaled.

This narrows the M1278 failure:

1. the clean compiler is not useless; its sparse prefix carries rank signal
2. CUB can be restored mechanically by support fill
3. the resulting construction still harms top-rank ordering

The failure is therefore not just missing support.  The prefix/fill composition
still lacks row-safe ordering awareness.

## Decision

Stop this repair line.

Do not continue with more fixed fill budgets, threshold tweaks, or post-hoc
guards over the same source.  M1279 supports the broader M1224-M1253 diagnosis:
positive atoms and useful movements exist, but ordinary output protection does
not make them deployable.  The next branch should move the positive signals
into the source construction or training objective itself.

## Next Valid Direction

Use the retained signals as objective/source ingredients:

- M1224/M1225: CUB-specific teacher separates target from support harm.
- M1244: action-source structure has a strong oracle upper bound.
- M1251: `signed_sum_s1` is a real query-time native movement signal.
- M1277: sparse clean target prediction is learnable.

The next experiment should construct a target/harm-separated source before
native replay, then require the same gate:

1. full target/harm observability audit
2. hard-row native replay only if observability is clean
3. LODO/full shared15 only if hard-row replay is macro-safe

## Artifacts

- Script: `scripts/replay_m1279_clean_compiler_support_fill.py`
- JSON: `runs/m1279_clean_compiler_support_fill_smoke_v1/m1279_clean_compiler_support_fill.json`
- Markdown: `runs/m1279_clean_compiler_support_fill_smoke_v1/m1279_clean_compiler_support_fill.md`
