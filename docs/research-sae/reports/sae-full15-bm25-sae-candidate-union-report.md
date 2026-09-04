# SAE Full15 BM25+SAE Candidate Union Report

## Scope

This pass tests the next unified-index hypothesis:

```text
BM25 token candidates + SAE atom candidates
  -> one candidate pool
  -> exact normalized BM25+SAE rerank
```

The purpose is to check whether BM25 token evidence fills the residual misses
from the SAE-only candidate path without requiring a large SAE atom candidate
budget.

Input student:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-token-char/
```

Evaluator:

```text
scripts/research_sae_bm25_sae_candidate_union.py
```

Generated outputs:

```text
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-bm25-sae-candidate-union/
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-bm25-sae-candidate-union-cost-sweep/
```

The Python query time in this report is diagnostic only. The script computes
exact rerank scores in Python and is not a native index performance estimate.

## Current Training Matrix

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 baseline | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| Snowflake-SAE teacher | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| Dense-pair student previous | 0.8232 | 0.8231 | 0.7086 | 0.6778 |
| Budget16 loss `0.01` | 0.8280 | 0.8264 | 0.7154 | 0.6837 |
| Budget16 loss `0.04` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| Budget16 loss `0.08` | 0.8327 | 0.8255 | 0.7223 | 0.6891 |

Decision from the training matrix: keep `budget16 loss=0.04` as the balanced
frontier. `0.08` gives a tiny Recall@100 gain but loses MRR@20 and MAP@100.

## Exact BM25+SAE Reference

The exact full BM25+SAE student path for the current balanced frontier is:

| Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| ---: | ---: | ---: | ---: |
| 0.8324 | 0.8291 | 0.7225 | 0.6913 |

## First Candidate Union Sweep

This sweep used SAE configs `d16_p128` and `d32_p128`, plus BM25 top-k
candidates.

| Config | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `d16_p128_bm25300` | 0.8327 | 0.8291 | 0.7225 | 0.6921 | 1112.8 | 1443.7 |
| `d16_p128_bm25200` | 0.8326 | 0.8291 | 0.7224 | 0.6920 | 1061.8 | 1443.7 |
| `d16_p128_bm25100` | 0.8321 | 0.8291 | 0.7223 | 0.6910 | 1011.8 | 1443.7 |
| `d16_p128_bm2550` | 0.8316 | 0.8291 | 0.7220 | 0.6885 | 988.2 | 1443.7 |
| `d16_p128_bm250` | 0.8140 | 0.8239 | 0.7123 | 0.6713 | 968.7 | 1443.7 |

BM25 top candidates clearly fill the residual SAE-only misses. With only
`100` BM25 lexical candidates, `d16_p128` nearly matches the exact BM25+SAE
quality surface.

## Cost-Focused Sweep

The second sweep added lower SAE cost points:

```text
active_dims:       8, 16, 32
postings_per_dim:  64, 128
bm25_candidate_k:  50, 100, 150, 200, 300
```

Top recall rows:

| Config | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `d32_p64_bm25200` | 0.8328 | 0.8291 | 0.7224 | 0.6922 | 1032.4 | 1378.0 |
| `d32_p64_bm25150` | 0.8327 | 0.8291 | 0.7225 | 0.6919 | 1007.3 | 1378.0 |
| `d16_p128_bm25300` | 0.8327 | 0.8291 | 0.7225 | 0.6921 | 1112.8 | 1443.7 |
| `d16_p128_bm25200` | 0.8326 | 0.8291 | 0.7224 | 0.6920 | 1061.8 | 1443.7 |
| `d8_p128_bm25200` | 0.8325 | 0.8291 | 0.7223 | 0.6913 | 739.1 | 781.7 |

Low-candidate rows below `900` candidates/query:

| Config | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `d8_p128_bm25200` | 0.8325 | 0.8291 | 0.7223 | 0.6913 | 739.1 | 781.7 |
| `d8_p128_bm25150` | 0.8318 | 0.8291 | 0.7223 | 0.6906 | 706.7 | 781.7 |
| `d16_p64_bm25200` | 0.8312 | 0.8291 | 0.7225 | 0.6919 | 765.2 | 850.5 |
| `d16_p64_bm25150` | 0.8311 | 0.8291 | 0.7224 | 0.6915 | 733.7 | 850.5 |
| `d8_p64_bm25300` | 0.8286 | 0.8291 | 0.7226 | 0.6915 | 606.2 | 454.4 |
| `d8_p64_bm25200` | 0.8275 | 0.8291 | 0.7225 | 0.6909 | 531.7 | 454.4 |

## Findings

BM25 lexical candidates are not just a second-stage quality add-on. In this
simulation they allow the SAE side to use a much smaller candidate budget while
preserving the exact BM25+SAE quality surface.

The most important current point is:

```text
d8_p128_bm25200
```

It matches the exact BM25+SAE quality within noise:

| Metric | Exact | Candidate Union | Delta |
| --- | ---: | ---: | ---: |
| Recall@100 | 0.8324 | 0.8325 | +0.0001 |
| MRR@20 | 0.8291 | 0.8291 | 0.0000 |
| NDCG@10 | 0.7225 | 0.7223 | -0.0002 |
| MAP@100 | 0.6913 | 0.6913 | 0.0000 |

It does so with about `739` candidate documents, `782` SAE postings, and `200`
BM25 lexical candidates per query.

The aggressive low-cost point is:

```text
d8_p64_bm25200
```

This drops to about `532` candidates and `454` SAE postings, but Recall@100
falls to `0.8275`. It may be useful as a latency-first tier, but it is not the
balanced frontier.

## Decision

Promote BM25+SAE candidate union as the next active physical-design direction.

Current candidate targets:

```text
balanced:      d8_p128_bm25200
safe fallback: d16_p128_bm25100
latency tier:  d8_p64_bm25200
```

The next engineering step is to port this union logic into the native payload
simulator/read-only PostgreSQL path so the cost model moves from Python
diagnostics to index-structure counters.
