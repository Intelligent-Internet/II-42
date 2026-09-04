# M681 Broader Retrieval-Constrained Teacher

## Purpose

M681 addresses the M680 failure directly.

M680 showed that the M679 selector learned from M675 promoted-positive events
does not generalize to the full shared15 native query surface. The likely
failure mode was teacher coverage: M675 only captured qrels positives that the
M674 top95-preserving BM25 tail reorder can promote.

M681 broadens the teacher to all qrels-positive documents that are:

- present in the native top1000 candidate set;
- outside the current top100;
- available through the same native PostgreSQL unified posting path.

This is still a data-analysis stage, not a trainer.

## Artifacts

- Summary JSON: `runs/m681_broader_retrieval_teacher_v1/m681_teacher_summary.json`
- Generated markdown: `runs/m681_broader_retrieval_teacher_v1/m681_teacher_summary.md`
- Local raw events: `runs/m681_broader_retrieval_teacher_v1/m681_teacher_events.jsonl`
- Script: `scripts/build_m681_broader_retrieval_teacher.py`
- Tests: `tests/test_build_m681_broader_retrieval_teacher.py`

The raw event JSONL is `61M`, so it is kept as a local reproducible artifact
instead of being committed. The committed summary and script are enough to
recreate it.

## Macro Result

| Metric | Value |
| --- | ---: |
| Positive qrels scanned | 39,742 |
| Top100 positives | 12,913 |
| Under-ranked top1000 positives | 13,885 |
| Candidate-miss positives | 12,944 |
| Dense-miss event share | 0.832193 |
| BM25-high-rank event share | 0.080879 |
| Support-shared event share | 0.992222 |
| M675 overlap share | 0.024487 |
| Train / holdout events | 11,810 / 2,075 |

## Dataset Coverage

| Dataset | Under-ranked events | Candidate miss | Dense-miss share | M675 overlap |
| --- | ---: | ---: | ---: | ---: |
| arguana | 0 | 0 | 0.000000 | 0.000000 |
| climate-fever | 17 | 2 | 0.294118 | 0.176471 |
| cqadupstack | 151 | 108 | 0.662252 | 0.046358 |
| dbpedia-entity | 631 | 279 | 0.543582 | 0.071315 |
| fever | 0 | 0 | 0.000000 | 0.000000 |
| fiqa | 11 | 2 | 0.545455 | 0.000000 |
| hotpotqa | 1 | 0 | 0.000000 | 1.000000 |
| msmarco | 1,366 | 133 | 0.814056 | 0.046120 |
| nfcorpus | 1,577 | 1,464 | 0.851617 | 0.026633 |
| nq | 0 | 0 | 0.000000 | 0.000000 |
| quora | 0 | 0 | 0.000000 | 0.000000 |
| scidocs | 107 | 42 | 0.672897 | 0.046729 |
| scifact | 3 | 1 | 0.666667 | 0.666667 |
| trec-covid | 10,005 | 10,911 | 0.856472 | 0.016492 |
| webis-touche2020 | 16 | 2 | 0.187500 | 0.437500 |

## Interpretation

M681 confirms the M680 diagnosis.

M675 was too narrow:

- M675 had 340 promoted-positive events.
- M681 has 13,885 under-ranked qrels-positive events.
- Only 2.45% of M681 events overlap M675.

That explains why M679 could beat the BM25 top-k reference on the M675 event
slice but failed the full shared15 gate. The model was trained on a highly
selective boundary event surface, not the broader under-ranked positive
distribution.

M681 also shows why the next stage must be careful:

- 83.2% of events are dense-miss positives, so dense-only mimic is insufficient.
- 99.2% have at least 8 shared atoms, so query-side posting expansion has a
  plausible support path.
- Only 8.1% are BM25-high-rank positives, so BM25-tail selection alone cannot
  cover this teacher.
- The distribution is very imbalanced: trec-covid alone contributes 10,005
  events. Directly training on raw events would overfit that row.

The result supports the review direction:

- do not return to traditional SAE reconstruction;
- keep dense-root unified posting as the engineering shape;
- replace dense-only mimic and event-slice selector training with a broader
  retrieval-constrained generated-posting objective;
- keep full native shared15 as the promotion gate.

## Decision

Preserve M681 as the new teacher source for the next compiler attempt.

Do not train directly on the raw event distribution. The next trainer must use
dataset/query-balanced sampling and explicit floors:

1. candidate upper bound must not drop;
2. Recall@100 and MAP@100 must improve together on full shared15;
3. top95/head overlap must remain above the configured floor;
4. event-query wins alone are insufficient;
5. no dataset-specific thresholds.

## Next Step

M682 should be a conservative generated-posting compiler trained from M681:

- Input: query atoms plus selected under-ranked positive support atoms.
- Teacher sampling: dataset-balanced and query-balanced, capped per query.
- Objective: promote under-ranked positives while preserving dense/root support.
- First evaluation: full shared15 native matrix, not only event queries.
- Rejection: if gains come mostly from trec-covid or if CUB/top95 drops.

This is the first stronger teacher surface after M680. It is worth one
carefully bounded trainer, not an unconstrained scale-up.
