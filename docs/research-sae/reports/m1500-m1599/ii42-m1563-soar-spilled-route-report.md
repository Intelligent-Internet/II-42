# M1563 SOAR Spilled-Route Report

## Decision

**Do not promote or train M1563 from the NFCorpus canary.**

SOAR produces the intended orthogonal residual geometry and improves all dense
overlap depths at lower posting-read cost, but the gain is below the
predeclared candidate-access gate.  This is a real structural signal, not yet
a dense-equivalent posting source.

## Basis

M1563 implements the fixed `lambda=1` assignment from SOAR
(`arXiv:2404.00774`):

```text
L(r', r) = ||r'||^2 + ||proj_r(r')||^2
```

This directly targets the correlated quantization-error failure of M1542's
nearest/second-nearest centroid assignments.  Construction is corpus-only and
qrels-free, with two posting edges per document and one inverted index.

## Surface

- NFCorpus: 2,063 documents and 100 queries.
- Frozen BGE and M1542 spherical codebook.
- Exact 15% unique candidate union.
- Deterministic centroid query routing and route-cover oracle.
- Exact dense reranking over touched candidates.
- ClearML task: `ea15721c81054ee4a86c572fe4101386`.
- M1542 parity maximum absolute delta: `0.0`.

## Geometry

| Diagnostic | Naive centroid dual | SOAR dual | Change |
| --- | ---: | ---: | ---: |
| primary/secondary score-error correlation | 0.298020 | 0.104795 | -64.84% |
| mean best route rank | 9.4764 | 9.1081 | -3.89% |
| top1 pair coverage | 0.1710 | 0.1761 | +0.0051 |
| top4 pair coverage | 0.4766 | 0.4830 | +0.0064 |
| top8 pair coverage | 0.6596 | 0.6673 | +0.0077 |

The residual-decorrelation prediction is strongly confirmed.  The resulting
centroid-rank change is positive but does not reach the required 10% reduction.

## Retrieval

| Source | Policy | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | Reads |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| centroid dual | centroid | 0.8510 | 0.6443 | 0.490156 | 0.355283 | 0.455250 | 0.1730x |
| SOAR dual | centroid | 0.8630 | 0.6643 | 0.505430 | 0.345336 | 0.444219 | 0.1671x |
| centroid dual | oracle | 0.8650 | 0.7344 | 0.526094 | 0.332412 | 0.434107 | 0.1686x |
| SOAR dual | oracle | 0.8810 | 0.7534 | 0.533711 | 0.345540 | 0.442362 | 0.1662x |

SOAR deltas under deterministic routing are `+0.0120` O@10, `+0.0200`
O@100, and `+0.015273` O@256.  Oracle O@100 gains `+0.0190`.  Every dense
overlap direction is positive and reads fall by `0.00586x`, but the `+0.05`
and `+0.03` access gates are not met.

Qrels metrics are mixed: oracle Recall/NDCG improve, while deterministic
Recall/NDCG decrease.  They are not gate evidence for first-stage equivalence,
but they prevent treating the small overlap gain as a product win.

## Interpretation

M1563 differs from M1560-M1562: its intended mathematical mechanism is
observed and candidate access moves in the predicted direction.  It fails on
magnitude, not sign or leakage.

The SOAR paper reports that benefits increase with corpus size and higher
recall targets.  NFCorpus is only 2,063 documents, so a single separately
contracted scale-mechanism check on FiQA is scientifically justified.  This is
not an override of the failed canary gate and cannot authorize model training.

If the larger corpus does not materially amplify O@100/O@256 gains, stop SOAR.
If it does, repeat unchanged on SciFact and only then reconsider a three-row
capacity gate.  Do not tune lambda, fanout, codebook size, or budget.
