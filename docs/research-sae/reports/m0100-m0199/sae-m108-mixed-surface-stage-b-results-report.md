# SAE M108 Mixed-Surface Stage-B Results Report

Date: 2026-05-23

## Decision

M108 tests whether the M107 larger-supervision Stage-B ranker can recover the
stronger M105 ranking delta on the original M105/M97 postings surface by adding
M97 into checkpoint selection, training loss, or normalization.

The answer is negative for the simple mixing family. All variants remain safely
positive against fixed BM25+SAE, and M108 selection-only improves transfer NDCG
over M107 transfer, but none recover M105's M97-local NDCG/MAP delta. The best
M108 variant is selection-only:

- M107 transfer eval delta on M97: Recall/MRR/NDCG/MAP
  `+0.0040/+0.0058/+0.0022/+0.0020`.
- M108 selection-only eval delta on M97: `+0.0041/+0.0050/+0.0029/+0.0025`.
- M105 in-surface eval delta on M97: `+0.0022/+0.0100/+0.0044/+0.0047`.

Therefore M108 closes with `transfer-positive-below-m105`. The next step should
not be more mixture-weight sweeping. M109 should introduce an explicitly
surface-aware residual/calibration path, or a richer feature family that lets
one exported ranker preserve both broad M81 supervision and M97-local ranking
behavior.

## Runs

| Variant | Run | Normalization | Transfer Train Weight | Best Epoch |
| --- | --- | --- | ---: | ---: |
| M108A selection-only | `/home/huoju/leask/runs/m108-mixed-surface-stageb-v1` | primary | 0.00 | 300 |
| M108B trainmix035 | `/home/huoju/leask/runs/m108-mixed-surface-stageb-trainmix035-v1` | primary | 0.35 | 300 |
| M108C combinednorm | `/home/huoju/leask/runs/m108-mixed-surface-stageb-combinednorm-v1` | combined M81+M97 | 0.00 | 290 |
| M108D trainmix100 | `/home/huoju/leask/runs/m108-mixed-surface-stageb-trainmix100-v1` | primary | 1.00 | 300 |

All runs use:

- Primary training surface: full M81 train materialization from M107.
- Transfer surface: original M105/M97 postings-generated surface.
- Representation: `m96_k512`.
- Candidate configs: `d8_p16_bm25100`, `d8_p32_bm25100`,
  `d16_p16_bm25100`, `d16_p32_bm25100`.
- Checkpoint selection: primary validation plus transfer validation.

## Data

| Surface | Documents | Queries | Runtime Rows |
| --- | ---: | ---: | ---: |
| Primary M81 full-train surface | 71,417 | 3,920 | 15,680 |
| Transfer M105/M97 surface | 30,059 | 886 | 3,544 |

## Transfer Eval Delta Vs Fixed BM25+SAE

| Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M105 in-surface reference | +0.0022 | +0.0100 | +0.0044 | +0.0047 |
| M107 transfer reference | +0.0040 | +0.0058 | +0.0022 | +0.0020 |
| M108A selection-only | +0.0041 | +0.0050 | +0.0029 | +0.0025 |
| M108B trainmix035 | +0.0042 | +0.0041 | +0.0028 | +0.0025 |
| M108C combinednorm | +0.0042 | +0.0050 | +0.0029 | +0.0024 |
| M108D trainmix100 | +0.0042 | +0.0041 | +0.0027 | +0.0025 |

## Primary Eval Delta Vs Fixed BM25+SAE

| Variant | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M107 full-train reference | +0.0029 | +0.0114 | +0.0067 | +0.0041 |
| M108A selection-only | +0.0030 | +0.0108 | +0.0067 | +0.0042 |
| M108B trainmix035 | +0.0030 | +0.0106 | +0.0067 | +0.0041 |
| M108C combinednorm | +0.0030 | +0.0107 | +0.0066 | +0.0041 |
| M108D trainmix100 | +0.0030 | +0.0108 | +0.0066 | +0.0041 |

Primary-surface quality is stable across the variants. The failure is specific
to recovering M105's M97-local ranking delta, not to losing broad M81
supervision.

## Interpretation

M108 rules out three simple explanations:

1. **Checkpoint selection only is not enough.** It improves M107 transfer
   NDCG from `+0.0022` to `+0.0029`, but still misses M105's `+0.0044`.
2. **Adding M97 train rows directly to the loss is not enough.** Transfer
   train weights `0.35` and `1.0` do not improve M97 NDCG/MAP and slightly
   reduce MRR.
3. **Combined M81+M97 feature normalization is not enough.** The result is
   nearly identical to primary-only normalization.

The remaining likely issue is model expressiveness at the surface level. The
single global residual ranker learns a broad safe correction, but cannot
simultaneously reproduce M105's local M97 behavior and M107's broader M81
behavior through scalar mixture or normalization changes alone.

## Next Step

M109 should test a controlled surface-aware ranker without changing the runtime
contract too much:

1. Add runtime-safe query/surface descriptors, not dataset IDs. Candidate
   signals include query family derived from query id style, query length,
   BM25 concentration, SAE concentration, candidate overlap, and candidate
   count.
2. Split the residual head into a global head plus a small gated local residual
   head. The gate must use only runtime-safe descriptors.
3. Keep fixed BM25+SAE initialization and the M108 two-surface selection gate.
4. Acceptance target: preserve M107 primary eval NDCG delta near `+0.0067`
   while recovering transfer eval NDCG/MAP to at least `+0.0035/+0.0035`.

If M109 cannot recover M97-local ranking through runtime-safe surface-aware
features, the product-safe route should prefer the stable M107/M108 global
ranker and stop trying to exactly reproduce M105's local delta.
