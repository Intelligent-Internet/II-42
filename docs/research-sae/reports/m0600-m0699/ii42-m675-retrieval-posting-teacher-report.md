# M675 Retrieval-Constrained Posting Teacher Audit

## Purpose

M675 evaluates the review suggestion that the next useful route is not a return
to traditional SAE reconstruction, but a dense-faithful, retrieval-constrained
generated-posting compiler.

The audit uses the native same-query M674 rule as a strict teacher source:

- Keep the P1-a0125 ranking head stable with `preserve_top_k=95`.
- Admit only M674 top100 boundary crossings.
- Record only qrels-positive documents promoted into top100.
- Enrich each event with native P1/BM25 ranks, dense-hit status, and
  query-doc atom support.

This is not a trainer. It is a data-analysis bridge for deciding whether a
support-safe compiler has concrete target events.

## Artifacts

- Summary JSON: `runs/m675_retrieval_posting_teacher_v1/m675_teacher_summary.json`
- Event JSONL: `runs/m675_retrieval_posting_teacher_v1/m675_teacher_events.jsonl`
- Generated markdown: `runs/m675_retrieval_posting_teacher_v1/m675_teacher_summary.md`
- Script: `scripts/build_m675_retrieval_posting_teacher.py`

## Shared15 Result

Full local shared15 completed in 18.34 seconds.

| Metric | Value |
| --- | ---: |
| Query rows with qrels | 1342 |
| Queries with promoted positive | 136 |
| Promoted positive events | 340 |
| Dense-miss promoted share | 0.508824 |
| Mean BM25 rank for promoted positives | 14.062 |
| Mean positive shared atom count | 26.594 |
| Mean positive missing doc-head atoms | 22.679 |

Dataset breakdown:

| Dataset | Events | Dense-miss share | Mean BM25 rank | Mean shared atoms | Mean missing doc-head atoms |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | 0 | 0.000000 | 0.000 | 0.000 | 0.000 |
| climate-fever | 3 | 0.666667 | 15.333 | 10.667 | 29.333 |
| cqadupstack | 7 | 0.142857 | 17.714 | 18.286 | 26.857 |
| dbpedia-entity | 45 | 0.200000 | 21.200 | 16.889 | 26.844 |
| fever | 0 | 0.000000 | 0.000 | 0.000 | 0.000 |
| fiqa | 0 | 0.000000 | 0.000 | 0.000 | 0.000 |
| hotpotqa | 1 | 0.000000 | 7.000 | 11.000 | 30.000 |
| msmarco | 63 | 0.619048 | 24.857 | 24.079 | 23.508 |
| nfcorpus | 42 | 0.309524 | 9.905 | 13.952 | 27.905 |
| nq | 0 | 0.000000 | 0.000 | 0.000 | 0.000 |
| quora | 0 | 0.000000 | 0.000 | 0.000 | 0.000 |
| scidocs | 5 | 0.200000 | 10.400 | 16.000 | 26.000 |
| scifact | 2 | 0.500000 | 5.000 | 13.000 | 29.000 |
| trec-covid | 165 | 0.648485 | 8.776 | 35.024 | 19.194 |
| webis-touche2020 | 7 | 0.000000 | 22.571 | 17.571 | 27.000 |

## Interpretation

The review direction is useful, but only if implemented as a constrained
posting-compiler route.

M675 confirms that support-safe boundary crossing exists. M674 can promote
qrels-positive documents into top100 while preserving the top95 head. These
events are concrete training targets for a generated-posting compiler.

The important signal is that 50.9% of promoted positives are dense top100
misses. This means a dense-only mimic objective cannot cover the remaining
retrieval gap. The next teacher must be retrieval-constrained: qrels positives,
BM25/entity/corpus evidence, and native candidate competition, while retaining
dense-support floors.

The atom support statistics are also actionable. Promoted positives share a
mean 26.6 query atoms, but still miss a mean 22.7 atoms in their top document
head. That is the right shape for bounded generated-posting deltas: add or
boost retrieval-useful query-side support without freely rewriting the dense
geometry.

## Limits

M675 is not yet a full training set. Five datasets have zero promoted-positive
events from this teacher source: arguana, fever, fiqa, nq, and quora. Several
others have only a few events. Therefore M674-derived teacher events are enough
to start the next experiment, but not enough to claim a global solution.

The next training phase should not use M675 alone. It should combine:

- M675 support-safe promoted positives.
- qrels positives already in top1000 but under-ranked.
- BM25/entity/corpus pseudo positives.
- Dense-support and candidate-upper-bound guards.

## Decision

Do not return to traditional SAE reconstruction as the main route. Preserve its
engineering lessons only: atom utility, sparsity, IDF/fanout, and native index
lifecycle.

Do not continue arbitrary dense-mimic micro-adjustments as the main route. The
prior failures around M599/M600/M636/M637/M652 show that unconstrained boundary
movement often breaks dense overlap or Recall floors.

Proceed with a retrieval-constrained generated-posting compiler:

1. Freeze P1-a0125/M674 native surfaces as the baseline.
2. Train only bounded query/posting deltas.
3. Require dense-support, CUB, NDCG, and MRR floors.
4. Accept only native DB-path Recall@100/MAP@100 gains.
5. Expand teacher sources only after this support-safe seed proves useful.

## Next Experiment

M676 should be a minimal trainer, not another audit:

- Input: M675 events plus under-ranked qrels positives in native top1000.
- Output: query-side generated-posting deltas or atom boosts.
- Loss: listwise or pairwise promotion loss over native candidate sets.
- Guards: preserve top head, no CUB regression, no NDCG/MRR regression.
- Rejection: if gains appear only on trec-covid/msmarco or require dataset-specific
  thresholds, stop and broaden teacher construction before scaling.
