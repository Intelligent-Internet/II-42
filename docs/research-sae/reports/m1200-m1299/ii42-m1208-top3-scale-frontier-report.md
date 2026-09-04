# M1208 Top3 Scale Frontier

M1208 tests whether the CUB loss in the current aggressive `top3` proposal is
mainly caused by over-amplification.  It keeps the proposal source fixed and
only sweeps the `top3` posting-delta scale on the same shared15 surface.

This is a proposal-shape audit, not a new model.

## Setup

- Surface: native shared15 cache
- Query count: 1342
- Baseline: frozen P1 native baseline
- Compared actions: `top3` at scales 0.025, 0.05, 0.075, 0.10, 0.125
- Safety anchor: `top1_s0.10`

## Macro Result

| Variant | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `top3_s0.125` | 1 | +0.002088 | +0.004875 | +0.003740 | +0.003156 | -0.000232 |
| `top3_s0.075` | 1 | +0.001610 | +0.003509 | +0.003358 | +0.002682 | -0.000033 |
| `top3_s0.10` | 1 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `top3_s0.05` | 1 | +0.001281 | +0.002477 | +0.002085 | +0.002092 | -0.000093 |
| `top1_s0.10` | 0 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |
| `top3_s0.025` | 1 | +0.000405 | +0.001113 | +0.000635 | +0.000128 | -0.000388 |

## Interpretation

Scale is a real knob, but not a complete fix.

`top3_s0.075` is the most useful fixed-scale discovery from this audit.  It
nearly removes the CUB harm of `top3_s0.10` while improving NDCG and MRR, but
it gives back some Recall and MAP.  It is a safer operating point, not a clear
new default.

`top3_s0.125` gives the largest rank gains, but the CUB loss grows.  This is
not acceptable as a default because it amplifies the same structural safety
problem that M1191 was designed to veto.

`top1_s0.10` remains the safest fixed action, but it is too conservative to
replace the high-gain frontier.

## Decision

- Do not promote any fixed `top3` scale as the new default.
- Keep `top3_s0.075` as a safety-biased intermediate action.
- Use this result to test an ordered-risk policy rather than continue a blind
  scale sweep.

## Next Direction

The next non-micro-tuning experiment is an ordered action policy:

1. Use `top3_s0.10` where risk looks low.
2. Fall back to `top3_s0.075` for medium-risk queries.
3. Fall back to `top1_s0.10` for high-risk queries.

This differs from M1206 because the intermediate action is not another source;
it is the same useful source at a safer scale.  The hypothesis is that risk is
more learnable as action strength than as a hard top3-vs-top1 class.

## Artifacts

- JSON: `runs/m1208_top3_scale_frontier_v1/m1208_top3_scale_frontier.json`
- Markdown: `runs/m1208_top3_scale_frontier_v1/m1208_top3_scale_frontier.md`
- Script: `scripts/audit_m1208_top3_scale_frontier.py`
