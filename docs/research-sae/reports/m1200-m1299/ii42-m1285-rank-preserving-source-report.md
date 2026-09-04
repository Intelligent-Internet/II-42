# M1285 Rank-Preserving Source Construction Audit

## Question

M1283 and M1284 showed that M721b rank harm is not separable enough after
selection.  M1285 moves the constraint before selection:

> Can a pair-boundary source penalize baseline-head impact while preserving
> target visibility?

This is a source frontier audit only.  It does not run native replay.

## Surface

Hard-row smoke:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Variants:

- baseline `rp1`
- ratio penalties against top10/top50 baseline head support
- rank penalties against top10/top50 baseline head support
- budgets 16, 32, 64

## Result

Best eval frontier:

| Variant | TargetVis | TargetShare | NegShare | Head10Abs | Head50Abs |
| --- | ---: | ---: | ---: | ---: | ---: |
| `rp1_b64` | 0.883729 | 0.505083 | 0.001318 | 0.007647 | 0.037771 |
| `ratio_head50_b64` | 0.874835 | 0.500000 | 0.001318 | 0.006957 | 0.034779 |
| `ratio_head10_b64` | 0.880764 | 0.503389 | 0.001318 | 0.007150 | 0.036197 |
| `rp1_b32` | 0.663043 | 0.757907 | 0.000000 | 0.007965 | 0.039492 |
| `rank_head50_l025_b64` | 0.337615 | 0.192959 | 0.000188 | 0.000363 | 0.002421 |

## Interpretation

M1285 does not find a replay-worthy rank-preserving source.

Weak ratio penalties barely reduce head impact and do not reduce negative-only
share.  Strong rank penalties remove head impact, but they destroy target
visibility.  This matches M1284: baseline-head support is not the right
controlling interface for M721b harm.

## Decision

Do not run native replay for M1285 variants.

Do not continue with:

- `pair_hgb_rp1 - head impact` score variants;
- head10/head50 rank penalties;
- head-impact ratio variants;
- further M721b exact-source filter/penalty tuning.

This closes the local M721b repair loop.  M721b/M722 remain valuable as proof
that pair-interaction movement exists, but current selected-source and
head-risk views are not enough to make it row-safe.

The next branch should return to the wider M700-M710 diagnosis: target atoms
are visible in a wider source, but budget compression and ordering are the
real blocker.  The next experiment should change the source interface, not
patch the M721b selected atoms.

## Artifacts

- Script:
  `scripts/audit_m1285_rank_preserving_source.py`
- JSON:
  `runs/m1285_rank_preserving_source_smoke_v1/m1285_rank_preserving_source.json`
- Markdown:
  `runs/m1285_rank_preserving_source_smoke_v1/m1285_rank_preserving_source.md`
