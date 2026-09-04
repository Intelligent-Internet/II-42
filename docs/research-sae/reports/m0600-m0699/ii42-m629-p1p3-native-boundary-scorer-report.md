# M629 / P1.3 Native-Boundary Scorer Report

Status: `guarded_not_promoted`.

M629 tested whether a larger native top100-boundary training surface could
recover P1.3-a010 candidate-present under-ranked positives better than M628,
without changing the P1.3 encoder/posting generator.

## Scope

- Frozen first stage: `P1.3-a010`.
- Training surface: full M604 native candidate rows.
- No dataset-specific thresholds, alpha, branches, query ids, doc ids, dataset
  ids, dense rank, or dense hit flags were used as model features.
- BM25 was used only as native scorer features.
- Acceptance required native replay, not selected offline replay.

## Artifacts

| Artifact | Path |
| --- | --- |
| Dataset JSONL | `runs/m629_p1p3_native_boundary_dataset_v1/m629_p1p3_native_boundary_dataset.jsonl` |
| Dataset summary | `runs/m629_p1p3_native_boundary_dataset_v1/m629_p1p3_native_boundary_dataset_summary.json` |
| Dataset audit | `runs/m629_p1p3_native_boundary_dataset_v1/m629_p1p3_native_boundary_dataset_summary.md` |
| M629-A model | `runs/m629_p1p3_native_boundary_ranker_v1/m629_p1p3_native_boundary_ranker_model.json` |
| M629-A native replay | `runs/m629_p1p3_native_replay_shared15_v1/m629_p1p3_native_replay.json` |
| M629-B model | `runs/m629_p1p3_native_boundary_ranker_cross_v1/m629_p1p3_native_boundary_ranker_model.json` |
| M629-B native replay | `runs/m629_p1p3_native_replay_cross_shared15_v1/m629_p1p3_native_replay_cross.json` |
| M629-C model | `runs/m629_p1p3_native_boundary_listwise_ranker_v1/m629_p1p3_native_boundary_listwise_model.json` |
| M629-C native replay | `runs/m629_p1p3_native_replay_listwise_shared15_v1/m629_p1p3_native_replay_listwise.json` |
| Gap delta JSON | `runs/m629_p1p3_gap_delta_shared15_v1/m629_p1p3_gap_delta_shared15.json` |
| Gap delta MD | `runs/m629_p1p3_gap_delta_shared15_v1/m629_p1p3_gap_delta_shared15.md` |
| M603-style eval rows | `runs/m629_p1p3_native_replay_cross_eval_style_shared15_v1/` |

## Dataset

M629 native-boundary dataset:

| Metric | Value |
| --- | ---: |
| Rows scanned | 1,946,411 |
| Rows selected | 595,547 |
| Train queries | 1,040 |
| Eval queries | 302 |
| Positive rows | 39,742 |
| Candidate-present positives | 27,888 |
| Candidate-miss positives | 6,716 |
| Top100 negatives | 120,582 |
| Boundary 101-300 negatives | 260,422 |
| Sampled tail 301-1000 negatives | 186,655 |

## Native Results

All rows below use the same `m629-p1p3-v1` eval split.  The baseline is
`P1.3-a010`.

| Model | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Net top100 positives | Promoted | Displaced |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `M628 same split` | +0.000000 | -0.006168 | -0.016613 | +0.000000 | -77 | 45 | 122 |
| `M629-A pairwise base` | +0.000000 | +0.000057 | +0.001374 | +0.000000 | +7 | 26 | 19 |
| `M629-B pairwise cross` | +0.000000 | +0.000078 | +0.001923 | +0.000000 | +7 | 30 | 23 |
| `M629-C listwise cross` | +0.000000 | +0.000055 | +0.001769 | +0.000000 | +7 | 23 | 16 |

Best M629 model: `M629-B pairwise cross`.

Best native macro:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Candidate UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1.3-a010` | 0.740811 | 0.676841 | 0.867565 | 0.838500 | 0.978581 |
| `P1.3-a010+M629-B` | 0.740811 | 0.676919 | 0.869488 | 0.838500 | 0.978581 |

## Interpretation

M629 confirms that native-boundary training is better aligned than M628 on the
same split: M628 replay is unstable and loses Recall@100 on the M629 split,
while all M629 variants preserve NDCG@10/MRR@20 and improve Recall@100.

However, the gain is too small for promotion.  The best model recovers only
30 under-ranked positives and displaces 23 existing top100 positives, for a net
gain of 7 top100 positives across shared15 eval.  This is below the target
`+0.005` Recall@100 acceptance threshold and is not enough to justify replacing
the frozen P1.3-a010 scorer path.

The important negative result is that larger native-boundary data and stronger
auditable features do not unlock a large second-stage scorer recovery.  The
remaining gap is unlikely to be solved by more small global scorer tweaks in
this feature family.

## Decision

Do not promote M629 as the P1.3 default scorer.

Keep `M629-B pairwise cross` as a guarded diagnostic artifact only.  It is the
best M629 variant and can be used to inspect recovered examples, but it does
not meet the acceptance gate.

Stop this second-stage scorer micro-tuning line unless a materially different
training signal is introduced.  The next useful bottleneck is likely first-stage
P1.3 score geometry / query-local dense calibration, or a more integrated
native scorer objective with substantially richer supervision.

## Verification

Completed during this run:

- `python3 -m py_compile scripts/build_m629_native_boundary_dataset.py scripts/train_m629_native_boundary_ranker.py scripts/apply_m629_native_boundary_ranker.py`
- `python3 -m py_compile scripts/train_m629_native_boundary_ranker.py scripts/train_m629_native_boundary_listwise_ranker.py scripts/apply_m629_native_boundary_ranker.py`
- `pytest -q tests/test_build_m629_native_boundary_dataset.py tests/test_train_m629_native_boundary_ranker.py tests/test_apply_m629_native_boundary_ranker.py`

Final workspace-wide verification is still required before closing the goal:

- run py_compile over all modified Python scripts, including M628 scripts;
- run all new/modified M628/M629 tests;
- run `git diff --check`.
