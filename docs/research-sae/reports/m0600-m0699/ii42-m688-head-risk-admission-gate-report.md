# M688 Head-Risk Admission Gate

## Status

M688 is complete and rejected as a promotion candidate.

It reuses the M687 counterfactual rows and exposes proposed top95 overlap as a
qrels-free inference feature. The goal is narrow: test whether M687 failed
because the gate lacked explicit head-risk information, without changing the
M686 atom-delta compiler.

The answer is mostly no. Head-risk features make the gate much safer, but they
do not recover holdout Recall.

## Setup

- Input summary:
  `runs/m687_metric_aware_atom_gate_v1/m687_summary.json`
- Input counterfactual rows:
  `runs/m687_metric_aware_atom_gate_v1/m687_counterfactual_rows.jsonl`
- Output summary:
  `runs/m688_head_risk_admission_gate_v1/m688_summary.json`
- Output report:
  `runs/m688_head_risk_admission_gate_v1/m688_report.md`
- Surface: M687 native shared15 rows, replayed artifact-only.
- Split: unchanged M687 train/holdout split.
- Compiler: unchanged M686 `s002_a8` atom-delta shape.

M688 compares:

- M687 metric gate;
- M688 rule gate: M687 score plus proposed top95 floor;
- M688 logistic head-risk gate;
- M688 small GBDT head-risk gate;
- metric oracle from M687 labels.

## Main Result

Holdout delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.003747 | +0.000190 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 1.000000 |
| atom-delta all | +0.000956 | -0.000360 | -0.000336 | -0.000137 | -0.000079 | 0.993356 | 1.000000 |
| M687 metric gate | +0.000000 | -0.000063 | -0.000031 | -0.000137 | -0.000007 | 0.996678 | 0.561475 |
| M688 rule top95 gate | +0.000000 | -0.000063 | -0.000031 | -0.000137 | -0.000007 | 0.997325 | 0.545082 |
| M688 logistic head-risk | +0.000000 | -0.000008 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 0.229508 |
| M688 GBDT head-risk | +0.000000 | -0.000050 | -0.000028 | +0.000000 | +0.000000 | 1.000000 | 0.327869 |
| metric oracle | +0.000820 | +0.000075 | +0.000030 | +0.000000 | +0.000000 | 1.000000 | 0.393443 |

Full shared15 delta vs baseline:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 1.000000 |
| M687 metric gate | +0.000008 | +0.000002 | +0.000048 | +0.000008 | +0.000011 | 0.996666 | 0.593145 |
| M688 rule top95 gate | +0.000008 | +0.000003 | +0.000048 | +0.000008 | +0.000011 | 0.997333 | 0.570045 |
| M688 logistic head-risk | +0.000008 | +0.000016 | +0.000035 | +0.000032 | +0.000000 | 1.000000 | 0.271237 |
| M688 GBDT head-risk | +0.000009 | +0.000035 | +0.000033 | +0.000037 | +0.000002 | 1.000000 | 0.345753 |
| metric oracle | +0.000200 | +0.000078 | +0.000050 | +0.000037 | +0.000024 | 1.000000 | 0.422504 |

## Gate Diagnostics

The head-risk models are not weak classifiers:

| Gate | Holdout AUC | Precision | Recall | Predicted positives |
| --- | ---: | ---: | ---: | ---: |
| logistic head-risk | 0.976914 | 0.964286 | 0.562500 | 56 |
| GBDT head-risk | 0.969383 | 0.912500 | 0.760417 | 80 |

This is the key diagnostic. M688 can predict the M687 metric-safe labels much
better than M687, but the accepted safe rows still do not produce holdout
Recall lift. The gate mostly learns to avoid damage, not to select
recall-bearing deltas.

## Interpretation

M688 answers the question left by M687:

> Was M687 failing mainly because the gate did not see head/top95 risk?

No. Adding proposed top95 overlap solves much of the head-risk problem, but the
safe rows selected by a high-precision gate do not carry enough top100
promotion signal on holdout.

This refines the route:

- Dense-root unified posting remains the correct engineering form.
- Traditional SAE reconstruction is still not the path.
- Arbitrary dense-mimic/output-head tuning remains exhausted.
- Admission threshold tuning is now also a weak path.
- The next useful work is to improve recall-bearing atom visibility and the
  retrieval teacher, not to make another gate over the same proposal pool.

## Decision

Reject M688 as a promotion candidate.

Keep the artifact as a useful stop signal. It proves that the current M686/M687
proposal surface has a learnable safety boundary, but that boundary is not the
same as a useful recall-recovery boundary.

## Next Step

Proceed to M689 only if it changes the proposal/teacher surface.

Recommended M689:

1. Audit the M687 oracle-positive rows that actually increase Recall.
2. Measure whether their target atoms were visible in the current M686
   proposal pool.
3. Split the oracle positives into:
   - visible and selected by M688;
   - visible but not selected;
   - not visible to the atom proposal pool.
4. If most Recall-bearing positives are not visible, expand proposal sources
   using retrieval teacher evidence instead of tuning admission.
5. If visible but not selected, train a recall-bearing atom objective separate
   from the safety label.

The next breakthrough target is not "safer gate"; it is "safe plus
recall-bearing generated atoms."
