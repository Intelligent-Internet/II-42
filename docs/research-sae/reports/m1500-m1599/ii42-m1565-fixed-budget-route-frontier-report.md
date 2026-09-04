# M1565 Fixed-Budget Route Frontier Report

## Decision

**Stop nearest-two corpus routes as a standalone dense-replacement source.**

The M1564 near-dense result at a 15% candidate union does not survive a fixed
engineering budget. Neither deterministic query routing nor the route-cover
oracle reaches the predeclared 1,000-candidate capacity gate. Do not train a
cluster selector, tune centroids, add route fanout, or call this semantic path
dense-equivalent.

This does not reject a unified inverted index. It establishes that the next
source must combine semantic cluster lists with a genuinely complementary
posting namespace, rather than further optimizing cluster assignment alone.

## Integrity

- Official FiQA: 57,638 documents, 648 queries, and 1,706 qrel edges.
- Frozen BGE encoder and 3,603-route spherical codebook.
- Two nearest-centroid postings per document.
- Qrels-free reusable basis: 195,707,280 bytes.
- Basis SHA-256:
  `46c2a0fe2b0b453c91d7bf40cecbab3e3c2bcbc02289ee88fd05400a91e4f9ee`.
- ClearML task: `1fd7056a6e4345ce995b8cc3be19e514`.
- Runtime: 3,119.7 seconds.
- M1564 15% anchor maximum absolute delta: `0.0`.

The exact parity proves that the result measures only candidate-budget
contraction; it is not caused by a different encoder, root, codebook, or route
assignment.

## Fixed-Budget Frontier

| Budget | Policy | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads | Probes |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 256 | centroid | 0.771451 | 0.534136 | 0.381498 | 0.601778 | 0.346717 | 0.292509 | 0.428231 | 0.640981 | 0.005005 | 7.41 |
| 256 | oracle | 0.804321 | 0.604074 | 0.393862 | 0.613058 | 0.355210 | 0.297592 | 0.438496 | 0.651884 | 0.004874 | 9.88 |
| 512 | centroid | 0.861420 | 0.682361 | 0.543138 | 0.649963 | 0.366565 | 0.307940 | 0.450751 | 0.728708 | 0.010415 | 14.64 |
| 512 | oracle | 0.903086 | 0.784645 | 0.550275 | 0.676488 | 0.377456 | 0.316699 | 0.462616 | 0.755516 | 0.009917 | 18.09 |
| 1000 | centroid | 0.921759 | 0.808503 | 0.702311 | 0.683089 | 0.383019 | 0.322550 | 0.468258 | 0.822706 | 0.021159 | 29.41 |
| 1000 | oracle | 0.968519 | 0.933225 | 0.698085 | 0.714086 | 0.394805 | 0.331988 | 0.479564 | 0.839381 | 0.019772 | 30.86 |
| 2048 | centroid | 0.969907 | 0.906235 | 0.842683 | 0.720558 | 0.397244 | 0.334942 | 0.481697 | 0.866777 | 0.045015 | 62.81 |
| 2048 | oracle | 0.997994 | 0.997562 | 0.855155 | 0.733470 | 0.400541 | 0.337819 | 0.485501 | 0.883619 | 0.043694 | 61.38 |
| 8646 | centroid | 0.997222 | 0.989738 | 0.981216 | 0.733148 | 0.399796 | 0.337321 | 0.485030 | 0.896979 | 0.211050 | 298.45 |
| 8646 | oracle | 1.000000 | 1.000000 | 0.985050 | 0.733984 | 0.400339 | 0.337638 | 0.485244 | 0.898421 | 0.210820 | 298.16 |

Exact dense has Recall@100 `0.733984`, NDCG@10 `0.400339`, MAP@100
`0.337638`, MRR@20 `0.485244`, and CUB `0.902807`.

## Failure Anatomy

At 1,000 candidates, deterministic O@100 is only `0.808503`. The oracle
raises it to `0.933225`, so query route selection is imperfect, but the oracle
still misses the required `0.98`. Training only the query selector therefore
cannot close the gap.

At 2,048 candidates, oracle O@100 reaches `0.997562`, while oracle O@256 stays
at `0.855155`. This separates two effects:

1. enough route lists can recover the dense head;
2. the two-centroid posting source does not expose the broader dense tail at a
   practical fixed budget.

The 15% anchor hides this limitation by touching 8,646 candidates and about
21% of posting entries. Its near-perfect overlap is a high-touch capacity
ceiling, not a deployable access result.

## Literature And Next Source

HI2 (`arXiv:2210.05521`) reports the same low-budget IVF failure and addresses
it by storing embedding-cluster entries and salient-term entries in one
inverted index. Its unsupervised path uses KMeans plus BM25 term selection;
its supervised path distills both selectors from dense score distributions.

That evidence supports one new source-capacity test, not a continuation of
route tuning:

```text
one document reference
  -> corpus-route posting entries
  -> selected lexical-term posting entries
  -> one union of inverted lists
  -> one compact dense-root codec over reached candidates
```

M1566 must first test this qrels-free combined source at fixed cost. Only if
the combined source reaches dense-faithful candidate capacity may selector
distillation or protected-tail ranking begin. If exact lexical postings cannot
repair the fixed-budget route frontier, stop the entire cluster-plus-term
route before training.
