# M1660A BMP Exact-TopK Correctness Patch

Date: 2026-07-11

Decision: **retain the official BMP design, apply one auditable correctness
patch, and require full FiQA exhaustive parity before any performance claim.**

## Preflight Failure

The unmodified official BMP source at commit `c0a17ffc` was built for native
ARM64 and tested with one term, 128 documents, block size 16, compressed range
maxima, and exact top-100 search.

The exhaustive result contained 100 documents. BMP returned only 96. The
missing rows were exactly the first four documents in the final qualifying
block.

Two source-level causes were reproduced:

1. the candidate-block loop fetched a current block but scored it only when a
   following block existed, so the final candidate block was never scored;
2. the initial per-term kth lower bound was installed as an exclusive heap
   threshold, excluding documents whose integer score equaled that safe bound.

This is an engine exactness defect, not learned-sparse model loss, document
ordering, or quantization quality.

## Minimal Patch

`patches/bmp-0.2-exact-topk.patch` makes only two corrections:

- initialize the integer threshold one unit below the inclusive kth lower
  bound;
- process every candidate block, including the final and empty-iterator cases.

It does not change block size, range maxima, compression, impact values,
query normalization, pruning alpha, term retention, or ranking scores.

The patched Rust crate passed its release tests. Repeating the identical
synthetic smoke returned 100/100 documents, with exact IDs and integer scores.

## Audit Boundary

The M1700 step-4 surface later exposed a third independent exactness defect.
For FiQA query `1826`, the exhaustive integer dot product is `65,806`; the
upstream `u16` accumulator wraps that score and drops the true top document.
The same patch therefore also promotes block-score accumulation, range upper
bounds, initial thresholds, and top-k scores to `u32`. Stored posting impacts
remain `u8`, so index bytes and quantization are unchanged. Regression tests
cover a `73,440` score and upper bound.

The full M1660/M1700 result must identify itself as:

- upstream source commit `c0a17ffc`;
- Python distribution `bmp 0.2.6` built from that source;
- the SHA-256 of `patches/bmp-0.2-exact-topk.patch`.

No engine-cost conclusion is accepted unless all 648 FiQA queries pass
exhaustive score-multiset and strict-boundary parity. The final combined patch,
SHA-256 `8d348124142b74bb35f31cb7b762338dc6c6e390ed0f0e9b08368f094d6f68b4`,
was built and rerun sequentially on spark-2 Linux/aarch64. Both the root and
M1700 step-4 indexes achieved strict-boundary parity `1.0` over all 648
queries. The candidate's `65,806` maximum score therefore verifies the `u32`
path beyond the original overflow boundary.

This closes the correctness defect. It does not promote M1700: the candidate
index was `1.073x` larger and its same-machine BMP p50 latency was `1.174x` the
root, in addition to the model-side maxDF and decoded-posting cost failures
recorded in the M1700 report.
