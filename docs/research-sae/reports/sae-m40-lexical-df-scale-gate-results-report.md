# SAE M40 Lexical-DF Scale Gate Results Report

Status: closed as a positive direction, not a product gate pass.

## Summary

M40 tested the M39 follow-up hypothesis:

```text
validated broad-query source
+ runtime-safe lexical DF scale gate
  -> improve trec-covid without globally lowering SAE weight
```

The result is the strongest direction since M39. It still does not pass the
strict no-collapse product gate, but it forms a much better Pareto point:

```text
M40 best gate:
Recall@100 0.8670
NDCG@10    0.7887
MAP@100    0.7596
trec MAP   0.4701
```

Compared with M36, this improves aggregate NDCG/MAP substantially while also
improving `trec-covid` MAP:

```text
M36 fixed_w0p5: NDCG 0.7731, MAP 0.7429, trec MAP 0.4516
M40 DF gate:    NDCG 0.7887, MAP 0.7596, trec MAP 0.4701
```

This validates the route:

```text
broad-query supervision + query-type-aware semantic scale
```

## Implementation

M40 extends the M31 trainer with opt-in lexical-DF calibration:

```text
--calibration-feature-mode lexical_df
--enable-df-gate
--broad-scale-guard-weight
--broad-sae-scale-target
```

The added features are runtime-safe:

- query content-token mean BM25 document frequency;
- query content-token high-DF share.

M40 also adds:

```text
scripts/research_sae_m40_df_gate_sweep.py
```

The sweep reads M40 query latents and evaluates gate choices without retraining.

## Training Result

Training output:

```text
results/sae/m40/lexical-df-gate-train-eval-current
```

Training scope:

| Metric | Value |
| --- | ---: |
| Train datasets | 9 |
| Train documents | 146,353 |
| Train queries | 4,981 |
| Train qrel pairs | 146,212 |
| Eval datasets | 15 |
| Eval queries | 1,342 |

Direct M31 sources:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC NDCG | TREC MAP |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m31_fixed_w0p5` | 0.8679 | 0.8813 | 0.7739 | 0.7455 | 0.6424 | 0.4595 |
| `m31_df_gate` initial | 0.8587 | 0.8682 | 0.7639 | 0.7340 | 0.6636 | 0.4837 |
| `m31_calibrated` | 0.8721 | 0.9097 | 0.7965 | 0.7682 | 0.5672 | 0.3898 |
| `m31_fixed_w0p25` | 0.8414 | 0.8428 | 0.7349 | 0.7031 | 0.6616 | 0.4842 |

Interpretation:

- learned calibration still over-optimizes aggregate and hurts broad-query MAP;
- global lower SAE still helps `trec-covid` but hurts aggregate;
- the initial DF gate keeps the TREC benefit while reducing the global damage,
  but its default low/high values are not optimal.

## DF Gate Sweep

Sweep output:

```text
results/sae/m40/df-gate-sweep
```

Best result:

| Threshold | Low SAE | High SAE | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC NDCG | TREC MAP |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.15 | 0.35 | 0.75 | 0.8670 | 0.8985 | 0.7887 | 0.7596 | 0.6519 | 0.4701 |

The best gate marks broad queries as:

| Dataset | Broad queries |
| --- | ---: |
| `trec-covid` | 45 |
| `msmarco` | 1 |
| `dbpedia-entity` | 5 |
| `nfcorpus` | 23 |

This is the key point: the lexical-DF gate mostly targets `trec-covid` and
does not broadly suppress semantic matching on `msmarco`.

## Comparison

| Run/source | Recall@100 | NDCG@10 | MAP@100 | TREC NDCG | TREC MAP | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M36 `fixed_w0p5` | 0.8671 | 0.7731 | 0.7429 | 0.6429 | 0.4516 | 2384.2 |
| M39 `fixed_w0p5` | 0.8670 | 0.7707 | 0.7435 | 0.6332 | 0.4559 | 2430.6 |
| M39 `fixed_w0p25` | 0.8409 | 0.7347 | 0.7028 | 0.6594 | 0.4822 | 2430.6 |
| M40 `fixed_w0p5` | 0.8679 | 0.7739 | 0.7455 | 0.6424 | 0.4595 | 2734.7 |
| M40 DF gate sweep | 0.8670 | 0.7887 | 0.7596 | 0.6519 | 0.4701 | 2734.7 |

M40 improves ranking quality but increases SAE postings compared with M36/M39.
That cost increase is from the trained query atom distribution, not from the
gate itself. It must be handled before product promotion.

## Decision

M40 is not a product-ready model because the strict no-collapse gate is still
not met and SAE postings increase.

However, M40 is a positive direction:

- M39's broad-query supervision is useful.
- A runtime-safe lexical-DF gate can keep most aggregate quality while
  improving broad-query MAP.
- The route is now more promising than deeper encoder retries or plain
  hard-bucket weighting.

## Next Direction

M41 should focus on cost and stability:

```text
M40 lexical-DF gate
+ fanout-aware query atom regularization
+ gate-aware model selection
  -> keep NDCG/MAP gain while reducing SAE postings
```

The next run should not increase synthetic query count first. It should
preserve the M40 gate signal while controlling the physical footprint.
