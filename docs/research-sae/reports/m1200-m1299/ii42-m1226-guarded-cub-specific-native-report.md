# M1226 Guarded CUB-Specific Native Replay

## Question

M1225 produced the strongest recent macro gain, but six of fifteen datasets
still had at least one negative metric.  M1226 keeps M1225 fixed:

- teacher: M1224 CUB-specific atom target/harm
- selector: `rule_source_abs_top8`
- scale: `1.0`

It only adds held-out query-level guards trained on native replay outcomes.

## Results

### Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1225_cub_source_abs_top8_s1` | 4.574 | +0.000821 | +0.000031 | +0.000271 | +0.000379 | +0.000015 | 0 |
| `m1226_all_safe_p05` | 2.361 | +0.000094 | +0.000154 | +0.000603 | +0.000915 | +0.000803 | 0 |
| `m1226_cub_rank_safe_p05` | 2.361 | +0.000094 | +0.000154 | +0.000603 | +0.000915 | +0.000803 | 0 |
| `m1226_rank_safe_p05` | 2.361 | +0.000094 | +0.000154 | +0.000603 | +0.000915 | +0.000803 | 0 |
| `m1226_cub_safe_p05` | 3.201 | +0.000018 | -0.000280 | -0.000286 | -0.000201 | +0.000015 | 3 |

The hard-row guard does what it was designed to do: it improves CUB safety and
keeps all macro metrics non-negative for the strict labels.

### Full Shared15

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | NegMetrics |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1225_cub_source_abs_top8_s1` | 7.970 | +0.001685 | +0.002312 | +0.002033 | +0.002252 | +0.000012 | 0 |
| `m1226_cub_safe_p05` | 5.753 | +0.000072 | +0.001576 | +0.001523 | +0.001600 | +0.000156 | 0 |
| `m1226_cub_rank_safe_p05` | 4.656 | +0.000272 | +0.001102 | +0.001007 | +0.000928 | +0.000157 | 0 |
| `m1226_all_safe_p05` | 4.692 | +0.000272 | +0.001102 | +0.001007 | +0.000928 | +0.000144 | 0 |
| `m1226_rank_safe_p05` | 4.544 | +0.000333 | +0.000734 | +0.000690 | +0.000493 | +0.000171 | 0 |

All guarded variants are macro-positive.  `cub_safe_p05` has the best guarded
score, but gives up most of M1225's Recall gain.

### Row-Level Shape

Row-level negative metric counts:

| Variant | Negative Rows / 15 | Main remaining failures |
| --- | ---: | --- |
| `m1225_cub_source_abs_top8_s1` | 6 | `cqadupstack`, `fiqa`, `webis-touche2020`, `trec-covid`, `nfcorpus`, `dbpedia-entity` |
| `m1226_cub_safe_p05` | 7 | `cqadupstack`, `fiqa`, `scidocs`, `webis-touche2020`, plus smaller CUB/rank issues |
| `m1226_cub_rank_safe_p05` | 5 | `cqadupstack`, `fiqa`, `scidocs`, `webis-touche2020`, `dbpedia-entity` |
| `m1226_all_safe_p05` | 5 | same as `cub_rank_safe_p05` |
| `m1226_rank_safe_p05` | 5 | `cqadupstack`, `fiqa`, `webis-touche2020`, `dbpedia-entity` |

The guard reduces selected actions and improves macro CUB, but it does not
solve row-level fragility.  The most severe negative row, `cqadupstack`, remains
severe after guarding.

## Interpretation

M1226 is useful but not a breakthrough.

It proves that the M1225 CUB-specific replay signal can be made macro-safe with
a held-out guard.  It also proves that query-level guard features are not enough
to solve the row-level problem.

The important result is therefore structural:

- M1224/M1225 found a better teacher/candidate label surface.
- M1226 shows post-hoc query gating cannot fully repair that surface into a
  robust policy.
- The next step should not be another threshold or label-surface gate sweep.

## Decision

Keep M1225 as the strongest macro signal.

Keep M1226 as a safer but weaker deployment-shaped variant.

Do not continue hand-tuning the row-level guard.  The next valid step is to move
the CUB-specific teacher into a generated-posting training objective, or to
change candidate construction for the known failure rows.  In either case, the
acceptance gate must include row-level negative-count reduction, not only macro
all-positive metrics.

## Artifacts

- Script: `scripts/replay_m1226_guarded_cub_specific_native.py`
- Smoke JSON: `runs/m1226_guarded_cub_specific_native_smoke_v1/m1226_guarded_cub_specific_native.json`
- Full JSON: `runs/m1226_guarded_cub_specific_native_v1/m1226_guarded_cub_specific_native.json`
