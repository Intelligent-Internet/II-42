# M676 Generated-Posting Delta Feasibility Audit

## Purpose

M676 tests whether M675's retrieval-constrained signal can become a query-side
generated-posting objective.

This is deliberately an oracle feasibility audit. It uses qrels-positive
document atoms from M675 at evaluation time, mutates the query atoms, and runs
the mutated query through the native DB scorer. It is not a deployable model and
must not be reported as a production evaluation.

The question is narrower:

Can a small bounded query atom delta move under-ranked positives through the
native unified posting scorer without collapsing the preserved top95 head?

## Artifacts

- Summary JSON: `runs/m676_generated_posting_delta_feasibility_v1/m676_delta_feasibility_summary.json`
- Generated markdown: `runs/m676_generated_posting_delta_feasibility_v1/m676_delta_feasibility_report.md`
- Script: `scripts/audit_m676_generated_posting_delta_feasibility.py`

## Setup

Input teacher:

- `runs/m675_retrieval_posting_teacher_v1/m675_teacher_events.jsonl`
- 136 event queries
- 340 promoted-positive events

Delta grid:

- append top `4` or `8` positive-doc atoms
- scale appended missing atoms by `0.05` or `0.10`
- optionally boost same-sign shared atoms by `1.25`
- cap query atom count at `192`

All variants are evaluated through the native PostgreSQL scorer path.

## Event-Query Surface

| Source | Queries | Target hit@100 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 136 | 0.000000 | 0.408907 | 0.333115 | 0.657345 | 0.891506 | 1.000000 |
| M674-k95 | 136 | 1.000000 | 0.470810 | 0.337941 | 0.657345 | 0.891506 | 1.000000 |
| best oracle delta | 136 | 0.399142 | 0.441290 | 0.342537 | 0.666077 | 0.900072 | 0.947136 |

Best oracle delta:

- `append8_s0.1_shared1.25`
- Recall@100 delta vs baseline: `+0.032383`
- MAP@100 delta vs baseline: `+0.009422`
- NDCG@10 delta vs baseline: `+0.008732`
- MRR@20 delta vs baseline: `+0.008566`
- Candidate upper bound delta vs baseline: `+0.002958`
- Target hit@100: `0.399142`
- Top95 overlap: `0.947136`

Best conservative variant by head preservation:

- `append8_s0.1_shared1`
- Recall@100 delta vs baseline: `+0.027876`
- MAP@100 delta vs baseline: `+0.007087`
- NDCG@10 delta vs baseline: `+0.007818`
- MRR@20 delta vs baseline: `+0.004543`
- Target hit@100: `0.261520`
- Top95 overlap: `0.971672`

## Interpretation

M676 gives a real positive signal, but not a direct solution.

Positive signal:

- Query-side atom deltas can move native DB rankings in the intended direction.
- The best oracle variant improves Recall, MAP, NDCG, and MRR on event queries.
- The conservative variant still improves all ranking metrics while preserving
  about 97.2% of the top95 head.

Hard limit:

- Raw positive-document atom append does not reproduce M674's target movement.
- M674 reaches target hit@100 of `1.0` by deterministic top95-preserving tail
  admission.
- The best oracle delta reaches only `0.399142`.
- Therefore M676 should not be promoted as a scorer or a production query
  rewrite rule.

The result supports the review direction in a specific way:

- Yes, direct/unified posting remains the right engineering form.
- Yes, dense-only mimic is insufficient.
- No, traditional SAE reconstruction is still not the right main objective.
- No, blindly appending positive-document atoms is not enough.

The useful next step is a trained bounded compiler that learns when and how to
boost/add query atoms, with head preservation as a first-class constraint.

## Stop Rules Preserved

M676 is not a full-matrix win and does not satisfy the final project goal.
It is a feasibility stage only.

Do not continue this exact oracle policy if:

- top95 overlap drops below the guard,
- gains only come from qrels leakage,
- native shared15 full-query metrics regress,
- the method requires dataset-specific thresholds.

## Decision

Proceed to M677, but with constraints:

1. Train a bounded query atom boost model, not a raw doc-atom append rule.
2. Use M675/M676 as teacher-analysis inputs, not as production labels alone.
3. Combine event positives with under-ranked qrels positives in native top1000.
4. Optimize native candidate-set promotion while preserving top95/head support.
5. Accept only if native DB evaluation improves Recall@100 and MAP@100 without
   NDCG/MRR/head-overlap regression.

If M677 cannot beat P1-a0125 and M674 on a held-out native event-query split,
the next move should be broader teacher construction, not larger model scale.
