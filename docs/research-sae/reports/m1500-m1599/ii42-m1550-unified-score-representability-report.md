# M1550 Unified Score Representability Report

## Result

Decision: **stop blind whole-ranking residual training; continue with an
explicit protected-tail selector audit**.

The live PostgreSQL reconstruction reproduced the recorded M1549 metrics on
nfcorpus, scifact, and fiqa with maximum absolute error `0.0`.  The teacher and
feature construction are therefore valid.

The experiment used 1,271 native queries and leave-one-dataset-out training.
Teacher labels were generated only from dense top256, BM25 top256, and the
M1549 top99 rule.  Qrels were used only after rankings were frozen.

## Main observations

| Model family | Head@100 | Target@100 | Recall gain kept | Result |
| --- | ---: | ---: | ---: | --- |
| Local fixed score | 0.9830 | 0.2807 | not admissible | fails dense head |
| Local nonlinear score | 0.7826 | 0.3031 | negative/unstable | rejects whole-score MLP/GBDT |
| Rank-context nonlinear score | 1.0000 | 0.7564 | 0.8764 macro | near teacher, but not row safe |

The rank-context model recovered `87.64%` of the three-row macro M1549 Recall
gain.  Its macro Recall delta over dense was `+0.020974`, versus `+0.023931`
for the exact M1549 teacher.  However, because this model was allowed to score
the whole candidate set, it changed the dense head slightly:

- macro NDCG@10 delta: `-0.000034`;
- macro MRR@20 delta: `-0.000061`;
- nfcorpus had the corresponding row-level harm;
- fiqa retained only `7.4%` of its small teacher Recall gain.

The fixed local scorer selected lexical weight `0.0` in all LODO folds.  Even
then it did not preserve the native dense head because exact dense rescoring of
the ANN candidate union is not identical to the original VectorChord order.
Its apparent quality gains therefore do not satisfy the teacher contract and
must not be promoted.

## Interpretation

This is not evidence that lexical residual learning is useless.  It is evidence
that the model must not be responsible for re-ranking the protected dense head.
The proposed product architecture already permits an index-internal
protected-tail policy, so the correct next experiment is narrower:

1. freeze dense ranks 1-99 exactly;
2. train or distill only the selector/order over the remaining tail;
3. place one selected lexical candidate at rank 100;
4. measure candidate admission separately from boundary ordering.

This changes the learning problem from an unsafe whole-ranking regression into
a qrels-free tail-selection problem.  It also makes NDCG@10 and MRR@20
invariance structural rather than a soft loss preference.

## Stop rule carried forward

Do not start GPU residual-head training unless the protected selector retains
at least `50%` of M1549 Recall gain on every nontrivial held-out row and at
least `80%` in macro, with exact dense top99 preservation.  If pair-local
lexical features cannot pass, keep the deterministic lexical head and
protected controller inside the unified II-42 index.
