# M1193 Mixed Proposal Smoke

## Purpose

M1192 showed that M1191 damage-veto is a useful control policy, but the current
proposal pair is structurally limited:

- aggressive: BM25 top3
- conservative: BM25 top1

M1193 tests one new proposal family before adding another learned gate:

- mixed: BM25 top2 + BM25 tail1

All variants use the same append8 / scale0.10 posting delta and native shared15
replay.

## Artifacts

- Script: `scripts/audit_m1193_mixed_proposal_smoke.py`
- Full JSON:
  `runs/m1193_mixed_proposal_smoke_v1/m1193_mixed_proposal_smoke.json`
- Full Markdown:
  `runs/m1193_mixed_proposal_smoke_v1/m1193_mixed_proposal_smoke.md`
- Smoke:
  `runs/m1193_mixed_proposal_smoke_smoke_v1/`

## Full Shared15 Macro

| Variant | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `oracle_mixed_pair` | +0.002308 | +0.004936 | +0.004592 | +0.004743 | +0.000869 |
| `top3` | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `mixed_top2_tail1` | +0.001957 | +0.003553 | +0.002756 | +0.002758 | -0.000310 |
| `top1` | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |

## Interpretation

Mixed top2+tail1 is not a better fixed proposal than top3.

It slightly improves MRR versus top3, but:

- MAP is lower
- NDCG is lower
- CUB is more negative
- Top95 overlap is also slightly lower

The oracle mixed pair is positive, but it is weaker than the existing
M1191/M1192 oracle pair.  That means mixed proposal adds some useful
alternatives, but not enough to justify integrating it into the current learned
gate.

## Decision

Reject `BM25 top2 + BM25 tail1` as the next proposal expansion.

This is a useful stop signal: do not tune mixed top/tail thresholds or scales.
The failure matches the earlier pure-tail instability: tail evidence can add
CUB in some places but destabilizes ranking and does not solve row harm cleanly.

## Next Direction

The next proposal should not be a naive BM25 tail mix.

Better candidates:

1. dual-source proposal: docs that appear in both BM25 and P1 candidates but are
   below the protected head
2. source-count proposal: high source_count / high normalized-score agreement
3. row-specific audit first: inspect cqadupstack and scidocs candidate rows to
   find whether harm is lexical-tail, semantic-tail, or normalization induced

Recommended next step: M1194 dual-source proposal smoke.  It should remain a
single proposal-family test, not a grid.
