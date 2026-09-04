# M1325 Rank-Source Oracle

## Question

M1321/M1324 made risk7 macro metrics all positive, but left row-level harm.
M1325 asks whether this harm can be removed by query-level rank-source choice
among the already replayed variants.

This is a no-DB capacity audit over existing replay JSON. It is not deployable
and does not justify training another selector yet.

## Command

```bash
python3 scripts/audit_m1325_rank_source_oracle.py \
  --inputs runs/m1321_signed_sum_candidate_two_stage_risk7_v1/m1318_anatomy.json,runs/m1324_low_reserve_candidate_two_stage_risk7_v1/m1318_anatomy.json \
  --output-root runs/m1325_rank_source_oracle_risk7_v1
```

## Result

| Variant | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_best_any` | 0 | 0 | +0.003866 | +0.005433 | +0.006570 | +0.003511 | +0.002159 | +0.057953 |
| `oracle_best_nonnegative_else_baseline` | 0 | 0 | +0.003131 | +0.005163 | +0.005455 | +0.002528 | +0.002559 | +0.049670 |

Selection counts for strict oracle:

| Source | Queries |
| --- | ---: |
| `m1324_rank_prefix_uniform_l1` | 283 |
| `baseline` | 128 |
| `m1324_candidate_self_low_reserve1` | 94 |
| `m1321_candidate_self_signed_sum` | 56 |
| `m1324_rank_fill_fs0p5` | 37 |
| `m1321_rank_prefix_uniform_l1` | 1 |

## Interpretation

This is a strong capacity signal.

M1321/M1324 row harm is not inevitable. A query-level choice between baseline,
candidate-self, prefix-rank, and fill-rank sources can make all risk7 datasets
non-negative while producing larger macro gains than any single source.

But this does not mean we should train another ordinary selector. The oracle
uses qrels-derived outcomes. Prior M1245/M1296/M1322 evidence says current
query-time features are weak for safe source choice.

The actual retained lesson is:

> Ranking source choice matters, and a useful source mixture exists. The next
> work must make that source choice intrinsic and observable, not bolt on a
> post-hoc gate.

## Decision

Keep M1325 as the current best direction signal.

Stop candidate-stage composition variants for now. The candidate source can
carry support; the next bottleneck is rank-source choice.

Next bounded work:

1. Inspect M1325 source-choice anatomy by dataset and source.
2. Check whether the strict-oracle choices are mostly dataset-specific. If yes,
   do not train a selector.
3. If the choices show reusable structure, design a generated ranking objective
   that learns source choice jointly with the rank-source construction.
