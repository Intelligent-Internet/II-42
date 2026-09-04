# M1201 Consensus Candidate Guard Shared15

## Purpose

M1201 expands the M1199/M1200 branch from hard rows to full shared15. The test
keeps the same fixed consensus atom proposal and the same qrels-free top1000
candidate-overlap guard. No dataset-specific threshold is tuned.

## Artifacts

- Script used: `scripts/audit_m1200_candidate_boundary_guard.py`
- JSON:
  `runs/m1201_consensus_candidate_guard_shared15_v1/m1200_candidate_boundary_guard.json`
- Markdown:
  `runs/m1201_consensus_candidate_guard_shared15_v1/m1200_candidate_boundary_guard.md`

## Surface

- Datasets: 15 shared15 rows
- Query count: 1342
- Proposal: `consensus2_mean_s0.10`

## Macro

| Source | Take | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `consensus2_mean_s0.10` | 1.000 | +0.000379 | +0.000833 | +0.000347 | +0.000938 | -0.000035 |
| `guard_t0.98` | 0.986 | +0.000312 | +0.000789 | +0.000270 | +0.000404 | -0.000113 |
| `guard_t0.99` | 0.882 | +0.000144 | +0.000560 | +0.000201 | +0.000306 | +0.000059 |
| `guard_t0.995` | 0.721 | -0.000030 | +0.000078 | +0.000014 | +0.000175 | +0.000057 |
| `guard_t0.999` | 0.590 | +0.000000 | +0.000022 | +0.000017 | +0.000037 | -0.000003 |

## Interpretation

This is the first positive broader signal in this branch:

- `guard_t0.99` is macro-positive on all five tracked metrics.
- It uses one global qrels-free threshold.
- It keeps 88.2% of consensus proposal applications.
- It fixes the unguarded consensus CUB regression.

The signal is still small and row-level regressions remain, especially:

- `cqadupstack`: MAP/NDCG remain slightly negative.
- `nfcorpus`: Recall/CUB/NDCG regress under `guard_t0.99`.
- `scidocs` and `trec-covid`: NDCG remains slightly negative.
- `webis-touche2020`: Recall remains slightly negative.

## Decision

Promote this branch from hard-row smoke to broader validation candidate, not to
final default.

The next useful step is not another scalar threshold search. It is a learned or
rule-based global guard that combines:

- candidate-k overlap for CUB safety;
- rank-order/head stability for NDCG safety;
- consensus atom compactness for proposal quality.

Stop condition for the next step: if a combined qrels-free guard cannot improve
row-level safety while preserving the M1201 macro-positive contract, keep
`guard_t0.99` as the best fixed baseline and move to another proposal source.
