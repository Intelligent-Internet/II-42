# M761 Task-Robust Policy Audit

## Decision

M758 is a real macro-safe signal, but it is not yet task-robust enough for
promotion to a broader/native default.

When the M758 policies are broken down by dataset, every proposal budget has a
few negative rows. A stricter task-robust search can eliminate negative tasks,
but the recovered gain becomes very small. The next bottleneck is therefore
not proposal generation and not global macro policy selection; it is
query/task risk isolation.

## M760 Per-Dataset Findings

| Budget | Macro Result | Negative Tasks |
| --- | --- | --- |
| top16 | dMAP +0.000246, dNDCG +0.000332, dCUB +0.000032, dO@100 +0.000000 | fiqa, scidocs |
| top32 | dMAP +0.000057, dNDCG +0.000684, dCUB +0.000012, dO@100 +0.000000 | cqadupstack, dbpedia-entity, nfcorpus, scidocs |
| top64 | dMAP +0.000177, dNDCG +0.000213, dCUB +0.000013, dO@100 +0.000000 | climate-fever, trec-covid, webis-touche2020 |

The negative rows do not spend dense overlap or candidate upper bound. They are
rank-geometry regressions, mostly MAP/NDCG/MRR at a small number of rows.

## Task-Robust Search Probe

A stricter probe required:

- dev global gate pass;
- no dev dataset-level negative gate;
- test replay with no negative dataset rows.

The probe found safe policies, but the useful effect collapsed:

| Budget | Best no-negative test policy | Applied | dMAP | dNDCG | dCUB | dO@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| top16 | `margin_bundle`, O128=1.0, O256=1.0 | 1 | +0.000000 | +0.000078 | +0.000000 | +0.000000 |
| top32 | `score_mean_margin`, md100=0.0001 | 27 | +0.000005 | +0.000000 | +0.000012 | +0.000000 |

This is important: a non-negative deterministic policy exists, but a global
all-task floor makes it too conservative to capture most of the M758 macro
gain.

## Interpretation

The route has progressed through three clear stages:

1. M754: qrels-free global proposals have a large oracle ceiling.
2. M758: deterministic native-context policy extracts a small safe macro gain.
3. M760/M761: that macro gain is not uniformly safe across tasks.

The current failure mode is not a model-capacity failure. It is policy
calibration under heterogeneous task risk. Different datasets prefer different
proposal budgets and context scores:

- top16 is strongest on MAP/CUB balance but hurts fiqa/scidocs;
- top32 is strongest on NDCG/MRR but has more small MAP regressions;
- top64 has broader coverage but different negative rows.

## Next Step

M762 should isolate negative-row risk without using dataset-specific tuning.

Concrete plan:

1. Build a risk-feature audit over the M760 negative rows.
2. Compare negative rows against neutral/positive rows using only
   inference-available features:
   - context overlap/margin at 10/20/50/100/128/256;
   - score mean/std deltas;
   - proposal rank/counts from dev;
   - selected budget family;
   - baseline native score margins.
3. Add a global abstention guard, not a learned reranker:
   - keep the M758 policy score;
   - skip rows matching high-risk feature bands;
   - require no negative task rows on dev;
   - replay unchanged on test.
4. Accept only if it preserves most of the top16/top32 macro gain while
   reducing negative rows.

## Stop Condition

If a global abstention guard cannot reduce negative task rows without reducing
M758 gains to near-zero, then this line should not be promoted as a general
native policy. It can remain a diagnostic signal, and the next route should
move from global deterministic policy to query-local native feedback or a
separate calibrated risk estimator.
