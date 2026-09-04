# SAE Block-Max Phase 4.9 Doc-Vector Candidate Rerank Report

Date: 2026-05-12

## Purpose

Phase 4.8 proved that a native approximate candidate-generator plus full-query
exact rerank can reproduce the Phase 4.7 candidate counts and overlap, but the
prototype spent too much time in rerank:

```text
candidate docs * query dimensions -> dimension-range binary lookup
```

Phase 4.9 removes that bottleneck by adding a row-wise compact document vector
section to the packed payload:

```text
doc_ord -> sorted (global_dim_id, impact) pairs
```

The exact reader path remains unchanged. The new section is used only by the
approximate candidate-rerank path.

## Payload Change

Packed micro-block payloads now support `SBMXM001` version `3`.

Version `1` remains compatible:

```text
header
doc table
dimension directory
block-id ordered micro-block entries
impact stream
query harness
```

Version `3` adds:

```text
doc_vector_pair_count
doc_vector_starts[doc_count + 1]
doc_vector_pairs[doc_vector_pair_count]
```

Each document vector row is sorted by `global_dim_id`, so rerank can merge-scan
the query dimensions and the document dimensions.

## Implementation

Updated files:

```text
scripts/research_sae_block_max_export_packed_microblock.py
scripts/research_sae_packed_microblock_benchmark.py
scripts/research_sae_native_candidate_rerank_benchmark.py
tests/research_sae_packed_microblock_reader.c
```

The native benchmark now exports v3 payloads by default for the candidate-rerank
path. The general packed micro-block benchmark still exports v1 unless
`--include-doc-vectors` is explicitly passed through the helper.

## Repeat-3 Benchmark

Configuration:

```text
layout = sae_overlap_greedy
micro_block_size = 1
top_k = 100
repeat = 3
payload = SBMXM001 v3 with doc vectors
```

Exact v3 baseline:

| Dataset | Exact | C seconds | Block-entry visits | Decoded postings |
| --- | ---: | ---: | ---: | ---: |
| `scifact` | `100/100` | `0.103729` | `7761.96` | `1070.78` |
| `scidocs` | `100/100` | `0.044189` | `7318.08` | `1054.37` |
| `nfcorpus` | `100/100` | `0.042553` | `6761.80` | `942.20` |
| `arguana` | `100/100` | `0.042927` | `7719.65` | `1287.02` |
| `fiqa` | `100/100` | `0.041633` | `6620.86` | `1071.59` |

Five-dataset means:

| Path | C seconds | Candidates | Exact@100 overlap | Generator visits | Rerank binary steps | Rerank doc terms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| exact doc-bound | `0.055006` | `100 opened` | `1.0000` | `7236.47` | `0.00` | `0.00` |
| `d16_pdim32` | `0.056798` | `377.93` | `0.7727` | `1845.36` | `0.00` | `45426.80` |
| `d12_pdim32` | `0.050509` | `292.96` | `0.6874` | `1383.06` | `0.00` | `35136.11` |
| `d16_pdim24` | `0.051059` | `296.02` | `0.6915` | `1845.36` | `0.00` | `35510.15` |

Compared with Phase 4.8, `d16_pdim32` improves from about `0.091s` to
`0.057s`, and rerank binary steps drop from about `160k` per query to `0`.

The cost is payload size. On the scifact slice, generation bytes increase from
about `1.08 MB` for the v1 exact generation to about `2.10 MB` with doc vectors.
That is expected because v3 keeps both the dimension-major impact stream and
the row-wise document-major copy.

## Decision

This is a successful native payload step. Candidate rerank is no longer the
dominant measured bottleneck.

The remaining avoidable cost is candidate generation:

```text
selected query dimensions
  -> scan block-id ordered dimension range
  -> keep top impact postings
```

The next payload step should add an impact-head directory:

```text
global_dim_id
  -> head_start
  -> head_count
  -> doc_ord, impact pairs sorted by impact desc
```

That should let candidate generation touch only the requested posting head
instead of scanning the full dimension range. After that, repeat the same
benchmark to check whether approximate `d16_pdim32` becomes clearly faster than
the exact doc-bound baseline while preserving Phase 4.7 quality.

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
  --output-dir results/sae/phase49/doc-vector-smoke-v1

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact \
  --configs 16:32 \
  --repeat 1 \
  --output-dir results/sae/phase49/doc-vector-smoke

python3 scripts/research_sae_native_candidate_rerank_benchmark.py \
  --datasets scifact scidocs nfcorpus arguana fiqa \
  --configs 16:32,12:32,16:24 \
  --repeat 3 \
  --output-dir results/sae/phase49/doc-vector-candidate-rerank-repeat3
```
