# M1289 Pseudo-Witness Student Native Replay

M1289 tests whether the M1288 pair/witness source can be approximated by a
deployable query-time student.

M1288 itself is strong but teacher-side: it uses dense-boundary positive and
negative witness documents.  M1289 keeps M1288 as the teacher and trains a
student that can only use:

- raw candidate atom features;
- native baseline head/boundary/tail document atom context;
- optionally, global atom priors learned from train split teacher selections.

The goal is to decide whether M1288 can be turned into a query-time policy
without carrying oracle witness documents into inference.

## Runs

### Native-context student

- Output:
  `runs/m1289_pseudo_witness_student_limit25_v1/m1289_native.json`
- Surface: `arguana,cqadupstack,fiqa,scidocs`
- Limit: 25 queries per dataset
- Teacher: M1288 `risk=0.5/1`, `budget=192`
- Student groups: `pseudo`, `raw_pseudo`

| Student | Rows | Features | PositiveShare | TrainAUC |
| --- | ---: | ---: | ---: | ---: |
| `pseudo:teacher_risk0.5_b192` | 92160 | 36 | 0.125000 | 0.718350 |
| `raw_pseudo:teacher_risk0.5_b192` | 92160 | 60 | 0.125000 | 0.719505 |
| `pseudo:teacher_risk1_b192` | 92160 | 36 | 0.125000 | 0.709510 |
| `raw_pseudo:teacher_risk1_b192` | 92160 | 60 | 0.125000 | 0.708730 |

Best eval native replay:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | TeacherOverlap | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pseudo_teacher_risk0.5_b192_b192_s0.05` | 0.516667 | 0.520000 | 6 | 7 | 0.948684 | 0.330729 | 0.544705 | 0.086198 |

### Native-context plus global atom prior

- Output:
  `runs/m1289_pseudo_witness_student_prior_limit25_v1/m1289_native.json`
- Student groups: `prior`, `raw_prior`, `pseudo_prior`, `raw_pseudo_prior`

| Student | Rows | Features | PositiveShare | TrainAUC |
| --- | ---: | ---: | ---: | ---: |
| `prior:teacher_risk0.5_b192` | 92160 | 5 | 0.125000 | 0.838631 |
| `raw_prior:teacher_risk0.5_b192` | 92160 | 29 | 0.125000 | 0.873775 |
| `pseudo_prior:teacher_risk0.5_b192` | 92160 | 41 | 0.125000 | 0.879989 |
| `raw_pseudo_prior:teacher_risk0.5_b192` | 92160 | 65 | 0.125000 | 0.880681 |
| `prior:teacher_risk1_b192` | 92160 | 5 | 0.125000 | 0.843468 |
| `raw_prior:teacher_risk1_b192` | 92160 | 29 | 0.125000 | 0.877462 |
| `pseudo_prior:teacher_risk1_b192` | 92160 | 41 | 0.125000 | 0.882271 |
| `raw_pseudo_prior:teacher_risk1_b192` | 92160 | 65 | 0.125000 | 0.883692 |

Best eval native replay:

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | TeacherOverlap | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `raw_pseudo_prior_teacher_risk0.5_b192_b192_s0.05` | 0.516667 | 0.520000 | 5 | 6 | 0.960000 | 0.569661 | 0.445783 | 0.066927 |

## Interpretation

M1289 fails the deployable approximation gate.

The important detail is that adding global atom priors materially improves
teacher-selection fit:

- train AUC rises from about `0.72` to about `0.88`;
- eval teacher-overlap rises from about `0.33` to about `0.57`.

But native replay still does not improve pair success.  The best variants stay
below baseline and still introduce pair regressions.  Target recall is far
below M1288, while negative-only selection remains too high.

This means the deployable student is learning a broad approximation of the
teacher selected set, but not the rank-effective, low-harm subset that made
M1288 work.  The missing signal is therefore not just local native context or
global atom identity prior.

## Decision

Stop this M1289 approximation branch.

Do not scale it to full four-dataset or shared15.  Do not deepen this exact
student with the same feature families.  It would be another selector loop over
a feature view that already failed native replay.

Keep the result as a useful diagnostic:

- M1288 remains a strong teacher/source signal.
- A deployable approximation needs a different supervision interface, not just
  better thresholding or more trees.
- The next branch should target the rank-effective subset directly, for
  example by training on native replay outcomes or by constructing a source
  whose candidates are generated from rank movement constraints rather than
  generic teacher-selected atoms.
