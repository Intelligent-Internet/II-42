# SAE M41 Gate-Aware Fanout Regularization Results Report

Status: closed as a positive cost-control direction, not a product gate pass.

## Summary

M41 tested whether M40's lexical-DF gate quality can be kept while reducing the
SAE posting fanout caused by the trained query atom distribution.

Result: M41 succeeds as a cost-control frontier. It does not dominate M40 on
quality, but it creates several useful operating points:

```text
M40 sweep:     NDCG 0.7887, MAP 0.7596, SAE postings 2734.7
M41 w0p01:     NDCG 0.7879, MAP 0.7580, SAE postings 2620.5
M41 w0p02:     NDCG 0.7863, MAP 0.7559, SAE postings 2476.9
M41 w0p04:     NDCG 0.7848, MAP 0.7541, SAE postings 2240.4
```

The best balanced point is `w0p01`: it preserves almost all M40 quality while
reducing SAE postings by about `4.2%`. The aggressive point `w0p04` reduces SAE
postings by about `18.1%` and still keeps NDCG/MAP above the M36/M39 fixed
baselines, but it gives up more of the M40 ranking gain.

## Implementation

M41 adds opt-in exported-atom fanout controls to the M31 trainer:

```text
--topk-fanout-loss-weight
--topk-fanout-active-dims
--broad-topk-fanout-multiplier
--ordinary-topk-fanout-multiplier
```

The key choice is to regularize the top-k support logits that become query
atoms, rather than only the full soft query vector. This better matches the
physical payload that the unified sparse engine will touch at query time.

M41 also updates the DF-gate sweep to include physical means. The sweep itself
still only changes score weights; physical cost is determined by the generated
query atom payload.

## Training Outputs

| Run | Output |
| --- | --- |
| smoke | `results/sae/m41/smoke-topk-fanout` |
| `w0p04` | `results/sae/m41/topk-fanout-gate-train-eval-current` |
| `w0p02` | `results/sae/m41/topk-fanout-w0p02-gate-train-eval-current` |
| `w0p01` | `results/sae/m41/topk-fanout-w0p01-gate-train-eval-current` |

Sweep outputs:

| Run | Output |
| --- | --- |
| M40 rerun with physical means | `results/sae/m40/df-gate-sweep` |
| `w0p04` sweep | `results/sae/m41/df-gate-sweep` |
| `w0p02` sweep | `results/sae/m41/df-gate-sweep-w0p02` |
| `w0p01` sweep | `results/sae/m41/df-gate-sweep-w0p01` |

## Direct Fixed-Weight Comparison

These rows use `m31_fixed_w0p5` before the post-hoc DF-gate sweep.

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M36 | 0.8671 | 0.8795 | 0.7731 | 0.7429 | 0.4516 | 2923.6 | 2384.2 |
| M39 | 0.8670 | 0.8813 | 0.7707 | 0.7435 | 0.4559 | 2922.9 | 2430.6 |
| M40 | 0.8679 | 0.8813 | 0.7739 | 0.7455 | 0.4595 | 2948.5 | 2734.7 |
| M41 `w0p04` | 0.8662 | 0.8785 | 0.7702 | 0.7419 | 0.4556 | 2906.8 | 2240.4 |
| M41 `w0p02` | 0.8668 | 0.8791 | 0.7711 | 0.7430 | 0.4590 | 2926.3 | 2476.9 |

Direct fixed-weight quality is not the main M40/M41 operating mode, but it
confirms that the top-k fanout penalty affects physical atom support as
intended.

## DF-Gate Sweep Comparison

| Run | Threshold | Low SAE | High SAE | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC NDCG | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M40 sweep | 0.15 | 0.35 | 0.75 | 0.8670 | 0.8985 | 0.7887 | 0.7596 | 0.6519 | 0.4701 | 2948.5 | 2734.7 |
| M41 `w0p01` | 0.15 | 0.35 | 0.75 | 0.8663 | 0.8976 | 0.7879 | 0.7580 | 0.6528 | 0.4698 | 2938.7 | 2620.5 |
| M41 `w0p02` | 0.15 | 0.35 | 0.75 | 0.8665 | 0.8954 | 0.7863 | 0.7559 | 0.6531 | 0.4685 | 2926.3 | 2476.9 |
| M41 `w0p04` | 0.12 | 0.35 | 0.75 | 0.8646 | 0.8945 | 0.7848 | 0.7541 | 0.6660 | 0.4717 | 2906.8 | 2240.4 |

## Interpretation

M41 confirms that the M40 cost issue is trainable. The gate itself only changes
scores, but the top-k fanout regularizer changes the exported query atom
support and therefore lowers physical postings.

The result is a quality-cost tradeoff rather than a pure win:

- `w0p01` is the safest operating point. It keeps nearly all M40 ranking
  quality and lowers SAE postings modestly.
- `w0p02` is a middle operating point. It lowers postings meaningfully while
  keeping full15 quality above the earlier M36/M39 fixed-weight baselines.
- `w0p04` is the aggressive operating point. It gives the lowest postings and
  the best TREC NDCG among M41 runs, but loses more aggregate quality.

This is useful because product engineering will need a tunable physical-cost
frontier, not just one maximum-quality model.

## Decision

M41 is positive evidence for continuing the M40 route:

```text
validated broad-query supervision
+ runtime-safe lexical-DF gate
+ exported-atom fanout regularization
```

It still is not a product gate pass. The strict no-collapse gate remains open,
and M41 does not prove direct dense-removal readiness. It does, however, solve
the immediate M40 concern that the lexical-DF route necessarily increases SAE
posting cost.

## Next Direction

The next useful work should not be another scalar fanout-loss sweep. M41 has
already established the frontier. The next step should focus on a more
structural atom allocation objective:

```text
query atom utility = ranking contribution / posting fanout
```

Candidate M42 directions:

- train an atom-utility target from per-query ablations;
- keep the M40/M41 DF gate but select atoms by utility-aware support logits;
- add model selection on `NDCG + MAP - lambda * normalized_sae_postings`;
- test whether `w0p01` or `w0p02` is the right default physical profile before
  trying deeper query encoders again.
