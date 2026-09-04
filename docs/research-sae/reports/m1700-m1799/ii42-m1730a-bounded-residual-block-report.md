# M1730A Bounded Residual Block Report

## Decision

**Stop the bounded residual-block route before native engineering or model
training.** The formal v2 audit passes every evaluator-integrity check but
fails both independent mechanism gates:

1. a 96-byte residual payload does not preserve dense ordering even when every
   M1727 candidate is scored;
2. compact safe block summaries cannot prune any selected block, so all
   candidates must be decoded.

The initial v1 used rotated FP32 vectors for its exact control and differed
from the cached M1727 O@100 by `0.00001` at one boundary. v2 restores original
cached vectors for exact controls and block bounds. No representation, block,
cost, or gate parameter changed. Only v2 is formal.

## Frozen Surface

- M1600 20,000-document deterministic train sample for PCA fitting;
- disjoint M1600 1,000-query/8,988-document validation pool;
- exact M1727A-v2 frozen-logit expansion and BM25 candidates;
- M1600 8x512 document-assignment parity `1.000000`;
- fixed rank-96 PCA residual, INT8 coefficients, 99.9th percentile scale;
- fixed 16-entry geometric blocks and five Lloyd iterations;
- fixed rank-96 INT8 block summaries with safe full-space radius;
- no qrels, training, threshold search, score calibration, or parameter grid.

## Quality

| Surface | O@10 | O@100 | O@256 |
| --- | ---: | ---: | ---: |
| Exact score, all M1727 candidates | 0.987000 | 0.889080 | 0.785539 |
| Compact residual, all candidates | 0.817300 | 0.703900 | 0.685762 |
| Exact score, safe-block decoded | 0.987000 | 0.889080 | 0.785539 |
| Compact residual, safe-block decoded | 0.817300 | 0.703900 | 0.685762 |

The exact all-candidate row reproduces M1727/M1728 at every depth. The exact
safe-block row matches it exactly, proving that the block-bound implementation
is correct.

Compact retention relative to exact is:

- O@10 `82.81%`;
- O@100 `79.17%`;
- O@256 `87.30%`.

All are below the fixed 95% gate. Mean reconstruction cosine is `0.863818`,
with p05 `0.819715`. The omitted residual dimensions materially control top-k
order; block selection cannot repair a scorer that already fails when every
candidate is available.

## Block And Storage Cost

| Measurement | Mean | p95 |
| --- | ---: | ---: |
| Candidate documents | 1,178.475 | 1,459.050 |
| Summary scans | 171.998 | 209.100 |
| Opened blocks | 171.998 | 209.100 |
| Decoded posting entries | 1,617.025 | 1,986.150 |
| Decoded unique documents | 1,178.475 | 1,459.050 |

The balanced namespace solves the M1544 summary-scan explosion: M1730 scans
about 172 summaries/query rather than roughly 12,000. It does not solve bound
selectivity. Every scanned block is opened and every candidate is decoded.
The rank-96 quantized centroid plus safe radius is too loose after accounting
for omitted dimensions and within-block variation.

Storage shape is acceptable in isolation:

- document payload: 114 bytes/document;
- block summaries: 84.31 bytes/document;
- global bases/scales: 33.24 bytes/document on this corpus;
- total: 231.54 bytes/document, below the 256-byte gate;
- semantic posting reads: `0.179909x`;
- max DF: `0.009457`.

The route fails on score fidelity and decoded work, not metadata bytes or
posting-list balance.

## Cross-Experiment Meaning

M1730 answers the exact remaining question after M1727/M1728:

- M1727 candidate admission is real;
- M1728 one-hop score resolution is weak;
- compact forward residuals do not restore enough resolution at 96 bytes;
- safe geometric block summaries cannot bound the missing full-space energy
  tightly enough to avoid decoding the complete candidate union.

This also refines M1544. Smaller balanced lists reduce summary scans by two
orders of magnitude, but the geometric-bound failure remains. M1710's poor
residual-code score was therefore not only an access-policy problem.

## Stop Boundary

Do not continue with:

- another PCA rank, quantile, block size, route count, or residual width;
- learned summary thresholds or approximate pruning after observing the safe
  failure;
- text-to-residual compiler training against this failed representation;
- native/qrel evaluation that hides the structural score gap.

The strict single-vector dense-root route is now closed for both one-hop and
bounded residual scoring. A future continuation must change the representation
itself, for example a genuinely token-level multi-vector late-interaction
model trained for that engine shape. That would be a new model and product
contract, not M1731 or a repair of M1730.

## Reproducibility

- host: `spark-1`;
- formal run:
  `/home/huoju/leask/runs/ii42-m1730-residual-blocks-v1/runs/m1730a-bounded-residual-blocks-seed1730-v2`;
- excluded integrity-debug run: corresponding `v1` directory;
- summary SHA-256:
  `bed38631fbfee7171da60a373a92ea1950ce88dd22ed2748ea4ec6db2964f1f8`;
- local result: `ii42-m1730a-bounded-residual-blocks-result.json`.
