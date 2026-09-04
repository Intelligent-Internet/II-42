# II-42 M624 P1 Consensus-Teacher Admission Report

Date: 2026-07-06

## Objective

M624 tests whether the M623 consensus teachers can produce enough replay gain
to justify an auxiliary-teacher training run.

This is a proxy only:

- no model is trained,
- no P1 postings are changed,
- qrels are used only for evaluation,
- teacher sets are built from qrels-free signals.

## Inputs

- Gap root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Atom/text root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- Output JSON:
  `runs/m624_p1p3_consensus_teacher_admission_v1/m624_consensus_teacher_admission.json`
- Output Markdown:
  `runs/m624_p1p3_consensus_teacher_admission_v1/m624_consensus_teacher_admission.md`
- Script:
  `scripts/probe_m624_consensus_teacher_admission.py`

## Probe Shape

M624 preserves the top of the current P1.3-a010 ranking and admits only
bottom-slot documents that are selected by a consensus teacher.

Teachers tested:

- `p1_x_lexcov`
- `p1_x_bm25_x_lexcov`

Grid:

- preserve top-k: `95, 98, 99`
- max admission rank: `200, 500`
- teacher-k: `200, 500`

Acceptance gate:

- Recall@100 delta greater than `+0.001`,
- MAP@100 positive,
- no NDCG@10 regression,
- no MRR@20 regression.

The `+0.001` Recall gate is intentional: M619 and M621 already showed that
sub-0.001 replay gains are not worth native benchmark execution or training.

## Result

No config passed the gate.

Best by Recall:

| Config | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `top99_max500_p1_x_lexcov_k500` | +0.000000 | +0.000042 | +0.000424 | +0.000000 |
| `top99_max500_p1_x_bm25_x_lexcov_k500` | +0.000000 | +0.000042 | +0.000424 | +0.000000 |
| `top95_max500_p1_x_bm25_x_lexcov_k500` | +0.000000 | +0.000024 | +0.000118 | +0.000000 |
| `top95_max500_p1_x_lexcov_k500` | +0.000000 | +0.000021 | +0.000114 | +0.000000 |
| `top98_max500_p1_x_lexcov_k500` | +0.000000 | +0.000040 | +0.000037 | +0.000000 |

The strongest M624 signal remains in the same near-zero band as M619/M621:

| Line | Best dRecall@100 | Best dMAP@100 |
| --- | ---: | ---: |
| M619 model admission | +0.000387 | +0.000055 |
| M621 atom-conflict admission | +0.000368 | +0.000026 |
| M624 consensus teacher admission | +0.000424 | +0.000042 |

## Interpretation

M623 showed that consensus teachers are less noisy than single-signal
teachers, but M624 shows they still do not convert into meaningful ranking
gains.

The issue is not simply "we need a softer teacher".  If a qrels-free teacher
set contains enough useful positive evidence, a conservative admission proxy
should recover more than `+0.0004` Recall@100.  It does not.

Therefore this auxiliary-teacher family is not strong enough to justify a GPU
training run.

## Decision

Do not train M624-derived auxiliary teacher loss.

Do not run native DB/plugin benchmark for M624.

Stop the weak auxiliary-teacher route based on:

- P1 rank,
- BM25 rank,
- simple lexical overlap,
- intersections of those signals.

The next route needs a stronger teacher source, not another proxy grid:

1. Use a stronger semantic teacher than aligned dense top100, such as cross
   encoder / LLM relevance / generated query-document pseudo-pairs.
2. Or change the first-stage product requirement: keep P1.3-a010 as the
   current native candidate and move to engineering benchmark/reporting.
3. Do not continue fixed-alpha, bottom-slot admission, or weak lexical
   consensus tuning.

## Verification

Local verification passed:

- `python3 -m py_compile scripts/probe_m624_consensus_teacher_admission.py`
- `pytest -q tests/test_probe_m624_consensus_teacher_admission.py`
