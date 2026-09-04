# SAE M38 Hard-Bucket Weighting Results Report

Status: closed after one primary run.

## Summary

M38 tested whether explicit hard-bucket sample weighting can fix the robustness
gap found in M36/M37.

It does not. The run is stable and preserves the M36 aggregate almost exactly,
but it does not materially improve the hard-dataset collapses:

```text
M36 best: Recall@100 0.8671, NDCG@10 0.7731, MAP@100 0.7429
M38 best: Recall@100 0.8670, NDCG@10 0.7730, MAP@100 0.7432
```

The useful conclusion is negative but clear:

```text
available expanded real-query data
+ hard-bucket ranking-loss upweighting
  -> no robust broad-query recovery
```

The blocker is not solved by loss weighting over the current training surface.
It still points to missing query-distribution/supervision quality, or a more
structural query representation change.

## Implementation Notes

M38 added opt-in hard-bucket weighting to the M31 trainer. Default behavior is
unchanged:

```text
--hard-bucket-weight-mode off
--hard-bucket-weight 1.0
```

The weight is applied only to the ranking terms:

```text
teacher loss + qrel residual loss + BM25 preserve loss
```

The regularizers remain unweighted:

```text
fanout + support/value imitation + scale prior
```

This keeps the physical-cost gate meaningful.

The implementation also changed M31 epoch logging to sync scalar training
metrics from MPS once per epoch instead of once per example. This only affects
training overhead and does not change the objective.

## Run

Output:

```text
results/sae/m38/hard-bucket-weighted-nfcorpus-expanded
```

Training scope:

| Metric | Value |
| --- | ---: |
| Train datasets | 8 |
| Train documents | 128,816 |
| Train queries | 4,781 |
| Train qrel pairs | 126,212 |
| Eval datasets | 15 |
| Eval queries | 1,342 |

Hard-bucket coverage:

| Metric | Value |
| --- | ---: |
| Weighted examples | 334 |
| Weighted fraction | 0.0699 |
| Mean example weight | 1.1397 |
| Max example weight | 3.0 |

Bucket counts:

| Bucket | All examples | Weighted examples |
| --- | ---: | ---: |
| `broad_high_df` | 897 | 197 |
| `many_positive` | 201 | 201 |
| `semantic_teacher_advantage` | 632 | 149 |

## Full15 Matrix

| Run | Best | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidate docs | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | `m31_fixed_w0p5` | 0.8694 | 0.8780 | 0.7734 | 0.7449 | 2964.8 | 2778.6 |
| M36 nfcorpus-expanded | `m31_fixed_w0p5` | 0.8671 | 0.8795 | 0.7731 | 0.7429 | 2923.6 | 2384.2 |
| M37 transformer | `m31_fixed_w0p25` | 0.7868 | 0.7870 | 0.6680 | 0.6275 | 2969.6 | 2175.5 |
| M38 hard-bucket | `m31_fixed_w0p5` | 0.8670 | 0.8794 | 0.7730 | 0.7432 | 2925.6 | 2399.9 |

M38 is essentially tied with M36. It slightly raises SAE postings compared with
M36 and does not create a new Pareto point.

## Hard Dataset Table

### `trec-covid`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6226 | 0.4545 | 0.1435 | -0.2124 | -0.3094 |
| M36 nfcorpus-expanded | 0.6429 | 0.4516 | 0.1419 | -0.1922 | -0.3123 |
| M37 transformer | 0.6620 | 0.4689 | 0.1390 | -0.1731 | -0.2950 |
| M38 hard-bucket | 0.6427 | 0.4540 | 0.1427 | -0.1924 | -0.3099 |

M38 does not improve `trec-covid` beyond M36. M37 remains better on
`trec-covid` alone, but M37 is not viable because it collapses aggregate
quality and other hard datasets.

### `msmarco`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6320 | 0.8765 | 0.7894 | -0.0929 | -0.0587 |
| M36 nfcorpus-expanded | 0.6395 | 0.8784 | 0.7900 | -0.0853 | -0.0568 |
| M37 transformer | 0.5837 | 0.7867 | 0.7394 | -0.1411 | -0.1485 |
| M38 hard-bucket | 0.6387 | 0.8784 | 0.7900 | -0.0861 | -0.0568 |

M38 is effectively identical to M36.

### `dbpedia-entity`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6942 | 0.7542 | 0.8523 | 0.0085 | -0.0283 |
| M36 nfcorpus-expanded | 0.6834 | 0.7438 | 0.8558 | -0.0023 | -0.0387 |
| M37 transformer | 0.6270 | 0.6552 | 0.7818 | -0.0587 | -0.1273 |
| M38 hard-bucket | 0.6834 | 0.7434 | 0.8541 | -0.0022 | -0.0391 |

M38 is effectively identical to M36.

### `nfcorpus`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.5156 | 0.3138 | 0.5475 | 0.0922 | 0.0873 |
| M36 nfcorpus-expanded | 0.5009 | 0.2956 | 0.5331 | 0.0775 | 0.0691 |
| M37 transformer | 0.3737 | 0.1859 | 0.3143 | -0.0497 | -0.0406 |
| M38 hard-bucket | 0.5009 | 0.2959 | 0.5342 | 0.0775 | 0.0694 |

M38 does not convert the expanded `nfcorpus` supervision into a stronger
general robust query-side representation.

## Calibration Note

The generated run also reports high aggregate scores for `m31_calibrated` and
higher fixed SAE weights, but those settings worsen hard-dataset collapse:

| Source | Full15 NDCG@10 | Full15 MAP@100 | `trec-covid` NDCG@10 | `trec-covid` MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `m31_calibrated` | 0.7966 | 0.7673 | 0.4894 | 0.3261 |
| `m31_fixed_w0p5` | 0.7730 | 0.7432 | 0.6427 | 0.4540 |
| `m31_fixed_w1` | 0.7959 | 0.7680 | 0.4041 | 0.2601 |

This confirms the previous pattern: aggregate quality can hide severe
broad-query collapse. Collapse-aware selection remains necessary.

## Decision

M38 is closed as failed gate.

Do not continue:

- more hard-bucket weight sweeps on the same expanded training data;
- higher SAE weights selected by aggregate-only metrics;
- deeper encoder retries without new hard-bucket supervision.

Keep:

- opt-in hard-bucket weighting in M31 as an ablation tool;
- epoch-level MPS metric synchronization improvement;
- the evidence that only 334 of 4,781 examples receive the intended hard
  treatment under the best available expanded real-query source.

## Next Direction

The next useful step is not another scalar weight. It should attack the source
of the supervision:

```text
validated broad-query generator
or
teacher-neighborhood query distillation with query intent diversity
or
query-side representation objective that directly models many-positive
semantic neighborhoods
```

The M38 result makes the blocker narrower:

```text
hard examples are too sparse or too distributionally different
for weighting alone to transfer into robust query-side atoms.
```
