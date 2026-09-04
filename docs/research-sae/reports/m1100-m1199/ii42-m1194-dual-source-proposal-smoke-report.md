# M1194 Dual-Source Proposal Smoke

## Purpose

M1193 rejected naive `BM25 top2 + BM25 tail1`.  M1194 tests a more constrained
proposal family:

- documents that appear in both BM25 and P1 candidates
- below the protected fused top95 head
- nearest by native fused rank

This is meant to avoid pure lexical tail instability while still adding a
retrieval-conditioned candidate signal.

## Artifacts

- Script: `scripts/audit_m1194_dual_source_proposal_smoke.py`
- Full JSON:
  `runs/m1194_dual_source_proposal_smoke_v1/m1194_dual_source_proposal_smoke.json`
- Full Markdown:
  `runs/m1194_dual_source_proposal_smoke_v1/m1194_dual_source_proposal_smoke.md`
- Smoke:
  `runs/m1194_dual_source_proposal_smoke_smoke_v1/`

## Full Shared15 Macro

| Variant | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `oracle_dual_pair` | +0.001344 | +0.004431 | +0.004204 | +0.004554 | +0.000814 |
| `top3` | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `top1` | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |
| `dual_tail_docs3` | -0.000010 | +0.000363 | +0.000234 | -0.000193 | +0.000365 |

## Interpretation

Dual-source tail has the same basic shape as previous tail proposals:

- it improves CUB
- it does not reliably improve top100 recall
- it weakens ranking movement
- it can regress MRR

The oracle pair (`dual_tail_docs3` vs `top1`) is positive, but it is weaker than
the existing M1191/M1192 oracle pair (`top3` vs `top1`).  That means this
proposal family adds some useful alternatives but does not improve the current
frontier.

## Decision

Reject `dual_tail_docs3` as the next proposal expansion.

Do not add it to the M1191 damage-veto gate.

## Implication

The current evidence says tail-like proposals, even dual-source tail proposals,
are not the right next route.  They improve candidate availability but fail to
convert that into stable ranking gains.

The next useful step should inspect row-specific failure mechanics rather than
try another tail-flavored proposal immediately.

Recommended next step:

- M1195: cqadupstack/scidocs candidate movement audit
- compare baseline, top3, top1, and veto on harmed rows
- identify whether harm comes from qrels-positive displacement, lexical
  over-admission, atom perturbation size, or normalized score shift

Only after that should another proposal source be tested.
