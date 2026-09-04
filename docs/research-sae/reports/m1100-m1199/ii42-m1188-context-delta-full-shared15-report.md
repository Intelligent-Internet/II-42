# M1188 Context Delta Full Shared15 Replay

## Objective

M1188 applies M1187 qrels-free context-candidate atom deltas to all shared15
qrels queries, not only M675 event queries. This tests whether the event-query
signal survives broader regression.

## Setup

- Surface: shared15 qrels queries.
- Query count: `1342`.
- Native DB path: same P1/BM25 hybrid scorer used by M673/M674.
- Candidate variants:
  - `bm25_top_docs3_append8_s0.1_shared1`
  - `bm25_tail_docs3_append8_s0.1_shared1`
- Follow-up grid:
  - `bm25_top` only;
  - docs `1,3`;
  - scale `0.05,0.10`.

## Main Full Replay

Output:

- `runs/m1188_context_delta_full_shared15_v1/m1188_context_delta_full_shared15.json`
- `runs/m1188_context_delta_full_shared15_v1/m1188_context_delta_full_shared15.md`

| Source | Queries | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline` | 1342 | 0.864763 | 0.667856 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| `m674` | 1342 | 0.870086 | 0.668196 | 0.737286 | 0.820878 | 0.947106 | 1.000000 |
| `bm25_top_docs3_append8_s0.1_shared1` | 1342 | 0.866710 | 0.671564 | 0.740557 | 0.823026 | 0.947023 | 0.969503 |
| `bm25_tail_docs3_append8_s0.1_shared1` | 1342 | 0.866196 | 0.668118 | 0.737249 | 0.820347 | 0.947451 | 0.960515 |

### Deltas vs Baseline

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm25_top_docs3_append8_s0.1_shared1` | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 | 0.969503 |
| `bm25_tail_docs3_append8_s0.1_shared1` | +0.001433 | +0.000262 | -0.000037 | -0.000531 | +0.000345 | 0.960515 |

The `bm25_top_docs3` variant is globally useful for ranking metrics but has a
very small CUB regression. The `bm25_tail_docs3` variant preserves CUB but is
not ranking-clean.

## BM25-Top Grid

Output:

- `runs/m1188_context_delta_full_shared15_bm25top_grid_v1/m1188_context_delta_full_shared15.json`
- `runs/m1188_context_delta_full_shared15_bm25top_grid_v1/m1188_context_delta_full_shared15.md`

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm25_top_docs3_append8_s0.1_shared1` | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 | 0.969503 |
| `bm25_top_docs3_append8_s0.05_shared1` | +0.001281 | +0.002477 | +0.002085 | +0.002092 | -0.000093 | 0.983944 |
| `bm25_top_docs1_append8_s0.1_shared1` | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 | 0.985403 |
| `bm25_top_docs1_append8_s0.05_shared1` | -0.000033 | +0.001256 | +0.000880 | +0.001410 | -0.000028 | 0.992360 |

`bm25_top_docs1_append8_s0.1_shared1` is the conservative all-positive variant:
Recall, MAP, NDCG, MRR, and CUB are all positive, with Top95 overlap 0.9854.
The recall gain is smaller, but this is the first full-query, qrels-free,
native unified-posting delta with no macro metric regression in the measured
matrix.

## Dataset Notes

For `bm25_top_docs3_append8_s0.1_shared1`, most rows improve or stay flat, but
the main visible harm is `cqadupstack`:

- `cqadupstack`: dRecall `-0.001195`, dMAP `-0.006696`,
  dNDCG `-0.003987`, dMRR `-0.011729`.
- `scidocs`: dRecall `+0.010000`, dMAP `+0.005961`, but CUB `-0.002000`.
- `nfcorpus`: dRecall `+0.002942`, dMAP `+0.005138`,
  dNDCG `+0.010881`, CUB `-0.000614`.

This says the structure works, but needs a qrels-free safety gate before it can
be promoted as a default.

## Decision

M1188 is a real positive structural signal. It is not the final breakthrough
matrix yet.

Keep two candidates:

1. Aggressive candidate:
   `bm25_top_docs3_append8_s0.1_shared1`
   - best ranking gains;
   - CUB and row-level harm need gating.
2. Conservative candidate:
   `bm25_top_docs1_append8_s0.1_shared1`
   - all macro metrics positive including CUB;
   - smaller recall gain;
   - good default for safety-first replay.

Next step should not be another blind training run. It should be M1189:

- qrels-free head-overlap / perturbation-risk gate;
- compare baseline, aggressive, conservative, and gated aggressive;
- accept only if full shared15 retains ranking gains while removing CUB and
  `cqadupstack`-style row harm.
