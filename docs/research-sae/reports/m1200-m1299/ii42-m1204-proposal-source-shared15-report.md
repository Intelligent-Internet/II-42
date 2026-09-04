# M1204 Proposal Source Shared15 Sweep

M1204 expands the M1199 proposal-source comparison from hard rows to the full
shared15 native replay surface.  This tests whether the consensus atom proposal
was only locally safer, or whether older top-doc posting deltas still carry
stronger global signal.

## Setup

- Surface: shared15 native replay
- Query count: 1342
- Baseline: frozen P1 native query
- Sources compared: `top3`, `top1`, `consensus2_mean_s0.10`,
  `consensus2_mean_s0.05`, `compact3_sum_s0.05`

## Macro Result

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `top3` | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 | 0.037365 |
| `top1` | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 | 0.022423 |
| `consensus2_mean_s0.10` | +0.000379 | +0.000833 | +0.000347 | +0.000938 | -0.000035 | 0.007607 |
| `compact3_sum_s0.05` | +0.000439 | +0.001193 | +0.000386 | +0.000701 | -0.000127 | 0.004880 |
| `consensus2_mean_s0.05` | +0.000313 | +0.000438 | +0.000404 | +0.000390 | -0.000066 | 0.002944 |

## Interpretation

This is the first clear sign that the hard-row-only conclusion was too narrow.
`top3` is much stronger than the consensus proposal on full shared15, even
though it looked unsafe on the earlier hard subset.  `top1` is also important:
it keeps CUB positive and gives larger MAP/NDCG/MRR gains than the consensus
family.

The problem is not that top-doc proposal sources are dead.  The problem is that
the aggressive source needs a stronger qrels-free safety policy.  This points
back to retrieval-conditioned unified-posting: choose or scale the query-local
posting delta based on candidate movement risk.

## Decision

- Promote `top3` source back into the active branch as the high-gain proposal.
- Use `top1` as the safe fallback source, not just baseline fallback.
- Stop consensus-only proposal tuning for now; it is safer on hard rows but
  underpowered on the broader surface.

## Artifacts

- JSON: `runs/m1204_proposal_source_shared15_v1/m1199_consensus_atom_proposal_smoke.json`
- Markdown: `runs/m1204_proposal_source_shared15_v1/m1199_consensus_atom_proposal_smoke.md`
- Script: `scripts/audit_m1199_consensus_atom_proposal_smoke.py`
