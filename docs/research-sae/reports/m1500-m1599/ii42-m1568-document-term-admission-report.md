# M1568 Document-Term Admission Report

## Decision

**Stop the fixed term15 selector target; retain dense-teacher document-level
admission as a positive source mechanism.**

M1568 does not authorize selector training under its preregistered contract.
The transductive source passes the O@100/O@256 capacity thresholds, and every
held-out split improves substantially over unsupervised term15. However, the
held-out mean O@100 is `0.944023`, below the `0.95` floor, and mean raw posting
reads are `0.463157x`, above the `0.30x` budget.

This is not evidence for another selector, gate, or loss search. It isolates
two source/execution questions that must be answered before training:

1. can an exact impact-ordered inverted-index traversal recover the same top
   lexical candidates without scanning all high-DF postings;
2. only if access cost passes, can a separately preregistered document-term
   budget/source improve the remaining held-out O@100 gap.

## Surface And Integrity

- Official FiQA: 57,638 documents, 648 queries, and 1,706 qrel edges.
- One route namespace plus one exact lexical namespace; no ANN candidate path
  and no external BM25 engine.
- Fixed 15 selected terms per document and at most 32 query terms.
- Dense top256 ranks are the qrels-free document-admission teacher.
- Three deterministic 324/324 train/held-out splits.
- Route1000 maximum parity delta: `0.0`.
- Qrels-free candidate surface SHA-256:
  `960f53e488444704dc6c7f54921354ef4f3606571529224b192a60ab131dad20`.
- ClearML task: `be0f94299e6d4d9ab8b9ad6a77c34c3c`.
- Runtime: 111.4 seconds.

The transductive teacher selected 862,374 posting edges across 61,537 terms.
It directly observed 33,910 documents (`58.83%`) and used deterministic BM25
fallback for the remainder. The source's maximum selected-term DF ratio is
`0.345553`, despite storing only 14.96 postings per document.

## Capacity Matrix

| Surface | Variant | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| all queries | unsup deterministic | 0.949383 | 0.843287 | 0.741000 | 0.710759 | 0.395886 | 0.333553 | 0.482150 | 0.862693 | 0.044619x |
| all queries | unsup oracle | 0.958488 | 0.859645 | 0.763847 | 0.714375 | 0.397032 | 0.334209 | 0.483197 | 0.868109 | 0.044619x |
| all queries | teacher deterministic | 0.979167 | 0.903349 | 0.812223 | 0.732381 | 0.400953 | 0.337989 | 0.485890 | 0.887803 | 0.656657x |
| all queries | teacher oracle | 0.997840 | 0.990370 | 0.979474 | 0.731927 | 0.400680 | 0.337628 | 0.485211 | 0.898291 | 0.656657x |
| held-out mean | unsup deterministic | 0.946605 | 0.842058 | 0.740721 | 0.714587 | 0.394714 | 0.330145 | 0.485115 | 0.866839 | 0.044555x |
| held-out mean | unsup oracle | 0.954115 | 0.858066 | 0.762768 | 0.719322 | 0.396000 | 0.330924 | 0.485991 | 0.872259 | 0.044555x |
| held-out mean | teacher deterministic | 0.944239 | 0.843549 | 0.744446 | 0.706894 | 0.392253 | 0.328501 | 0.481479 | 0.854036 | 0.463157x |
| held-out mean | teacher oracle | 0.980967 | 0.944023 | 0.905302 | 0.729173 | 0.397009 | 0.332601 | 0.485789 | 0.886827 | 0.463157x |

Held-out oracle gains over unsupervised term15 are stable on all three splits:

| Seed | O@100 gain | O@256 gain |
| ---: | ---: | ---: |
| 1568 | +0.085864 | +0.143736 |
| 2568 | +0.085093 | +0.140191 |
| 3568 | +0.086914 | +0.143675 |

## Interpretation

The source mechanism is real. Document-level admission recovers most dense
top256 membership on unseen queries and improves both overlap depths on every
split. This is much stronger than whole-term DF filtering and rules out the
claim that useful common-term occurrences cannot be selected per document.

The fixed target is still not ready for distillation. First, raw access cost
is dominated by a small number of selected high-DF terms: 15 postings per
document does not imply bounded work per query. Second, the held-out
deterministic scorer is weaker than the oracle and slightly worse than the
unsupervised deterministic source. A learned scorer cannot recover documents
that are absent, and it should not be trained until source capacity and access
cost pass together.

The identical held-out gains across all splits are useful evidence against a
pure memorization explanation. The remaining `0.005977` O@100 shortfall is
small, but changing the threshold or term budget after observing it would
invalidate M1568. Any budget change must be an independent experiment after
the access mechanism is established.

## Next Gate

Run an exact impact-ordered posting-access audit over the frozen M1568 source:

1. construct the same transductive and three held-out term indexes;
2. retrieve the exact top256 lexical documents using impact-ordered postings,
   with no candidate or score approximation;
3. require candidate parity with the full scan before comparing reads;
4. count postings actually decoded and require mean combined reads `<=0.30x`;
5. do not change term selection, candidate budget, or overlap gates.

If exact access still exceeds the budget, stop this exact-term source family.
If it passes, the next independent source frontier may test whether a larger
fixed document-term budget closes held-out O@100 while preserving the measured
access budget. Neural selector training remains blocked until both questions
pass.
