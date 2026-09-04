# M1187 Context-Candidate Delta Replay

## Objective

M676 proved that oracle qrels-positive document atoms can move event-query
rankings through the native unified-posting scorer. M1187 removes that oracle:
proposal atoms are selected only from qrels-free native retrieval context.

This is the first retrieval-conditioned generated-posting audit after closing
the query-only M1183-M1186 branch.

## Setup

- Surface: M675 event queries.
- Query count: `136`.
- Baseline: P1-a0125 native same-query fusion.
- Reference rule: M674 top95-preserving BM25 tail admission.
- Proposal sources:
  - `bm25_top`
  - `bm25_tail`
  - `p1_tail`
  - `fused_tail`
- Delta shape:
  - append top `8` proposal-doc atoms;
  - scale `0.05` or `0.10`;
  - no shared boost in the selected run.

## Result

Full output:

- `runs/m1187_context_candidate_delta_replay_v1/m1187_context_candidate_delta_summary.json`
- `runs/m1187_context_candidate_delta_replay_v1/m1187_context_candidate_delta_report.md`

### Event-Query Surface

| Source | Queries | Target hit@100 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline` | 136 | 0.000000 | 0.408907 | 0.333115 | 0.657345 | 0.891506 | 0.765747 | 1.000000 |
| `m674` | 136 | 1.000000 | 0.470810 | 0.337941 | 0.657345 | 0.891506 | 0.765747 | 1.000000 |
| `bm25_tail_docs3_append8_s0.1_shared1` | 136 | 0.190564 | 0.428599 | 0.338428 | 0.662384 | 0.889274 | 0.766331 | 0.966486 |
| `bm25_top_docs3_append8_s0.1_shared1` | 136 | 0.101348 | 0.423939 | 0.338523 | 0.665458 | 0.892936 | 0.765811 | 0.972446 |

### Best Deltas vs Baseline

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm25_tail_docs3_append8_s0.1_shared1` | +0.019692 | +0.005313 | +0.005039 | -0.002232 | +0.000584 | 0.966486 |
| `bm25_top_docs3_append8_s0.1_shared1` | +0.015032 | +0.005409 | +0.008113 | +0.001430 | +0.000064 | 0.972446 |

## Interpretation

M1187 is the first strong evidence that retrieval-conditioned generated
posting has a qrels-free deployable shape:

- `bm25_top_docs3_append8_s0.1_shared1` improves Recall, MAP, NDCG, MRR, and
  CUB on M675 event queries.
- It uses only native retrieval context, not qrels-positive oracle doc atoms.
- It is still weaker than deterministic M674 target-hit movement, but unlike
  M674 it changes the unified posting query itself.

The generated report selected `bm25_tail` first because it emphasizes target
hit@100. For deployment feasibility, `bm25_top_docs3_append8_s0.1_shared1` is
the better candidate because all ranking metrics move positive.

## Decision

Proceed to full shared15 qrels-query regression before any training. The
candidate to test is:

```text
bm25_top_docs3_append8_s0.1_shared1
```

Use `bm25_tail_docs3_append8_s0.1_shared1` only as an event-local diagnostic,
because its MRR tradeoff is not acceptable as a global default.
