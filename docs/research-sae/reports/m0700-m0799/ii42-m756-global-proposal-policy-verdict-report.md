# M756 Global Proposal Policy Verdict

## Purpose

M756 summarizes the follow-up after M750/M752.  The question was whether the
native-context route can move from qrels-derived per-query coordinate proposals
to a qrels-free global proposal policy.

## M752 Full-Query Replay

M752 fixed the M750 evaluation leak by including all query rows in the macro.
Queries without replay attempts are counted as baseline no-op.

| Policy | Queries | Applied | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| O100 filter | 271 | 17 | +0.000176 | +0.000059 | +0.000000 | +0.000171 | +0.000000 | 1 |
| O128p99 filter | 271 | 14 | +0.000070 | +0.000471 | +0.001845 | +0.000023 | +0.000000 | 1 |

M752 is positive, but its attempt pool is still M654-derived and therefore not
fully deployable.

## M754 Qrels-Free Global Proposals

M754 freezes global `(dim, direction, scale)` proposals from dev accepted
attempts and replays those fixed proposals on all dev/test queries.

### Oracle Feasibility

The global proposal pool itself is strong:

| Proposal pool | Test attempts | Oracle dMAP | Oracle dNDCG | Oracle dMRR | Oracle dCUB | dO@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| top16 | 4336 | +0.000829 | +0.001402 | +0.000226 | +0.000063 | +0.000000 |
| top32 | 8672 | +0.001374 | +0.001711 | +0.000291 | +0.000043 | +0.000000 |
| top64 | 17344 | +0.001395 | +0.001711 | +0.000291 | +0.000050 | +0.000000 |

This is a meaningful result: the proposal source can be made qrels-free at
test time without losing the oracle opportunity.

### Learned Selector

The current accepted-region selector does not yet recover that oracle safely:

| Proposal pool | Applied | Accepted hits | dMAP | dNDCG | dMRR | dCUB | dO@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| top16 O100 | 10 | 1 | -0.000006 | -0.000010 | +0.000000 | +0.000000 | +0.000000 | 0 |
| top32 O100 | 268 | 33 | +0.000164 | +0.000322 | -0.000451 | -0.000059 | +0.000000 | 0 |
| top64 O100 | 99 | 19 | +0.000160 | +0.000415 | +0.000000 | -0.000001 | +0.000000 | 0 |

The top64 CUB failure is numerically tiny, but the strict gate still fails.
More importantly, top32 shows over-application and MRR/CUB loss.

## M755 Utility Selector Diagnostic

A metric-aware utility target was tested with ridge, gradient boosting, and
random forest regressors.  It did not solve the selection problem:

- ridge improved MAP/NDCG but had negative CUB on test,
- gradient boosting was too conservative and regressed MAP,
- random forest improved CUB/MAP but regressed MRR.

So the current failure is not just the binary accepted label.  The selector
needs a listwise or constrained objective that controls query-level tradeoffs.

## M757 Label Diagnostic

A sparse best-row label was also tested on the top64 global proposal pool:

| Label | Train positives | Test applied | Test accepted hits | dMAP | dNDCG | dCUB | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| selected best row | 74 | 9 | 2 | -0.000014 | -0.000057 | +0.000046 | 0 |
| accepted attempt | 778 | 99 | 19 | +0.000160 | +0.000415 | -0.000001 | 0 |

The exact best-row label is too sparse and does not generalize.  The accepted
label is still better, but it needs query-level constraints to avoid tiny CUB
loss.

## Current Diagnosis

The route is not dead.  It has moved to a sharper bottleneck:

1. Native context is the missing interface.
2. Qrels-free global proposals preserve strong oracle opportunity.
3. The learned selector is underconstrained and either over-applies or selects
   low-utility safe attempts.

This is no longer a representation or coordinate-search problem.  It is a
query-level constrained policy learning problem.

## Next Step

M757 should train a query-listwise selector over the fixed top32/top64 global
proposal pool.

Requirements:

1. The model scores all attempts in a query jointly.
2. Selection is optimized against query-level deltas, not row-level accepted
   labels.
3. The dev threshold must require positive or zero CUB, O@100, MRR, NDCG, MAP,
   and Recall.
4. The test gate is strict; no tolerance unless explicitly reported as a
   separate numerical-tolerance diagnostic.
5. Compare against the M754 oracle to measure selector gap.

Stop if M757 cannot recover a strict-gate subset of the top32/top64 oracle.
