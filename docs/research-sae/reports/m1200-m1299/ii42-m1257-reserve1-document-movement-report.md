# M1257 Reserve1 Document Movement

## Goal

M1256 showed that the low-tail atom's own qrels-free features do not separate
help from harm.  M1257 checks the next observable layer: native document
movement between true `reserve0_s1` and `reserve1_s1`.

This is still an observability audit.  It does not promote a new default.

## Runs

Smoke:

- `runs/m1257_reserve1_document_movement_smoke_v1/`
- Datasets:
  `cqadupstack`, `scidocs`, `webis-touche2020`

Full `shared15`:

- JSON:
  `runs/m1257_reserve1_document_movement_v1/m1257_reserve1_document_movement.json`
- Markdown:
  `runs/m1257_reserve1_document_movement_v1/m1257_reserve1_document_movement.md`
- Query count: `1342`

## Reserve1 vs Reserve0

| Comparison | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `reserve1_s1 - reserve0_s1` | +0.000163 | +0.000200 | -0.000357 | -0.000132 | -0.000022 | +0.000414 |

This matches M1256/M1255: reserve1 is macro-positive overall, but trades off
NDCG/MRR/CUB for Recall/MAP.

## Movement Buckets

| Bucket | Count | AnyNeg | CubNeg | RankNeg | RecallNeg | MeanScoreDelta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `top100_new:0` | 479 | 0.1211 | 0.0042 | 0.1190 | 0.0000 | -0.014159 |
| `top100_new:1-2` | 772 | 0.1360 | 0.0117 | 0.1269 | 0.0091 | -0.009801 |
| `top100_new:3+` | 91 | 0.2967 | 0.0330 | 0.2857 | 0.0330 | -0.087201 |
| `top10_new:0` | 1249 | 0.1345 | 0.0088 | 0.1281 | 0.0072 | -0.005377 |
| `top10_new:1-2` | 93 | 0.2366 | 0.0323 | 0.2258 | 0.0108 | -0.167394 |
| `top100_jaccard:stable` | 479 | 0.1211 | 0.0042 | 0.1190 | 0.0000 | -0.014159 |
| `top100_jaccard:small_move` | 772 | 0.1360 | 0.0117 | 0.1269 | 0.0091 | -0.009801 |
| `top100_jaccard:large_move` | 91 | 0.2967 | 0.0330 | 0.2857 | 0.0330 | -0.087201 |

## Interpretation

Document movement is more informative than raw tail-atom magnitude, but still
not a complete separator.

The clearest risk signals are:

- `top100_new:3+` is much riskier than `top100_new:0` or `1-2`
- `top10_new:1-2` is strongly risky
- no row moved a protected top10 document out of top100, so the harm is not
  simple head ejection

The pattern suggests over-expansion: when the low-tail atom causes broad top100
or top10 churn, it is likely harmful.  Small top100 movement is less risky and
keeps some Recall/MAP upside.

## Decision

Do not promote `reserve1_s1` as default.

Do run one bounded movement-aware replay:

- accept reserve1 only if `top100_new <= 2`
- accept reserve1 only if `top10_new == 0`
- accept reserve1 only if both conditions hold

This is not another arbitrary threshold grid.  It is a direct test of the
observed document-movement failure mode.

If this movement constraint does not improve the frontier, stop query-time
guarding for reserve1 and move the low-tail signal into training/objective
design.
