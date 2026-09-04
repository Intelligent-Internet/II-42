# SAE Full15 Candidate-Budget Physical Cost Report

## Scope

This pass evaluates whether the current best text-to-atoms student also works
as a physical candidate-generation representation.

Input student:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-token-char/
```

Temporary evaluator data root:

```text
/tmp/ii42_sae_full15_budget16_w0p04_physical/
```

Evaluator output:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-candidate-rerank/
```

Important scope limitation: this evaluator measures the SAE candidate path by
itself. It does not include the BM25 token side of the final unified sparse
index. The purpose is to test whether top query atoms can open a small enough
candidate set before exact SAE rerank.

## Method

The evaluator selects a limited number of query atoms, opens only the highest
impact postings for those atoms, and reranks the candidate documents with the
full original query atom vector.

Sweep:

```text
active_dims:       8, 16, 32
postings_per_dim:  16, 32, 64, 128
strategy:          top_weight
top_k:             100
datasets:          full 15-BEIR matrix
```

## Macro Results

The full exact SAE student path is the candidate-rerank upper bound in this
report:

```text
Recall@100 0.7278
MRR@20     0.4870
```

| Config | Recall@100 | MRR@20 | Candidates | Postings | Exact@100 Overlap | Query ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `d32_p128` | 0.7279 | 0.4904 | 1326.6 | 2263.6 | 0.9745 | 4.329 |
| `d16_p128` | 0.7250 | 0.4864 | 968.7 | 1443.7 | 0.9534 | 3.157 |
| `d32_p64` | 0.7164 | 0.4868 | 941.5 | 1378.0 | 0.9092 | 3.093 |
| `d8_p128` | 0.6991 | 0.4841 | 615.4 | 781.7 | 0.8481 | 2.034 |
| `d16_p64` | 0.6959 | 0.4812 | 646.5 | 850.5 | 0.8528 | 2.116 |
| `d32_p32` | 0.6612 | 0.4800 | 606.0 | 783.9 | 0.7293 | 1.975 |
| `d8_p64` | 0.6122 | 0.4646 | 386.9 | 454.4 | 0.6739 | 1.283 |
| `d16_p32` | 0.5703 | 0.4642 | 386.1 | 462.3 | 0.6298 | 1.274 |
| `d32_p16` | 0.5319 | 0.4580 | 361.1 | 428.8 | 0.4926 | 1.188 |
| `d8_p32` | 0.4319 | 0.4186 | 218.6 | 242.5 | 0.4371 | 0.734 |
| `d16_p16` | 0.3990 | 0.4113 | 214.6 | 241.6 | 0.3933 | 0.729 |
| `d8_p16` | 0.2577 | 0.3429 | 116.5 | 124.7 | 0.2502 | 0.406 |

## Findings

`d16_p128` is the best balanced first physical point. It preserves almost all
of the exact SAE candidate-path quality:

```text
Recall@100 delta: -0.0028
MRR@20 delta:     -0.0006
```

It does so with about `969` candidate documents and `1444` postings touched per
query on the full15 macro average. This is still not a final product cost, but
it is low enough to justify continuing the unified sparse physical design.

`d32_p128` is effectively exact quality, but costs about `1327` candidates and
`2264` postings per query. It is useful as a high-recall fallback point.

Configs below `pdim=64` lose too much Recall@100. They are useful only as lower
bound probes, not as current product candidates.

## Decision

Keep candidate-budget training as the active direction.

Promote two physical candidate targets:

```text
balanced:     active_dims=16, postings_per_dim=128
high-recall:  active_dims=32, postings_per_dim=128
```

The next work should connect this candidate path back to the BM25 side of the
unified sparse index, then measure BM25-token atoms plus SAE atoms together.
The key question is whether BM25 token postings fill the residual misses from
the SAE-only candidate path without materially increasing candidate cost.
