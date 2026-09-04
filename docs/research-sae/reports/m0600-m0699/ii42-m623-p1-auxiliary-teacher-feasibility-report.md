# II-42 M623 P1 Auxiliary Teacher Feasibility Report

Date: 2026-07-06

## Objective

M623 asks whether the next first-stage teacher/objective can use qrels-free
signals beyond aligned dense top100.  It does not train a model.

Qrels labels are used only for evaluation of candidate teachers.  Candidate
teachers themselves use only existing global signals:

- aligned dense top100,
- P1 rank,
- BM25 rank as a reference signal,
- lexical token overlap from query/document text,
- intersections of the above.

The goal is to decide whether a new first-stage objective is worth training,
and if so whether it should use hard pseudo-labels or low-weight soft
auxiliary targets.

## Inputs

- Gap root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root:
  `runs/m608_p1p3_aligned_dense_rankings_shared15_v1`
- Atom/text root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- Output JSON:
  `runs/m623_p1p3_auxiliary_teacher_feasibility_v1/m623_auxiliary_teacher_feasibility.json`
- Output Markdown:
  `runs/m623_p1p3_auxiliary_teacher_feasibility_v1/m623_auxiliary_teacher_feasibility.md`
- Script:
  `scripts/audit_m623_auxiliary_teacher_feasibility.py`

## Method

For each candidate teacher, M623 measures:

- coverage of candidate-present under-ranked dense-miss positives,
- selected precision,
- outside-top100 precision,
- candidate-positive recall.

The key target is under-ranked dense-miss coverage because M615 showed these
examples dominate the remaining gap.  Precision matters because a teacher with
high coverage and high noise should not be used as a hard target.

## Macro Result

Top candidates by under-ranked dense-miss coverage:

| Teacher | Coverage | Selected precision | Outside-top100 precision | Candidate-positive recall |
| --- | ---: | ---: | ---: | ---: |
| `p1_top500` | 0.602780 | 0.059230 | 0.037137 | 0.938259 |
| `lexical_coverage_top500` | 0.356836 | 0.064419 | 0.030133 | 0.837429 |
| `p1_x_lexcov_top500` | 0.334604 | 0.115996 | 0.066163 | 0.826265 |
| `lexical_jaccard_top500` | 0.334210 | 0.062662 | 0.028643 | 0.831689 |
| `bm25_top500` | 0.311482 | 0.047241 | 0.025531 | 0.857026 |
| `p1_x_bm25_x_lexcov_top500` | 0.247435 | 0.123633 | 0.071133 | 0.816611 |

Top candidates by outside-top100 precision:

| Teacher | Coverage | Selected precision | Outside-top100 precision |
| --- | ---: | ---: | ---: |
| `p1_x_lexcov_top100` | 0.000026 | 0.231567 | 0.105185 |
| `p1_x_bm25_x_lexcov_top200` | 0.102274 | 0.185698 | 0.099795 |
| `bm25_x_p1_top200` | 0.110448 | 0.174011 | 0.095158 |
| `p1_x_lexcov_top200` | 0.116522 | 0.170634 | 0.092103 |
| `p1_x_bm25_x_lexcov_top500` | 0.247435 | 0.123633 | 0.071133 |
| `p1_x_lexcov_top500` | 0.334604 | 0.115996 | 0.066163 |

## Interpretation

Single-signal teachers are too noisy:

- `p1_top500` covers many dense-miss positives but outside-top100 precision is
  only `0.037137`.
- `lexical_coverage_top500` has meaningful coverage but precision is only
  `0.030133`.
- `bm25_top500` is also noisy and should not become a hard first-stage target.

Consensus teachers are more useful:

- `p1_x_lexcov_top500` keeps one third of the dense-miss coverage while nearly
  doubling outside-top100 precision versus `p1_top500`.
- `p1_x_bm25_x_lexcov_top500` has lower coverage but the best balanced
  precision among the high-coverage candidates.
- Smaller top100/top200 intersections improve precision but lose too much
  coverage to explain the remaining bottleneck alone.

The signal is real but not clean enough for hard pseudo-label training.  It is
appropriate as a soft auxiliary target or curriculum prior with dense overlap
floors.

## Decision

Do not launch a full training run directly from these labels.

Do not use BM25 as the promoted first-stage inference scorer.

The next step should be M624, a bounded proxy before training:

1. Use `p1_x_lexcov_top500` and `p1_x_bm25_x_lexcov_top500` as soft auxiliary
   teacher candidates.
2. Run a native-replay admission/proxy test to check whether these teacher
   sets can improve Recall@100/MAP@100 without hurting NDCG/MRR.
3. If the proxy is positive, add a low-weight soft auxiliary loss to the
   posting compiler while preserving dense O@100/support floors.
4. If the proxy is near zero like M619/M621, stop this auxiliary-teacher route
   and move to a different teacher source.

## Proposed M624 Gate

M624 should pass all of:

- no NDCG@10 regression,
- no MRR@20 regression,
- Recall@100 and MAP@100 improve more than the M619/M621 near-zero band,
- improvements are not isolated to one dataset,
- dense overlap/support floor is preserved before any native benchmark.

Minimum useful replay threshold:

- Recall@100 delta should be greater than `+0.001`,
- MAP@100 delta should be positive,
- `cqadupstack` and `trec-covid` should not both regress.

## Verification

Local verification passed for the M623 audit script:

- `python3 -m py_compile scripts/audit_m623_auxiliary_teacher_feasibility.py`
- `pytest -q tests/test_audit_m623_auxiliary_teacher_feasibility.py`
