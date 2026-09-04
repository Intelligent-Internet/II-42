# SAE Block-Max Phase 4.11 V4 Quality/Cost Sweep Report

Date: 2026-05-12

## Purpose

Phase 4.10 made the v4 native candidate-rerank path physically plausible:

```text
query top-weight dimensions
  -> read dimension-local impact heads
  -> union candidate doc ords
  -> exact rerank through row-wise doc vectors
```

That benchmark measured exact-SAE overlap and C counters, but not real qrels quality. Phase 4.11 joins the native v4 cost surface with the Python candidate-budget qrels evaluator so the next read-only PostgreSQL payload target can be selected from one table.

The sweep is intentionally limited to `top_weight` selection because this is the only candidate policy currently implemented by the standalone C reader.

## Implementation

New script:

```text
scripts/research_sae_v4_quality_cost_sweep.py
```

Small supporting change:

```text
scripts/research_sae_native_candidate_rerank_benchmark.py
```

The native benchmark now accepts `--impact-head-size`, which separates the physical v4 head-size storage parameter from the candidate config. The combined sweep reuses:

- `research_sae_candidate_budget_rerank.evaluate_config` for qrels quality;
- `research_sae_native_candidate_rerank_benchmark.run_reader` for C timings and counters;
- the v4 packed payload exporter for `include_doc_vectors + impact_head_size`.

## Sweep Configuration

```text
datasets = scifact, scidocs, nfcorpus, arguana, fiqa
layout = sae_overlap_greedy
micro_block_size = 1
score_mode = normalized_idf_dot
top_k = 100
repeat = 3
head_sizes = 16, 24, 32
configs = d8/d12/d16 x pdim16/pdim24/pdim32
```

For a given head size, configs with `postings_per_dim > head_size` are skipped because the payload cannot represent those posting heads.

## Full Exact Qrels Baseline

| Dataset | Docs | Queries | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | 2000 | 100 | `0.9500` | `0.7206` |
| `scidocs` | 2000 | 100 | `0.7025` | `0.5973` |
| `nfcorpus` | 2063 | 100 | `0.3590` | `0.6546` |
| `arguana` | 2000 | 100 | `1.0000` | `0.4399` |
| `fiqa` | 2000 | 100 | `0.9126` | `0.7218` |

## Aggregate Results

| Head | Config | dR@100 | dMRR@20 | C seconds | Candidates | Exact overlap | Gen visits | Gen decoded | Generation MB |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 32 | `d16_pdim32` | `-0.0050` | `+0.0008` | `0.053887` | `377.9` | `0.7727` | `16.0` | `497.8` | `2.47` |
| 32 | `d12_pdim32` | `-0.0099` | `+0.0010` | `0.047881` | `293.0` | `0.6874` | `12.0` | `373.5` | `2.47` |
| 32 | `d16_pdim24` | `-0.0133` | `+0.0010` | `0.048395` | `296.0` | `0.6915` | `16.0` | `378.1` | `2.47` |
| 24 | `d16_pdim24` | `-0.0133` | `+0.0010` | `0.049555` | `296.0` | `0.6915` | `16.0` | `378.1` | `2.40` |
| 24 | `d12_pdim24` | `-0.0257` | `+0.0006` | `0.045166` | `227.5` | `0.6053` | `12.0` | `283.6` | `2.40` |
| 16 | `d16_pdim16` | `-0.0307` | `-0.0014` | `0.042484` | `206.9` | `0.5750` | `16.0` | `254.4` | `2.31` |

The full generated summary and JSON are under:

```text
results/sae/phase411/v4-quality-cost-sweep/
```

The binary payloads are diagnostic artifacts and are not tracked by git.

## Dataset-Level Check

The two most useful rows behave differently:

| Dataset | Config | dR@100 | dMRR@20 | Candidates | Exact overlap |
| --- | --- | ---: | ---: | ---: | ---: |
| `arguana` | `d12_pdim32` | `-0.0100` | `-0.0006` | `254.6` | `0.7151` |
| `arguana` | `d16_pdim32` | `-0.0100` | `-0.0006` | `332.8` | `0.7958` |
| `fiqa` | `d12_pdim32` | `-0.0142` | `+0.0001` | `294.5` | `0.7037` |
| `fiqa` | `d16_pdim32` | `+0.0007` | `+0.0013` | `378.6` | `0.7882` |
| `nfcorpus` | `d12_pdim32` | `+0.0047` | `+0.0021` | `301.6` | `0.6744` |
| `nfcorpus` | `d16_pdim32` | `+0.0025` | `+0.0015` | `387.9` | `0.7565` |
| `scidocs` | `d12_pdim32` | `-0.0200` | `+0.0029` | `305.7` | `0.6692` |
| `scidocs` | `d16_pdim32` | `-0.0180` | `+0.0009` | `393.2` | `0.7611` |
| `scifact` | `d12_pdim32` | `-0.0100` | `+0.0005` | `307.8` | `0.6739` |
| `scifact` | `d16_pdim32` | `+0.0000` | `+0.0008` | `396.7` | `0.7618` |

`d16_pdim32` is the conservative quality row. `d12_pdim32` is the better current default candidate because it reduces candidates and rerank work materially while keeping qrels loss near one recall point on the five-dataset mean.

## Decision

Use `head_size = 32`, `active_dims = 12`, and `postings_per_dim = 32` as the next read-only PostgreSQL payload target.

Keep `head_size = 32`, `active_dims = 16`, and `postings_per_dim = 32` as the conservative fallback if later workloads show unacceptable recall loss.

Do not pursue `head_size = 16` as the default: the storage win is small relative to the row-wise doc-vector copy, while recall loss grows to about three points even for the best compact row.

## Next Step

Port the selected v4 candidate-rerank path into the read-only PostgreSQL payload:

1. Generate the compact resident payload with `impact_head_size = 32`.
2. Expose query-time parameters for `active_dims = 12` and `postings_per_dim = 32`.
3. Return TID/score/diagnostics from the PostgreSQL function using the same native traversal contract.
4. Keep mutable delta overlay and maintenance out of scope until this read-only path is stable.

The remaining structural concern is storage: v4 duplicates the sparse impact stream in document-major order. That is acceptable for the next read-only prototype, but should be compressed before a production mutable design.

## Verification

Commands run:

```bash
python3 -m py_compile \
  scripts/research_sae_native_candidate_rerank_benchmark.py \
  scripts/research_sae_v4_quality_cost_sweep.py

python3 scripts/research_sae_v4_quality_cost_sweep.py \
  --datasets scifact \
  --head-sizes 16,32 \
  --configs 8:16,16:32 \
  --repeat 1 \
  --output-dir results/sae/phase411/v4-quality-cost-smoke

python3 scripts/research_sae_v4_quality_cost_sweep.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --head-sizes 16,24,32 \
  --configs 8:16,8:24,8:32,12:16,12:24,12:32,16:16,16:24,16:32 \
  --repeat 3 \
  --output-dir results/sae/phase411/v4-quality-cost-sweep
```
