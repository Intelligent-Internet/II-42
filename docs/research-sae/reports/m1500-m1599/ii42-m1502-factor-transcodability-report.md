# M1502 Factor Transcodability Report

Date: 2026-07-09
Decision: failed twice; close M1503-M1506

## Question

M1501 proved that a rank-16/TopK-4 free sparse bilinear factorization can
retain 93.33% of a qrels-aware direct-field oracle. M1502 asks the necessary
next question: are those query and document factors predictable from the
canonical frozen PPLX-1024 text roots on held-out queries and documents?

The gate requires at least 70% recovery of held-out free-factor retrieval
utility, a trained checkpoint rather than epoch 0, and positive predicted
utility in at least two of three seeds.

## Attempt A: Small Nonlinear Heads

Architecture: separate `1024 -> 128 -> 16` query and document MLP heads,
balanced active/inactive factor loss, hard TopK-4 inference, three
dataset-stratified seeds. Checkpoints were selected only by held-out factor
loss. Retrieval metrics were computed afterward with the frozen M1501 lambda.

| Seed | Best epoch | Query cosine | Query support recall | Doc cosine | Doc support recall | Free utility | Predicted utility | Recovery |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1502 | 0 | 0.0649 | 0.1912 | -0.0228 | 0.0927 | +1.937278 | +0.000013 | +0.000007 |
| 1503 | 2 | 0.0042 | 0.1275 | 0.0318 | 0.1207 | +0.506348 | -0.003202 | -0.006324 |
| 1504 | 1 | 0.0203 | 0.0686 | 0.0473 | 0.0917 | +0.100558 | +0.000304 | +0.003022 |

Mean recovery was `-0.00110`. One seed selected epoch 0 and the other two
improved validation loss only marginally. This is not a training-depth shape:
loss stopped generalizing almost immediately.

## Attempt B: Ridge Observability Ceiling

The second and final attempt replaced optimization with cross-validated
closed-form ridge regression. A global alpha was selected from factor
validation loss only. This tests whether the factors are at least a smooth
linear function of the PPLX roots and removes optimizer/depth ambiguity.

| Seed | Alpha | Beats zero validation | Query active AUC | Doc active AUC | Query cosine | Doc cosine | Predicted utility | Recovery |
| ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1502 | 100 | no | 0.292 | 0.640 | -0.0037 | 0.0587 | -0.001464 | -0.000756 |
| 1503 | 10 | yes | 0.375 | 0.711 | 0.0050 | 0.0396 | -0.005998 | -0.011845 |
| 1504 | 100 | no | 0.500 | 0.651 | -0.0039 | 0.1140 | -0.000126 | -0.001248 |

Mean recovery was `-0.00462`; all three predicted utilities were negative.
Two seeds did not beat the zero-factor validation baseline.

## Interpretation

The M1501 field has genuine retrieval capacity, but its query activation and
factor orientation are not predictable from the frozen PPLX sentence roots.
The strongest evidence is query-side observability: active/non-active AUC is
at or below chance, while target factor cosine is approximately zero. The
document side contains a weak activity signal, but it does not recover factor
direction or retrieval utility.

This separates two claims that were previously mixed:

1. A compact sparse bilinear correction exists for the audited qrel pairs.
2. A text encoder can emit that correction for unseen text.

M1501 supports claim 1. Both M1502 attempts reject claim 2 on the current
teacher. Deeper training would fit teacher/query identity rather than recover a
general text-to-posting function.

## Decision

This is the second M1502 gate failure. By the predefined stop rule:

- do not start M1503 qrels-free teacher scaling;
- do not start M1504 constrained joint training;
- do not run M1505 hard-row/shared15 promotion;
- do not modify the UnifiedPosting namespace for M1506;
- retain P1.3/M549U as the engineering route.

Artifacts:

- `scripts/train_m1502_factor_transcodability.py`
- `scripts/audit_m1502b_ridge_factor_transcodability.py`
- `runs/m1502_factor_transcodability_v1/`
- `runs/m1502b_ridge_factor_transcodability_v1/`
- `tests/test_train_m1502_factor_transcodability.py`
- `tests/test_audit_m1502b_ridge_factor_transcodability.py`
