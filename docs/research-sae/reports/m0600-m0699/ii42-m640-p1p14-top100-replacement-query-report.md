# M640 / P1.14 Top100 Replacement Query Report

Status: `not_promoted_first_stage_boundary_limit_reached`

## Goal

M640 tests whether M638/M639 failed because the boundary objective was too
smooth. Instead of scalar boundary blending, it trains an explicit replacement
surrogate: a boundary positive should beat the lowest non-relevant top100 tail
candidate, while head positives and dense-local geometry remain anchored.

This is still a first-stage query compiler probe:

- no BM25
- no fixed alpha
- no learned gate
- no dataset-specific tuning
- frozen document postings
- train query-side compiler only

## Runs

All runs use the M638 seed/split shape for direct comparison.

| Run | Replacement weight | Tail width | Head anchor | Positive anchor | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| A | 5.0 | 12 | 0.25 | 0.15 | Recall crosses, MAP/NDCG loss remains |
| B | 2.5 | 8 | 0.55 | 0.35 | best tradeoff: all-query metrics improve, boundary MAP still negative |
| C | 2.0 | 6 | 0.80 | 0.60 | more conservative, no boundary MAP fix |

## Dev Trace Summary

| Run | Epoch | Boundary dR@100 | Boundary dMAP | Boundary dNDCG | Boundary dO@100 | All dMAP | All dNDCG | All dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 2 | +0.013889 | -0.000356 | -0.005952 | +0.000000 | -0.000709 | -0.001266 | +0.000938 |
| B | 2 | +0.013889 | -0.000343 | -0.005952 | +0.000833 | +0.001009 | +0.000098 | +0.001302 |
| B | 3 | +0.013889 | -0.000388 | -0.005952 | -0.003333 | +0.000590 | -0.000166 | -0.000781 |
| C | 2 | +0.013889 | -0.000339 | -0.005952 | +0.000833 | -0.000774 | -0.002772 | +0.001146 |

## Key Evidence

M640-B is the strongest first-stage boundary result so far:

- boundary Recall@100 improves by +0.013889
- all-query MAP improves by +0.001009
- all-query NDCG improves by +0.000098
- all-query dense overlap improves by +0.001302

But the same checkpoint still fails the conservative gate because boundary
MAP@100 is -0.000343 and boundary NDCG@10 is -0.005952. This is not a metric
artifact: it means the newly crossed positives are still not ordered safely
inside the boundary slice.

## Diagnosis

M640 proves that the issue is not lack of a direct replacement surrogate. The
first-stage query compiler can:

1. move boundary positives across top100
2. preserve or improve all-query aggregate metrics
3. preserve dense overlap at the all-query level

The remaining blocker is local boundary ordering. The vector-level query update
has no candidate-specific feature context, so it cannot reliably distinguish
which tail negative should be replaced without disturbing the local ordering of
the boundary slice.

Further scalar loss tuning is now low-value. We have mapped the phase transition:

- too conservative: rank improves but Recall does not move
- enough pressure: Recall moves but boundary MAP/NDCG drops
- replacement surrogate: all-query metrics improve, but boundary rank-safety
  still fails

## Decision

Do not promote M640.

Do not expand to shared15 or official full matrix.

Preserve M640-B as evidence that first-stage query compiler can improve global
ranking while crossing boundary Recall, but not enough to satisfy local
rank-safety.

## Next Step

Move to a second-stage native reranker probe.

The next stage should freeze P1/M549 candidate generation and train a global,
auditable scorer over candidate features:

- P1 score
- BM25 score
- P1 rank
- BM25 rank
- reciprocal ranks
- atom overlap / weighted atom hits
- query and document length
- term coverage if available

Acceptance must still be strict:

- improve Recall@100 and MAP@100 over P1-a0125/M549 baseline
- do not materially regress NDCG@10 or MRR@20
- no dataset-specific tuning
- evaluate through the same candidate pool and native-style rows

M640 closes the current first-stage boundary scalar-loss line for now. The
highest-value path is to attack the scorer/reranker bottleneck directly.
