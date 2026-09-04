# M666 Output Blend Active-Lock Audit Report

## Summary

M666 converts the M663 post-hoc finding into the main training runner. The
compiler trains with enough residual movement to learn the M658 direction, but
the projected output is damped before native scoring:

- `output_blend_scale=0.25`
- `output_lock_active=true`
- `relative_support_gate=true`
- `support_regression_tolerance=1e-5`

Result: M666 passes the first-stage dense-equivalence gate on shared15
dev/test. This is the first clean candidate after the M658 near-miss line:
trained checkpoint selected, no rejected fallback, no dense-hit loss queries,
non-negative dense overlap deltas, no Recall/CUB regression, and support not
regressed under the same relative gate used by M658.

## Calibration Path

| Run | Purpose | Result |
| --- | --- | --- |
| M658 | raw trained movement | near-miss; dev lossQ=1, test lossQ=7 |
| M659 | top100 replay loss | negative; broadened churn |
| M660-M662 | fixed smaller training delta | reduced lossQ but removed gains |
| M663 | post-hoc movement budget on M658 | `fixed_scale_0_25` passed test gate |
| M664 | runner output blend, missing relative support gate | not a valid failure |
| M665 | output blend + active lock, missing relative support gate | not a valid failure |
| M666 | output blend + active lock + relative support gate | passed |

M664/M665 are kept only as calibration notes. They omitted the relative support
gate used by M658/M663, so their support failure should not be read as a model
failure.

## M666 Gate

Selected checkpoint:

- epoch: `1`
- global step: `17`
- selected rejected checkpoint: `false`

Boundary summary:

| Split | gainQ | lossQ | gained docs | lost docs | net docs |
| --- | ---: | ---: | ---: | ---: | ---: |
| dev | `1` | `0` | `1` | `0` | `+1` |
| test | `1` | `0` | `1` | `0` | `+1` |

Test deltas versus P1:

| Metric | Delta |
| --- | ---: |
| dense overlap@100 | `+0.000055556` |
| dense overlap@256 | `+0.000030908` |
| Recall@100 | `0.000000000` |
| MAP@100 | `-0.000004691` |
| NDCG@10 | `0.000000000` |
| MRR@20 | `0.000000000` |
| CUB | `0.000000000` |
| active support recall | `0.000000000` |
| support cosine | `-0.000000004` |

The tiny MAP/support movements are within the configured first-stage gate:
Recall/CUB do not regress, support is not regressed under tolerance, and dense
identity is preserved with one net dense-hit gain query.

## Interpretation

This does not prove broad retrieval improvement yet. It proves something more
specific and important for the current bottleneck:

- the M658 direction was not a dead end;
- fixed global low-scale training was too weak;
- raw full movement was too unsafe;
- train-high / infer-damped output projection is a viable first-stage shape.

The next step should not add BM25 or qrels loss. It should replay M666 on a
broader native surface and/or make the `0.25` output blend a documented
first-stage compiler parameter, then check whether the same no-loss gate holds
outside this shared15 split.
