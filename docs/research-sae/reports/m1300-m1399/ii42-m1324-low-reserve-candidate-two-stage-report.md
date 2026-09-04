# M1324 Low-Reserve Candidate Two-Stage

## Question

M1323 showed that `reserve1_s1` improves candidate support but leaves MRR
negative. M1324 tests whether using `low_reserve1` only as candidate stage,
then ranking with the M1321 lower-pressure rank query, fixes the remaining
ranking harm.

## Command

```bash
python3 scripts/audit_m1318_two_stage_row_anatomy.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --candidate-stage low_reserve1 \
  --output-root runs/m1324_low_reserve_candidate_two_stage_risk7_v1
```

Elapsed: `294.23s`.

## Result

| Variant | RankAtoms | NegMetrics | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1324_rank_fill_fs0p5` | 3.990 | 0 | 7 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | +0.000071 | +0.011388 |
| `m1324_rank_prefix_uniform_l1` | 2.469 | 1 | 8 | -0.000231 | +0.001486 | +0.001230 | +0.000662 | +0.000071 | +0.007160 |
| `m1324_candidate_self_low_reserve1` | 8.668 | 1 | 8 | +0.001960 | +0.001352 | +0.001683 | -0.001034 | +0.000071 | +0.004886 |

For comparison, M1321 `signed_sum_top8` candidate plus the same rank fill had:

| Variant | DatasetNeg | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1321_rank_fill_fs0p5` | 7 | +0.000428 | +0.001729 | +0.001290 | +0.000704 | +0.000059 | +0.011376 |

## Interpretation

`low_reserve1` as candidate stage only adds a tiny CUB improvement:
`+0.000059 -> +0.000071`.

The ranking deltas and dataset-level harm are otherwise unchanged. This means
the current blocker is not candidate support. It is the ranking query/source
used inside the candidate pool.

## Decision

Do not continue candidate-stage composition variants.

M1321/M1324 preserve the same structural lesson:

- signed-sum / low-reserve candidate support exists;
- lower-pressure rank fill can make macro metrics all positive;
- row-level harm persists because the rank source is not row-safe.

The next useful work should inspect prior rank-source and pairwise objective
lines, then replace the ranking source rather than adding another candidate
source or post-hoc guard.
