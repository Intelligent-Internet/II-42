# M1337 Prefix Tail Row Anatomy

## Question

M1336 found that `rank_prefix + candidate_tail_s0p5` is the best local
hard-row tail-blend signal:

- macro score turns positive;
- macro CUB is preserved;
- row-level ranking harm remains, especially on `cqadupstack`, with a small
  Recall loss on `webis-touche2020`.

M1337 asks whether that harm is localized enough to repair.  It replays only:

- `rank_prefix`;
- `m1337_candidate_tail_s0p5`.

Then it expands the result into query-level metric deltas, top100 document
movement, and tail-atom harm concentration.

## Run

```bash
python3 scripts/audit_m1337_prefix_tail_row_anatomy.py \
  --output-root runs/m1337_prefix_tail_row_anatomy_smoke_v1
```

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

Query count: `249`.

## Macro

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `rank_prefix` | 1.309 | 1 | +0.001977 | +0.000862 | +0.000654 | +0.000695 | -0.001606 | -0.026600 |
| `m1337_candidate_tail_s0p5` | 4.683 | 1 | +0.001847 | +0.000109 | -0.000083 | +0.000807 | +0.000000 | +0.010185 |

## Blend Minus Prefix

| Dataset | Queries | MeanTail | ScoreDelta | dRecall | dMAP | dNDCG | dMRR | dCUB | Buckets |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `cqadupstack` | 100 | 4.200 | -0.057353 | +0.000000 | -0.001919 | -0.001950 | -0.000526 | +0.000000 | `flat=73 harm=16 help=11` |
| `scidocs` | 100 | 1.860 | -0.014518 | +0.000000 | +0.000049 | +0.000055 | +0.000807 | +0.004000 | `flat=59 harm=24 help=17` |
| `webis-touche2020` | 49 | 4.776 | -0.008354 | -0.000658 | -0.000010 | +0.000124 | +0.000000 | +0.000000 | `flat=27 harm=14 help=8` |

The hard-row macro gain is not coming from a broad per-query improvement.
Most queries are flat.  Harm queries outnumber help queries on every row.

## Document Movement

Relevant top100 movement is almost absent:

| Bucket | Queries | NewRel@100 | DropRel@100 | ScoreDelta | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `rel_dropped` | 1 | 0.000 | 1.000 | -0.291107 | -0.032258 | -0.009986 | +0.000000 | +0.000000 | +0.000000 |
| `rel_flat` | 248 | 0.000 | 0.000 | -0.029457 | +0.000000 | -0.000716 | -0.000739 | +0.000113 | +0.001613 |

The main effect is rank reshuffling, not adding new relevant top100 documents.
This explains why MAP/NDCG can drop even when CUB is preserved.

## Atom Concentration

| Metric | Value |
| --- | ---: |
| Harm queries | 54 |
| Help queries | 36 |
| Top-5 harm-query coverage | 0.5926 |
| Top-10 harm-query coverage | 0.7963 |
| Top-20 harm-query coverage | 0.9259 |

Top harmful atoms:

| Atom | HarmCount | HelpCount | HarmPrecision | Total |
| ---: | ---: | ---: | ---: | ---: |
| 207 | 10 | 4 | 0.7143 | 14 |
| 154 | 10 | 8 | 0.5556 | 18 |
| 641 | 7 | 3 | 0.7000 | 10 |
| 39 | 7 | 6 | 0.5385 | 13 |
| 65 | 6 | 0 | 1.0000 | 6 |
| 195 | 5 | 2 | 0.7143 | 7 |
| 299 | 5 | 4 | 0.5556 | 9 |
| 84 | 5 | 0 | 1.0000 | 5 |
| 72 | 5 | 4 | 0.5556 | 9 |
| 727 | 4 | 1 | 0.8000 | 5 |

This is not random diffuse noise: a small group of atoms covers much of the
harm.  But it is also not a clean blacklist: several high-harm atoms also occur
in help queries.

## Interpretation

M1337 supports the review guidance.

`candidate_tail` is a real source signal, but the failure mode is not a simple
query-time gate problem.  The harm is partly atom-concentrated, yet row-spanning
and mixed with helpful cases.

## Decision

Do not run full `shared15` for M1336/M1337 as-is.

Do not continue scale sweeps or tail selectors.

One bounded follow-up is justified:

1. learn risky tail atoms on training folds;
2. penalize them at source level on heldout rows;
3. reject the branch if it cannot beat the unpenalized tail blend safely.

That follow-up is M1338.

## Artifacts

- Script:
  `scripts/audit_m1337_prefix_tail_row_anatomy.py`
- JSON:
  `runs/m1337_prefix_tail_row_anatomy_smoke_v1/m1337_prefix_tail_row_anatomy.json`
- Markdown:
  `runs/m1337_prefix_tail_row_anatomy_smoke_v1/m1337_prefix_tail_row_anatomy.md`
