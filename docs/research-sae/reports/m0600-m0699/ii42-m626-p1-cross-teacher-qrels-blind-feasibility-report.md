# M626 / P1 Cross-Teacher Qrels-Blind Feasibility

Status: completed.  No training launched.

## Objective

M626 checks whether a stronger cross-encoder teacher can provide a useful
qrels-blind first-stage training target for P1 posting/score-surface recovery.
This is not a scorer promotion, not a top100 admission policy, and not a native
benchmark.  Qrels are used only after scoring to audit whether the teacher
signal has enough precision and coverage to justify M627 training.

## Inputs

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root: `runs/m608_p1p3_aligned_dense_rankings_shared15_v1`
- Atom/text root: `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- Model: `cross-encoder/ms-marco-MiniLM-L6-v2`
- Sampling: qrels-blind `stride`

## Runs

| Run | Status | Datasets | Scored pairs | Notes |
| --- | --- | ---: | ---: | --- |
| `runs/m626_p1p3_cross_teacher_qrels_blind_feasibility_v1` | invalid smoke | 15 | 12000 | Global pair cap filled only the first datasets, so it is not a balanced conclusion. |
| `runs/m626_p1p3_cross_teacher_qrels_blind_feasibility_v2` | valid audit | 15 | 29996 | Balanced per-dataset cap, 4 stride queries per dataset. |

## V2 Best Teacher Candidates

| Teacher | Outside-top100 precision | Under-ranked dense-miss coverage | Selected precision | Outside positives | Selected |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cross_top25_x_bm25_top100` | 0.118519 | 0.006256 | 0.295833 | 8 | 981 |
| `cross_top50_x_lexcov_top100` | 0.108086 | 0.011531 | 0.210609 | 21 | 1913 |
| `cross_top50_x_bm25_top100` | 0.096561 | 0.010278 | 0.234897 | 16 | 1607 |
| `cross_top50_x_bm25_top200` | 0.091650 | 0.013900 | 0.221227 | 20 | 1959 |
| `cross_top50_x_p1_x_lexcov_top200` | 0.072154 | 0.014701 | 0.204294 | 29 | 2036 |
| `cross_top50_x_p1_top200` | 0.066260 | 0.015001 | 0.194425 | 32 | 2394 |

## Interpretation

The cross-encoder teacher is not train-ready for the first-stage
posting/compiler objective.

The best rows show non-trivial precision only after intersecting cross scores
with BM25 or lexical constraints, but their coverage of the dominant
under-ranked dense-miss positives is too small.  The strongest coverage among
the top candidates is about `0.015`, which is far below what is needed for a
general teacher objective.  Positive hits are also concentrated in a few rows,
especially `trec-covid`, rather than distributed across the surface.

This matches the M625 result: qrels-informative sampling can produce apparent
gains, but qrels-blind sampling does not provide a stable teacher signal.

## Decision

Do not start M627 training from the current cross-encoder teacher signal.

Stop the direct cross-teacher/admission route for now.  Keep the code as an
audit tool, because it is useful for checking future teacher sources, but do
not promote it into the P1 first-stage objective.

## Next Step

Return to the core P1 objective with the current frozen baseline:

- Keep `P1.3-a010` as the best frozen candidate.
- Keep M605 and M626 as regression/audit tools only.
- Do not use cross-encoder labels as first-stage supervision unless a future
  qrels-blind audit shows both materially higher coverage and broader
  per-dataset support.
- The next valid M627 candidate needs a different first-stage teacher or loss
  family, not another admission/reranker tweak.
