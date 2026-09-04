# SAE Block-Max Phase 4.10 Impact-Head Candidate Rerank Report

Date: 2026-05-12

## Purpose

Phase 4.9 removed the candidate-rerank binary lookup bottleneck with row-wise
document vectors. The remaining avoidable work was candidate generation:

```text
selected query dimensions
  -> scan full block-id ordered dimension range
  -> keep the top impact postings for that dimension
```

Phase 4.10 adds a dimension-local impact-head directory so the approximate
candidate generator can read the requested posting head directly.

## Payload Change

Packed micro-block payloads now support `SBMXM001` version `4`.

Version `4` keeps the version `3` row-wise document vectors and adds:

```text
impact_head_pair_count
impact_head_size
impact_head_starts[max_global_dim_id + 2]
impact_head_pairs[impact_head_pair_count]
```

Each impact-head pair is:

```text
doc_ord, impact
```

Pairs are sorted by descending impact within each dimension, with document-id
tie ordering preserved through `tie_ord`.

## Implementation

Updated files:

```text
scripts/research_sae_block_max_export_packed_microblock.py
scripts/research_sae_packed_microblock_benchmark.py
scripts/research_sae_native_candidate_rerank_benchmark.py
tests/research_sae_packed_microblock_reader.c
```

The native candidate-rerank benchmark now exports v4 payloads with
`impact_head_size = max(postings_per_dim)` across the configured approximate
runs. Version `1` exact payloads remain readable, and version `3` still falls
back to block-id range scanning for candidate generation.

## Smoke Result

On `scifact`, `d16_pdim32` changes the candidate generator counters from Phase
4.9:

| Counter | Phase 4.9 doc-vector | Phase 4.10 impact-head |
| --- | ---: | ---: |
| Generator visits | `1982.98` | `16.00` |
| Generator decoded postings | `1982.98` | `500.78` |
| Rerank binary steps | `0.00` | `0.00` |
| Candidates | `396.67` | `396.67` |
| Exact@100 overlap | `0.7618` | `0.7618` |

The semantic result is unchanged; only the physical generation path changes.

## Repeat-3 Benchmark

Configuration:

```text
layout = sae_overlap_greedy
micro_block_size = 1
top_k = 100
repeat = 3
payload = SBMXM001 v4
impact_head_size = 32
```

Five-dataset means:

| Path | C seconds | Candidates | Exact@100 overlap | Generator visits | Generator decoded | Rerank doc terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| exact doc-bound v4 | `0.057153` | `100 opened` | `1.0000` | `7236.47` | `1085.19` | `0.00` |
| `d16_pdim32` | `0.052813` | `377.93` | `0.7727` | `16.00` | `497.84` | `45426.80` |
| `d12_pdim32` | `0.047180` | `292.96` | `0.6874` | `12.00` | `373.52` | `35136.11` |
| `d16_pdim24` | `0.058429` | `296.02` | `0.6915` | `16.00` | `378.05` | `35510.15` |

Repeat-5 confirmed the same structural counters, but one host-side timing spike
hit `arguana`; do not over-interpret the repeat-5 wall-time means.

## Storage Cost

Five-dataset mean v4 generation shape:

| Metric | Mean |
| --- | ---: |
| Generation bytes | `2474891` |
| Doc-vector pairs | `128806` |
| Impact-head pairs | `43657` |

On the `scifact` slice:

| Payload | Generation bytes |
| --- | ---: |
| v1 exact | about `1.08 MB` |
| v3 doc vectors | `2.10 MB` |
| v4 doc vectors + impact heads | `2.45 MB` |

The impact-head directory adds modest storage relative to the row-wise
document-vector duplicate. The bigger storage decision remains whether the
document-major copy is acceptable for the target production scale.

## Decision

This is a successful physical index step. The approximate native path now avoids
both previously measured structural bottlenecks:

- rerank no longer performs dimension-range binary lookups;
- candidate generation no longer scans full block-id ordered dimension ranges.

The next useful work should move from physical plumbing back to retrieval
policy:

1. Run the v4 native path against the qrels metrics from Phase 4.7, not only
   exact-SAE overlap, so the quality/cost tradeoff can be selected in one table.
2. Sweep `impact_head_size` and candidate configs together, because v4 makes
   head size a physical storage parameter.
3. Consider compressing the duplicated document-vector stream after the quality
   budget is selected.

Do not start mutable maintenance yet. The read-only payload is still changing.

## Verification

Commands run:

```bash
python3 -m py_compile \
  scripts/research_sae_block_max_export_packed_microblock.py \
  scripts/research_sae_packed_microblock_benchmark.py \
  scripts/research_sae_native_candidate_rerank_benchmark.py

cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  tests/research_sae_packed_microblock_reader.c \
  -o /tmp/research_sae_packed_microblock_reader

python3 scripts/research_sae_packed_microblock_benchmark.py \
  --datasets scifact \
  --micro-block-size 1 \
  --layout sae_overlap_greedy \
  --output-dir results/sae/phase410/impact-head-smoke-v1

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact \
  --configs 16:32,12:32 \
  --repeat 1 \
  --output-dir results/sae/phase410/impact-head-smoke

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --configs 16:32,12:32,16:24 \
  --repeat 3 \
  --output-dir results/sae/phase410/impact-head-candidate-rerank-repeat3

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --configs 16:32,12:32,16:24 \
  --repeat 5 \
  --output-dir results/sae/phase410/impact-head-candidate-rerank-repeat5
```
