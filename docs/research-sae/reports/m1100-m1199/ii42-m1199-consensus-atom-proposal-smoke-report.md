# M1199 Consensus Atom Proposal Smoke

## Purpose

M1197/M1198 showed that the BM25-top document proposal moves useful candidates
but damages rank/order geometry on hard rows. M1199 changes the proposal shape:
instead of appending atoms from all BM25-top document atoms, it tests compact
atom-level proposals that require cross-document consensus or a smaller
aggregate atom set.

## Artifacts

- Script: `scripts/audit_m1199_consensus_atom_proposal_smoke.py`
- JSON:
  `runs/m1199_consensus_atom_proposal_smoke_v1/m1199_consensus_atom_proposal_smoke.json`
- Markdown:
  `runs/m1199_consensus_atom_proposal_smoke_v1/m1199_consensus_atom_proposal_smoke.md`

## Surface

- Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: 249
- Baseline: current P1-a0125 native query

## Macro

| Source | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `top3` | +0.003630 | -0.000027 | -0.000027 | -0.004234 | -0.000522 |
| `top1` | -0.000472 | +0.000414 | +0.000244 | -0.000262 | -0.000773 |
| `consensus2_mean_s0.10` | +0.001135 | +0.000756 | +0.000611 | +0.000914 | -0.000522 |
| `consensus2_mean_s0.05` | +0.001135 | +0.000400 | +0.000876 | +0.000915 | -0.000773 |
| `compact3_sum_s0.05` | +0.001863 | +0.000757 | +0.000570 | +0.001223 | -0.001325 |

## Interpretation

`consensus2_mean_s0.10` is a real proposal-shape improvement on the hard-row
surface:

- It keeps Recall/MAP/NDCG/MRR positive.
- It reduces entrants from 3.06/query under top3 to 0.59/query.
- It greatly improves cqadupstack, where top3/top1 were strongly negative.

The remaining blocker is CUB, especially scidocs. This means the consensus atom
proposal is more rank-stable but still needs candidate-boundary protection.

## Decision

Keep the consensus proposal branch. Do not return to BM25-top document-level
top3 as the main expansion. The next test should protect candidate-k/CUB rather
than top100 rank order.
