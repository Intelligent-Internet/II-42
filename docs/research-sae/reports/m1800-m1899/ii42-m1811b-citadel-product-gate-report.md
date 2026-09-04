# M1811B CITADEL Product-Gate Correction

## Correction

M1811A's recorded decision `stop_citadel_product_import` is not a valid product
conclusion. Its pre-registered gate assumed that CITADEL's token projection must
remain useful under unrestricted MaxSim. The official architecture does not
make that claim: learned keys, route weights, and 32-dimensional token vectors
are jointly trained and form one score.

The failed unrestricted control is still informative:

- unrestricted token MaxSim MRR@100: `0.220275`;
- exhaustive learned-route MRR@100: `0.800034`;
- exhaustive learned-route Recall@100: `1.000000`.

Routing is therefore not an approximation wrapped around an independent
MaxSim representation. It is part of the representation.

## Product-Surface Result

The official query shape also passes without using query top-2:

| Policy | Exact MRR | Candidate O@100 | Direct O@100 | Direct MRR | KB/query | KB/doc |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| q1/d5/prune0/cap1 | 0.798498 | 0.942720 | 0.934320 | 0.791504 | 35.09 | 6.65 |
| q1/d5/prune0.5/cap1 | 0.794340 | 0.937720 | 0.932040 | 0.791442 | 30.75 | 5.05 |
| q1/d2/prune0.5/cap1 | 0.778601 | 0.905560 | 0.903560 | 0.776101 | 20.47 | 2.54 |

The M1811A best diagnostic policy, q2/d5/prune0/cap8, reaches direct O@100
`0.993320`, direct MRR `0.797368`, `113.81 KiB/query`, and `6.65 KiB/doc`.
It is not selected for the next test because official training used query
top-1.

## Revised Decision

**Authorize an independent small full-corpus BEIR replay.** This is not a model
promotion. The MS MARCO paired canary has established that learned routing can
be represented and replayed as one key-to-vector-payload inverted index under
the product cost envelope. M1812 must now separate:

1. route/index fidelity on unseen corpora;
2. the external CITADEL checkpoint's retrieval quality;
3. the existing PPLX dense-root quality.

The CLS dense branch remains diagnostic-only and cannot enter the product gate.
