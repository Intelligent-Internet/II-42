# M642 / M641 Dominance Audit Report

Status: `breakthrough_not_promotion_ready`.

## Goal

M642 audits whether M641 should move to native DB/plugin replay, or whether it
is a real but still insufficient second-stage signal.  This step does not train
a new model.  It compares the three M641 replay artifacts:

- 80k canary
- 160k validation
- 240k focused full-grid

The audit adds three checks that were missing from the first M641 report:

1. Promotion threshold: Recall@100 delta must reach `+0.005`.
2. Dominance: positive gain should not be concentrated in one dataset.
3. Regression: no major per-dataset Recall@100 regression below `-0.001`.

## Inputs

| Run | Replay JSON |
| --- | --- |
| Canary | `runs/m641_p1p3_nonlinear_native_boundary_ranker_canary_v1/m641_p1p3_nonlinear_native_boundary_replay.json` |
| Validation | `runs/m641_p1p3_nonlinear_native_boundary_ranker_validation_v1/m641_p1p3_nonlinear_native_boundary_replay.json` |
| Full-grid | `runs/m641_p1p3_nonlinear_native_boundary_ranker_fullgrid_v1/m641_p1p3_nonlinear_native_boundary_replay.json` |

## Audit Results

| Run | Status | alpha | Preserve | Window | dRecall | dMAP | Net top100 | Max positive share | Major regressions |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Canary | `capacity_breakthrough_not_promotion_ready` | 0.100 | 95 | 300 | +0.004131 | +0.000184 | +9 | 0.816 | none |
| Validation | `capacity_breakthrough_not_promotion_ready` | 0.100 | 95 | 300 | +0.004468 | +0.000131 | +8 | 0.784 | msmarco |
| Full-grid | `capacity_breakthrough_not_promotion_ready` | 0.150 | 95 | 300 | +0.004193 | +0.000089 | +5 | 0.790 | cqadupstack, msmarco |

Best run by audit sort: `validation`.

Validation details:

| Metric | Value |
| --- | ---: |
| dRecall@100 | +0.004468 |
| dMAP@100 | +0.000131 |
| dNDCG@10 | +0.000000 |
| dMRR@20 | +0.000000 |
| Net top100 positives | +8 |
| Under-ranked positives promoted | 57 |
| Top100 positives displaced | 49 |
| Positive gain efficiency | 0.140 |
| Max positive dataset | fiqa |
| Max positive dataset share | 0.784 |

## Diagnosis

M641 is a genuine improvement over M629.  The nonlinear scorer repeatedly beats
M629-B's `+0.001923` Recall@100 reference while keeping MAP positive and
NDCG/MRR unchanged.  This means the second-stage native-boundary route is not
dead, and M629 was partly capacity-limited.

But M641 is not promotion-ready:

- none of the runs reaches the `+0.005` Recall@100 promotion threshold
- the best run's gain is dominated by FiQA (`78.4%` of positive dataset recall
  delta mass)
- validation has a major MSMARCO regression
- full-grid scaling does not improve the result and adds a cqadupstack
  regression
- net top100 improvement remains tiny, despite more promoted positives,
  because displacement is still high

The full-grid result is especially important: simply increasing train rows,
trees, or alpha neighborhood does not unlock the next step.  It shifts the
tradeoff toward more promotions and more displacements, rather than improving
promotion efficiency.

## Blocker

The current scorer objective can identify some under-ranked positives, but it
does not know which existing top100 positives are safe to displace.  This is the
same shape as the M640 first-stage blocker, now seen in the second-stage
feature model:

- first stage can cross the top100 boundary but disturbs local order
- second stage can promote more positives but displaces too many positives

The missing signal is not another alpha sweep.  It is a dominance-aware,
displacement-aware training objective.

## Decision

Do not promote M641 to P1 default.

Do not run native DB/plugin replay yet.  Native DB replay is expensive and M642
shows the current scorer has not passed the offline promotion/dominance gate.

Keep M641 as the best second-stage scorer direction so far and use it as the
next training target.

## Next Step

The next stage should be M643: dominance-aware nonlinear boundary training.

M643 should keep the M641 architecture but change the objective/data weighting:

1. Penalize displacement of known top100 positives more strongly.
2. Add dataset-balanced sampling or dataset-balanced sample weights.
3. Add a hard gate that rejects runs with major per-dataset regressions.
4. Optimize for net top100 positives, not just under-ranked positive
   promotions.
5. Keep the same audit surface: M629 native rows, no ids/dense features,
   no dataset-specific thresholds, no candidate generator changes.

Acceptance for M643:

- dRecall@100 >= `+0.005`
- dMAP@100 >= `0`
- dNDCG@10 and dMRR@20 non-negative or within existing tolerance
- no major dataset Recall@100 regression below `-0.001`
- max positive dataset share <= `0.75`

If M643 cannot pass this, the second-stage scorer family should stop expanding
and M641's recovered/displaced examples should be used to redesign the
first-stage generated-posting objective.

## Verification

Completed:

- `python3 -m py_compile scripts/audit_m642_m641_dominance.py scripts/train_m641_nonlinear_native_boundary_ranker.py`
- `pytest -q tests/test_audit_m642_m641_dominance.py tests/test_train_m641_nonlinear_native_boundary_ranker.py`
- `python3 scripts/audit_m642_m641_dominance.py`
