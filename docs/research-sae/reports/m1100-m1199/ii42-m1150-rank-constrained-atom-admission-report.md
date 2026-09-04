# ii42 M1150 Rank-Constrained Atom Admission Report

## Purpose

M1149 showed that naive batch addition of M1148 predicted atoms is not
rank-safe: Recall can rise, but CUB, MAP, NDCG, and MRR fall.  M1150 tests the
next narrower question:

> Can the same predicted atoms be useful if admitted one at a time under a
> strict native metric guard?

This is an oracle audit, not a deployable model.  It uses native metrics at
admission time, so it must not be reported as a production score.  Its job is
to test whether the atom route is structurally viable after M1149.

## Inputs

- M1148 predictions:
  `runs/m1148_boundary_atom_target_recovery_v1/target_recovery.json`
- M1150 replay:
  `runs/m1150_rank_constrained_atom_admission_v1/admission_replay.json`
- M1150 summary:
  `runs/m1150_rank_constrained_atom_admission_v1/summary.md`
- Script:
  `scripts/audit_m1150_rank_constrained_atom_admission.py`

## Method

For each M1147 event query:

1. Start from the original M1129 query.
2. Consider the top 16 M1148 predicted atoms.
3. Test scales `0.02`, `0.05`, and `0.10`.
4. Admit an atom only if all native metrics are non-regressing from the current
   state:
   - candidate upper bound;
   - Recall@100;
   - MAP@100;
   - NDCG@10;
   - MRR@20.
5. Require at least one metric to improve.
6. Stop after at most 8 admitted atoms per query.

This is intentionally stricter than M1149.  It checks whether rank-safe atom
movement exists before training an approximation.

## Result

Event-query macro:

| Source | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 0.981217 | 0.468327 | 0.350403 | 0.461003 | 0.656877 |
| admitted | 0.982532 | 0.512544 | 0.356646 | 0.467294 | 0.657869 |
| delta | +0.001315 | +0.044217 | +0.006243 | +0.006292 | +0.000992 |

Admission rate:

| Metric | Value |
| --- | ---: |
| queries | 149 |
| mean accepted atoms/query | 1.409340 |
| query accept rate | 0.565992 |

Dataset deltas:

| Dataset | Queries | Accept rate | Atoms/query | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 1 | 0.000000 | 0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `climate-fever` | 6 | 1.000000 | 1.333333 | +0.000000 | +0.375000 | +0.023833 | +0.010007 | +0.000000 |
| `cqadupstack` | 8 | 0.625000 | 1.250000 | +0.000000 | +0.000000 | +0.001100 | +0.000000 | +0.000000 |
| `dbpedia-entity` | 27 | 0.925926 | 3.074074 | +0.000000 | +0.005734 | +0.010150 | +0.011454 | +0.000000 |
| `fiqa` | 7 | 0.571429 | 1.285714 | +0.000000 | +0.083333 | +0.016051 | +0.012955 | +0.000000 |
| `msmarco` | 16 | 0.812500 | 1.875000 | +0.000000 | +0.000000 | +0.000754 | +0.002024 | +0.000000 |
| `nfcorpus` | 41 | 0.829268 | 2.902439 | +0.000000 | +0.052246 | +0.013937 | +0.020554 | +0.007271 |
| `quora` | 1 | 0.000000 | 0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `scidocs` | 18 | 0.277778 | 0.388889 | +0.000000 | +0.011111 | +0.003287 | +0.002709 | +0.004630 |
| `scifact` | 1 | 0.000000 | 0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `trec-covid` | 19 | 1.000000 | 4.052632 | +0.015779 | +0.003176 | +0.003191 | +0.015137 | +0.000000 |
| `webis-touche2020` | 4 | 0.750000 | 0.750000 | +0.000000 | +0.000000 | +0.002612 | +0.000658 | +0.000000 |

## Interpretation

M1150 is the first strong positive result after the M1149 stop signal.

The route is now more precise:

1. M1147 proved a same-surface atom teacher exists.
2. M1148 proved inference-time features can recover a portion of that teacher.
3. M1149 proved naive batch addition is unsafe.
4. M1150 proves rank-constrained admission can turn those same atoms into
   simultaneous positive movement across all tracked metrics.

The small number of accepted atoms is important.  The route does not need a
large generated bundle.  It needs a high-precision admission policy:

- mean accepted atoms/query: `1.409340`;
- only `56.6%` of event queries accept any atom;
- accepted movement is non-negative by construction and positive at macro
  level.

## Decision

Promote M1150 as an oracle-positive milestone.

Do not promote it as a deployable model.  Admission uses native metric/qrels
feedback and therefore cannot run at inference.  Its value is proving that the
M1147/M1148 atom route can be made rank-safe if admission is correct.

## Next Step

Proceed to M1151:

1. Use M1150 accepted atoms as labels.
2. Train a deployable admission proxy over the same inference-time proposal
   features used by M1148.
3. Keep leave-one-dataset-out validation.
4. First gate: recover M1150 accepted atoms with high precision.
5. Second gate: native event-query replay using predicted admissions.
6. Only if M1151 preserves M1150 direction should this expand from event
   queries to all shared15 queries.

Stop if M1151 cannot approximate accepted atoms without reintroducing the
M1149 tradeoff.  In that case the atom teacher is useful, but admission remains
the bottleneck.
