# SAE Block-Max Phase 4.8 Native Candidate Rerank Report

Date: 2026-05-12

## Purpose

Phase 4.8 ports the Phase 4.7 approximate candidate-budget idea into the
standalone C packed micro-block reader.

The goal is not to replace the exact read-only path yet. The goal is to verify
that the native traversal can execute this shape:

```text
selected query dimensions
  -> collect high-impact candidate postings
  -> union candidate docs
  -> rerank candidates with all original query dimensions
```

The exact doc-bound path remains the correctness reference and default reader
mode. The approximate path is enabled only by explicit reader flags.

## Implementation

Updated reader:

```text
tests/research_sae_packed_microblock_reader.c
```

New benchmark:

```text
scripts/research_sae_native_candidate_rerank_benchmark.py
```

Example approximate invocation:

```bash
research_sae_packed_microblock_reader packed_payload.bin \
  --approx-active-dims 16 \
  --approx-postings-per-dim 32
```

The C prototype currently uses the existing block-id ordered payload. For each
selected query dimension it scans that dimension range and keeps the top impact
postings in memory. This validates semantics, but it is not the final payload
layout. A production layout needs an impact-head directory so generation can
read the posting head directly.

The rerank step scores every candidate with the full query by binary-searching
the existing dimension range for `(dimension, doc_ord)`. This is also a
prototype choice. It exposes why a row-wise compact document vector is needed
for the next stage.

## Repeat-3 Benchmark

Configuration:

```text
layout = sae_overlap_greedy
micro_block_size = 1
top_k = 100
repeat = 3
```

Exact baseline:

| Dataset | Exact | C seconds | Block-entry visits | Decoded postings |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | `100/100` | `0.083503` | `7761.96` | `1070.78` |
| `scidocs` | `100/100` | `0.036017` | `7318.08` | `1054.37` |
| `nfcorpus` | `100/100` | `0.056909` | `6761.80` | `942.20` |
| `arguana` | `100/100` | `0.035122` | `7719.65` | `1287.02` |
| `fiqa` | `100/100` | `0.034267` | `6620.86` | `1071.59` |

Five-dataset means:

| Path | C seconds | Candidates | Exact@100 overlap | Generator visits | Rerank binary steps |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact doc-bound | `0.049164` | `100.00 opened` | `1.0000` | `7236.47` | `0.00` |
| `d16_pdim32` | `0.091059` | `377.93` | `0.7727` | `1845.36` | `159744.81` |
| `d12_pdim32` | `0.073004` | `292.96` | `0.6874` | `1383.06` | `123821.21` |
| `d16_pdim24` | `0.072920` | `296.02` | `0.6915` | `1845.36` | `125092.10` |

## Decision

The native candidate-generator semantics are correct and match Phase 4.7
candidate counts and full-SAE overlap for the same `top_weight` configs.

The current payload shape is not sufficient for a fast approximate mode:

- candidate generation already reduces dimension visits from about `7236` to
  `1383-1845`, but still scans block-id ordered ranges to find impact heads;
- exact rerank dominates the new path because each candidate/query term uses a
  binary lookup, producing roughly `124k-160k` binary steps per query.

The next implementation should not tune candidate counts further. It should
change the payload:

1. Add a row-wise compact document vector:

```text
doc_ord
  -> sorted (global_dim_id, impact) pairs
```

This lets rerank score each candidate by scanning the document's 64-ish active
SAE dimensions instead of doing `candidate_count * query_dim_count` binary
lookups into dimension ranges.

2. Add an optional impact-head directory:

```text
global_dim_id
  -> top impact postings by dimension
```

This lets candidate generation read the requested head directly instead of
scanning the whole dimension range.

The row-wise document vector should come first because the measured bottleneck
is rerank lookup cost.

## Verification

Commands run:

```bash
cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  tests/research_sae_packed_microblock_reader.c \
  -o /tmp/research_sae_packed_microblock_reader

python3 -m py_compile \
  scripts/research_sae_native_candidate_rerank_benchmark.py

python3 scripts/research_sae_packed_microblock_benchmark.py \
  --datasets scifact \
  --micro-block-size 1 \
  --layout sae_overlap_greedy \
  --output-dir results/sae/phase48/native-candidate-rerank-smoke

/tmp/research_sae_packed_microblock_reader \
  results/sae/phase48/native-candidate-rerank-smoke/scifact/packed_payload.bin \
  --approx-active-dims 16 \
  --approx-postings-per-dim 32

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact \
  --configs 16:32,12:32 \
  --output-dir results/sae/phase48/native-candidate-rerank-smoke

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --configs 16:32,12:32,16:24 \
  --output-dir results/sae/phase48/native-candidate-rerank

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --configs 16:32,12:32,16:24 \
  --repeat 3 \
  --output-dir results/sae/phase48/native-candidate-rerank-repeat3
```
