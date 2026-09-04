# M633 / P1.7 Representation Gap Audit

## Objective

M633 tests whether the current P1 bottleneck is malformed posting
representation rather than scorer depth. The audit compares dense-tail
positives against false-head negatives without BM25, qrels-primary training, or
dataset-specific tuning.

Input artifact:

`runs/m630_p1p4_score_geometry_audit_smoke_v1/m630_p1p4_score_geometry_dataset.jsonl`

Output artifacts:

- `runs/m633_p1p7_representation_compiler_smoke_v1/m633_representation_gap_audit.json`
- `runs/m633_p1p7_representation_compiler_smoke_v1/m633_representation_gap_audit.md`
- `runs/m633_p1p7_representation_compiler_smoke_v1/m633_deterministic_correction_config.json`

## Audit Surface

| Item | Value |
| --- | ---: |
| Contrast rows | 3420 |
| Dense-tail positives | 1710 |
| False-head negatives | 1710 |
| Datasets | nfcorpus, scifact, trec-covid |

Dataset split:

| Dataset | Dense-tail positives | False-head negatives | Rows |
| --- | ---: | ---: | ---: |
| nfcorpus | 731 | 731 | 1462 |
| scifact | 695 | 695 | 1390 |
| trec-covid | 284 | 284 | 568 |

## Representation Channel Result

| Channel | AUC | Cohen d | Positive mean | Negative mean |
| --- | ---: | ---: | ---: | ---: |
| overlap_support | 0.207743 | -1.048970 | 1.138017 | 1.231027 |
| head_artifact_penalty | 0.254122 | -0.901844 | 2.610678 | 3.012334 |
| signed_support | 0.272773 | -0.838659 | 2.301270 | 2.662161 |
| conflict_suppressed_support | 0.315319 | -0.668404 | 3.586045 | 4.062745 |

The representation signal is separable, but the direction is inverted. False
heads have stronger support/margin than dense-tail positives. A naive dense-tail
support boost is therefore the wrong shape.

## Strongest Feature Gaps

| Feature | AUC | Cohen d | Positive mean | Negative mean |
| --- | ---: | ---: | ---: | ---: |
| signed_margin_qz | 0.186308 | -1.125409 | 1.209578 | 1.391795 |
| atom_signed_dot_qz | 0.186308 | -1.125409 | 1.209578 | 1.391795 |
| atom_conflict_rate_qz | 0.712450 | +0.739109 | -1.237634 | -1.400692 |
| signed_margin_minus_top100_mean | 0.304151 | -0.648950 | -0.040635 | -0.033058 |
| atom_signed_dot_minus_top100_mean | 0.304151 | -0.648950 | -0.040635 | -0.033058 |

## Interpretation

M633-A disproves the simple hypothesis that dense-tail misses are caused by
missing positive support alone. The better hypothesis is that P1 has
over-confident false-head artifacts: a small set of representation channels
pushes non-dense-head documents too high while dense-tail documents remain
below top100.

This is representation-side evidence, not a new scorer win. It supports
compiler/basis repair as the next research direction only if the repair can
preserve dense overlap and top-rank metrics.
