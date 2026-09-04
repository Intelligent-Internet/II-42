# M1545 Additive Semantic/Lexical Rescue Contract

## Question

M1543 showed that reserving lexical budget inside a fixed 15% union harms two
rows because BM25 candidates replace useful semantic candidates. M1545 tests
the only remaining retrieval-specific hypothesis supported by that result and
the older M550 fixed-rescue line:

> Does adding a bounded 5% lexical rescue to the frozen 15% semantic route
> create relevant-document capacity that neither equal-budget source has?

M1545 cannot reopen the failed dense-equivalence claim. It is a final
retrieval-only capacity gate before any scorer training.

## Frozen Policy

- M1542 dual-assignment semantic route, fixed at 15% unique candidates;
- add highest-ranked unseen reference-BM25 candidates up to 20% total union;
- compare with exact BGE top20% and BM25 top20%;
- tail256 INT8 semantic scoring remains frozen;
- P1 weighted RRF remains fixed at semantic `0.875`, BM25 `0.125`, `k=60`;
- no alpha, budget, route, centroid, dataset, or threshold search;
- qrels are evaluation-only.

## Capacity Gate

The additive union passes a dataset when its CUB exceeds the better equal-20%
dense/BM25 CUB by at least `0.005`. Capacity is authorized only with:

- at least two of three dataset passes;
- no row more than `0.01` below its better baseline;
- total unique union at most `0.201`.

## Retrieval Gate

The frozen P1 fusion must improve macro Recall@100 and MAP@100 over both
equal-budget baselines, retain at least 98% macro NDCG/MRR, and retain 90%
Recall/NDCG on every row.

## Decisions

- **Capacity + retrieval pass:** move directly to native index validation.
- **Capacity pass, retrieval fail:** authorize one global constrained listwise
  scorer over the frozen additive union.
- **Capacity fail:** close semantic-route rescue and do not train a scorer.

