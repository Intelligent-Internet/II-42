# SAE M33 Hard-Family Supervision Results Report

Status: first hard-family pseudo-query arm completed; not promoted.

## Summary

M33 tested whether M32 failed mainly because `trec-covid`-style biomedical /
claim-heavy language was absent from train/dev. The experiment added a
`trec-covid-pseudo` training artifact built from corpus text only:

- 20,000 `trec-covid` documents;
- 300 pseudo queries derived from titles or first useful sentences;
- self-document pseudo qrels only;
- no official `trec-covid` test query text or qrels used as train labels;
- `quality_claim_allowed=false`.

Two runs were completed:

| Run | Train datasets | Train queries | Purpose |
| --- | ---: | ---: | --- |
| `m33-pseudo-x1` | 9 | 2,467 | Add one hard-family pseudo dataset |
| `m33-pseudo-x3` | 11 | 3,067 | Weight the same hard-family pseudo surface 3x |

Both runs used the M32 teacher-anchor final-ranking configuration and evaluated
on the current full15 regression surface.

## Full15 Aggregate

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| Teacher fixed-doc | 0.8447 | 0.8399 | 0.7490 | 0.7247 |
| M32 teacher-anchor best fixed | 0.8694 | 0.8780 | 0.7734 | 0.7449 |
| M33 pseudo x1 best fixed | 0.8661 | 0.8749 | 0.7727 | 0.7446 |
| M33 pseudo x3 best fixed | 0.8664 | 0.8777 | 0.7734 | 0.7455 |

The pseudo arm does not improve the aggregate frontier over M32. The x3 variant
roughly matches M32 aggregate MAP/NDCG, but this is not enough because the
product blocker is dataset collapse.

## TREC-COVID Blocker

| Source | Recall@100 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: |
| BM25 | 0.1384 | 0.6645 | 0.4693 |
| Teacher fixed-doc | 0.2081 | 0.8351 | 0.7639 |
| M31 unfrozen best | 0.1419 | 0.6539 | 0.4648 |
| M32 teacher-anchor best fixed | 0.1435 | 0.6226 | 0.4545 |
| M33 pseudo x1 best fixed | 0.1458 | 0.6391 | 0.4691 |
| M33 pseudo x3 best fixed | 0.1459 | 0.6208 | 0.4707 |

Pseudo self-qrels produce only a weak signal:

- x1 recovers MAP to approximately BM25 and modestly improves over M32, but it
  remains far below teacher.
- x3 does not help; it slightly improves MAP but worsens NDCG.
- Neither run solves the recall gap. Both stay around Recall@100 `0.146`,
  versus teacher `0.2081`.

This means corpus-title pseudo self-qrels are not sufficient to teach the query
encoder the teacher's biomedical semantic behavior.

## Collapse Table

| Run | Dataset | Metric | Delta vs teacher |
| --- | --- | --- | ---: |
| M33 pseudo x1 | `dbpedia-entity` | MAP@100 | -0.0321 |
| M33 pseudo x1 | `msmarco` | NDCG@10 | -0.0912 |
| M33 pseudo x1 | `msmarco` | MAP@100 | -0.0520 |
| M33 pseudo x1 | `trec-covid` | NDCG@10 | -0.1960 |
| M33 pseudo x1 | `trec-covid` | MAP@100 | -0.2948 |
| M33 pseudo x3 | `dbpedia-entity` | MAP@100 | -0.0359 |
| M33 pseudo x3 | `msmarco` | NDCG@10 | -0.0854 |
| M33 pseudo x3 | `msmarco` | MAP@100 | -0.0557 |
| M33 pseudo x3 | `trec-covid` | NDCG@10 | -0.2143 |
| M33 pseudo x3 | `trec-covid` | MAP@100 | -0.2931 |

The no-collapse gate still fails.

## Physical Cost

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| Teacher fixed-doc | 2,986.2 | 10,185.0 | 4,522.8 |
| M32 teacher-anchor best fixed | 2,964.8 | 10,185.0 | 2,778.6 |
| M33 pseudo x1 best fixed | 2,973.2 | 10,185.0 | 2,848.6 |
| M33 pseudo x3 best fixed | 2,964.9 | 10,185.0 | 2,864.4 |

Cost remains acceptable. The failure is quality, not physical feasibility.

## Decision

Do not promote M33 pseudo self-qrel training.

This arm answers a useful question: simply exposing the text-to-atoms encoder
to biomedical corpus language is not enough. The missing signal is likely the
teacher neighborhood / relevance distribution, not just vocabulary or document
style.

Next valid moves:

1. Build biomedical/claim-heavy teacher-neighborhood distillation without using
   held-out test qrels.
2. Keep the final-ranking objective and fixed teacher doc atoms.
3. Consider a stronger query encoder only under the same full15 no-collapse
   gate.
4. Do not proceed to doc-side training or SQL/API productization from the M33
   pseudo models.
