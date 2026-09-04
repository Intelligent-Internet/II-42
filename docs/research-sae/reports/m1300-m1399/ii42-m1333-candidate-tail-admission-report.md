# M1333 Candidate-Tail Admission

## Question

M1332 showed that global source-objective atom priors do not make M1330 source
choice intrinsic. The best hard-row frontier remained `rank_prefix`: high
precision but low target recall.

M1333 narrows the question:

> If `rank_prefix` is kept as the clean anchor, can candidate-self tail atoms
> outside that anchor be admitted safely?

This is an atom-level target/harm audit. It does not run native retrieval.

## Smoke Result

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

Tail label distribution:

| Label | Count |
| --- | ---: |
| `target` | 198 |
| `harm` | 642 |

Model separability:

| Metric | Value |
| --- | ---: |
| ROC-AUC | 0.4539 |
| Average precision | 0.2207 |

Tail frontier:

| Policy | TargetRecall | Precision | HarmPrecision | HarmRecall | PredCount | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_score_top2` | 0.5909 | 0.2734 | 0.7266 | 0.4844 | 1.845 | -0.8312 |
| `prior_score_top2` | 0.5808 | 0.2687 | 0.7313 | 0.4875 | 1.845 | -0.8569 |
| `candidate_rank_top2` | 0.5758 | 0.2664 | 0.7336 | 0.4891 | 1.845 | -0.8697 |
| `model_score_top1` | 0.3333 | 0.2845 | 0.7155 | 0.2586 | 1.000 | -0.9425 |
| `candidate_rank_top1` | 0.3232 | 0.2759 | 0.7241 | 0.2617 | 1.000 | -0.9800 |

Per-dataset best policies:

| Dataset | BestPolicy | TargetRecall | Precision | HarmPrecision | PredCount |
| --- | --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | `model_score_top2` | 0.4691 | 0.2011 | 0.7989 | 1.989 |
| `scidocs` | `candidate_rank_top2` | 0.8485 | 0.3944 | 0.6056 | 1.614 |
| `webis-touche2020` | `candidate_rank_top2` | 0.4510 | 0.2371 | 0.7629 | 1.980 |

## Interpretation

Candidate-tail admission is not currently separable.

The model is worse than random by ROC-AUC and only weakly above the target base
rate by average precision. Even the best top-2 policy has harm precision above
`0.72`.

This rejects the obvious follow-up to M1332:

```text
rank_prefix anchor + learned candidate-tail admission
```

The failure is useful because it localizes the problem. The high-precision
anchor is real, but the candidate-only tail is mostly harmful under the current
feature/objective view.

## Decision

Do not run native replay for `rank_prefix + admitted_tail`.

Do not deepen this tail classifier with the same feature family.

The next branch should not try another post-hoc tail admission model. The
signal must move earlier, into how candidate atoms are generated, not how they
are admitted after candidate generation.

## Next Valid Work

The next useful direction is to redesign the candidate source itself:

1. Use `rank_prefix` as the precision anchor.
2. Replace candidate-self tail with a new generated candidate source whose
   training objective penalizes M1333-style tail harms directly.
3. Validate before native replay with the same target/harm audit.
4. Only replay if tail harm precision falls materially while preserving target
   recall.

This means the remaining bottleneck is candidate-tail generation, not source
choice, not source-objective global priors, and not tail admission.

## Artifacts

- Script:
  `scripts/audit_m1333_candidate_tail_admission.py`
- Smoke JSON:
  `runs/m1333_candidate_tail_admission_smoke_v1/m1333_candidate_tail_admission.json`
- Smoke Markdown:
  `runs/m1333_candidate_tail_admission_smoke_v1/m1333_candidate_tail_admission.md`
