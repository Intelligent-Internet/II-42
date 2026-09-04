# M1720A Lexical-Quotient Residual Posting Report

Date: 2026-07-12

Decision: **stop the orthogonal lexical-quotient source**.

## Question

M1720A tested whether the dense evidence predictable from corpus lexical
features could be removed before semantic posting construction. The intended
effect was to leave a smaller, more selective residual that represented only
what frozen BM25 could not already publish.

The audit used the locked BGE/shared3 surface, a corpus-only hashed TF-IDF to
dense cross-covariance basis, rank 128, one route per document, and fixed 8%
and 15% semantic traversal budgets. Qrels affected only final measurements.
Raw dense routes were the same-budget control.

ClearML task: `aa7eeeb4046c4e8eace305ad29fe428d`.

## Integrity

The proposed orthogonal decomposition was implemented correctly:

- maximum score reconstruction error was `1.79e-7` or lower;
- reconstructed dense O@100 was exactly `1.0` on all three rows;
- the rank-128 basis captured `91.78%` to `94.35%` of lexical/dense
  cross-covariance energy.

The failure is therefore a representation result, not an evaluator failure.

## Structural Result

| Dataset | Lexical energy | Residual energy | Raw effective rank | Residual effective rank | Raw cluster cosine | Residual cluster cosine | Raw max DF | Residual max DF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | 0.739628 | 0.260372 | 192.79 | 247.64 | 0.673263 | 0.603436 | 0.022782 | 0.066408 |
| SciFact | 0.703814 | 0.296186 | 216.51 | 250.81 | 0.629596 | 0.566257 | 0.019500 | 0.052500 |
| FiQA | 0.738664 | 0.261336 | 210.08 | 260.44 | 0.606787 | 0.453917 | 0.022500 | 0.029000 |

All three structural rows fail. The lexical-correlated component contains
roughly 70% to 74% of dense energy, but it is also the easier part of the
dense geometry to cluster. Removing it raises, rather than lowers, residual
effective rank. The residual routes are less coherent and substantially more
imbalanced.

This explains why the original intuition did not translate into a cheaper
index. "BM25 cannot express it" does not imply "it is low-dimensional or
sparse." The complementary semantic information is the diffuse part of the
dense space on this surface.

## Retrieval Result

At the fixed 15% semantic budget:

| Dataset | Source | Dense-minus-BM25 recovery | Relevant miss recovery | Unified CUB | Dense O@100 upper | Reads |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | raw dense route | 0.244204 | 0.432320 | 0.623298 | 0.649500 | 0.150267 |
| NFCorpus | lexical quotient | 0.168491 | 0.158020 | 0.526502 | 0.152700 | 0.150267 |
| SciFact | raw dense route | 0.209619 | 1.000000 | 1.000000 | 0.636500 | 0.150000 |
| SciFact | lexical quotient | 0.167975 | 1.000000 | 1.000000 | 0.132200 | 0.150000 |
| FiQA | raw dense route | 0.219291 | 0.789474 | 0.990972 | 0.737400 | 0.150000 |
| FiQA | lexical quotient | 0.160149 | 0.105263 | 0.930782 | 0.128300 | 0.150000 |

Macro relevant BM25-miss recovery falls from `0.740598` to `0.421094`.
Exact, full-corpus residual ranking also passes the capacity gate on only one
of three rows, so the loss is not merely caused by spherical routing.

## Conclusion

The broad research objective remains valid: BM25 should publish lexical
evidence once, and semantic postings should be judged by the additional
evidence they recover. The specific orthogonal-subspace implementation is
invalid.

Do not sweep quotient rank, lexical hash size, route seed, or a training loss.
The measured mechanism is stable and causal: the projection removes the most
compressible dense structure and leaves a higher-rank residual.

The next justified experiment is a different hypothesis, not M1720A tuning:
construct a qrels-free score-level teacher
`relu(dense_rank_signal - BM25_rank_signal)` and test whether that relational
residual admits a shared query/document bilinear factorization under
leave-one-dataset-out validation. This directly tests CLEAR-style
complementarity without assuming it is an orthogonal dense subspace.

## Reproducibility

- Remote result:
  `/home/huoju/leask/runs/ii42-m1720-lexical-quotient-v1/runs/m1720a-lexical-quotient-shared3-v1/summary.json`
- Summary SHA-256:
  `0f4801cdc094575f3e52292f647e4c3a8616210d83fc931a06f7d5da5da49228`
- Runner: `scripts/run_m1720_lexical_quotient_residual_spark.sh`
- Audit: `scripts/audit_m1720_lexical_quotient_residual.py`
- Focused regression suite: 27 tests passed with M1520, M1541, M1542, and
  M1710 compatibility coverage.
