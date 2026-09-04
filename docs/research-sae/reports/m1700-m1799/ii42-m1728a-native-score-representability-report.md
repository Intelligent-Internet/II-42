# M1728A Native Score Representability Report

## Decision

**Stop the M1727 additive expansion before scorer or encoder training.** The
frozen expansion improves candidate access, but neither posting-decomposable
native score preserves enough of that gain. A fitted lexical alpha, nonlinear
reranker, or deeper residual head is not authorized.

## Frozen Surface

- exact M1727A-v2 candidate policy and `0.179909x` mean reads;
- M1600 full 1,000-query/8,988-document validation pool;
- frozen 8x512 source, query logits, document keys, and BM25 candidates;
- no qrels, fitted weights, normalization, rank features, or score search;
- exact dense reranking retained only as candidate upper.

## Result

| Surface | O@10 | O@100 | O@256 |
| --- | ---: | ---: | ---: |
| Exact-dense upper, base | 0.984700 | 0.865390 | 0.745742 |
| Exact-dense upper, expanded | 0.987000 | 0.889080 | 0.785539 |
| Native key-hit count, base | 0.384100 | 0.434640 | 0.443738 |
| Native key-hit count, expanded | 0.377700 | 0.436470 | 0.453500 |
| Native query-logit sum, base | 0.429700 | 0.444330 | 0.451707 |
| Native query-logit sum, expanded | 0.434100 | 0.447890 | 0.461199 |

Query-logit sum is the stronger score. Expansion changes it by:

- O@10 `+0.004400`;
- O@100 `+0.003560`;
- O@256 `+0.009492`.

It captures only `15.03%` of the exact-dense O@100 gain and `23.85%` of the
O@256 gain, below the fixed 50% requirements. Key-hit count captures
`7.72%/24.53%` and regresses O@10 by `0.0064`.

## Failure Mechanics

M1727 proves that the extra posting lists contain useful documents. M1728
shows why they do not become a native retrieval result:

1. one document publishes one key per group, so many documents share exactly
   the same coarse score pattern;
2. query logits order posting lists, but do not resolve documents within a
   list;
3. adding more low-ranked keys increases candidate coverage faster than score
   resolution;
4. exact dense reranking supplies the missing within-list geometry, which is
   explicitly unavailable to the product scorer.

The base direct score itself is the decisive evidence: O@100 `0.444330`
versus candidate upper `0.865390`. The blocker exists before residual
expansion is added.

## Historical Cross-Check

M1710 independently measured nearly the same geometry. Its first-order
8-key/group policy reached direct O@100/O@256 `0.448530/0.455977` while its
exact-dense candidate upper reached `0.830990/0.704930`. M1728 reproduces the
direct-score range on a BM25-unioned surface and shows that budgeted logit
expansion does not repair it.

M1530 already exhausted independent centroid training, shared low-rank
adaptation, and hard-plus-soft distillation without stable winner transfer.
M1600 exhausted a static query router, and M1723-M1726 show that joint source
movement damages the stronger full-scale initialization. Training another
head after M1728 would therefore repeat a closed score-factorization family,
not test a new causal mechanism.

## Route Boundary

Supported:

- frozen additive keys can improve dense candidate access within cost;
- base protection by construction is superior to joint residual movement;
- query logits contain a broad, qrels-free residual admission signal.

Not supported:

- a one-hop key-hit or key-logit posting score can preserve dense ranking;
- another local calibration can bridge an O@100 gap of roughly `0.42`;
- the M1727 candidate upper is a unified-posting product result.

The next route must add score resolution structurally inside one physical
index: block/centroid pruning followed by bounded residual or late-interaction
scoring over the touched block. That relaxes the one-hop scalar posting-dot
contract while retaining one semantic/lexical index lifecycle. If that
architectural relaxation is not acceptable, the evidence supports stopping
the pure posting replacement objective rather than training further.

## Reproducibility

- host: `spark-1`;
- run:
  `/home/huoju/leask/runs/ii42-m1728-native-score-v1/runs/m1728a-native-score-full-seed1601-v1`;
- summary SHA-256:
  `7c44ef508582cd5e58492a4f5468e5bc81fc38af57286022be81397fc9dde38b`;
- local result: `ii42-m1728a-native-score-representability-result.json`.
