# II-42 M620 P1.3 Atom-Interaction Separability Report

Date: 2026-07-06

## Objective

M620 checks whether richer P1 atom/posting interaction features can distinguish
candidate-present under-ranked positives from current top100 blockers.  This is
diagnostic only: no scorer is trained and no baseline is changed.

This follows M615-M619:

- M615 showed most remaining under-ranked positives are dense-miss.
- M617 showed a global pairwise linear scorer collapses or produces tiny gains.
- M619 showed conservative bottom-slot admission is safe but too small.

The question for M620 is whether atom interaction structure adds new signal
that scalar P1/BM25/fused scores do not contain.

## Inputs

- Gap root:
  `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- Output JSON:
  `runs/m620_p1p3_atom_interaction_separability_v1/m620_atom_interaction_separability.json`
- Output Markdown:
  `runs/m620_p1p3_atom_interaction_separability_v1/m620_atom_interaction_separability.md`

## Method

For each query, M620 samples:

- candidate-present positives ranked below top100 by current fused scorer,
- negative blockers currently inside top100.

It then computes native atom interaction features such as signed dot,
overlap count, coverage, and sign-conflict rate.  The primary diagnostic metric
is query-local AUC, because a reranker must compare candidates within the same
query.

## Result

15 datasets were scanned.  No atom rows were missing for sampled examples.

Top macro query-pair AUC values:

| Feature | Global AUC | Query-pair AUC | Query-macro AUC |
| --- | ---: | ---: | ---: |
| `atom_sign_conflict_rate` | 0.544491 | 0.868939 | 0.855406 |
| `atom_max_abs_product` | 0.520802 | 0.452664 | 0.439162 |
| `bm25_zscore` | 0.478388 | 0.431074 | 0.445276 |
| `atom_top8_abs_dot` | 0.513522 | 0.427836 | 0.425428 |
| `atom_overlap_count` | 0.515868 | 0.412681 | 0.424852 |
| `atom_dot` | 0.447081 | 0.015156 | 0.018952 |
| `p1_score` | 0.445657 | 0.015156 | 0.018952 |
| `fused_zscore` | 0.151245 | 0.000000 | 0.000000 |

The only strong query-local separability signal is
`atom_sign_conflict_rate`.  Most conventional interaction features are weak or
inverted.

## Interpretation

M620 says there is one real new signal, but it is narrow:

- `atom_sign_conflict_rate` separates sampled under-ranked positives from
  blockers within query.
- Scalar score features remain non-separable, matching M611/M617.
- Signed dot and P1 score are not useful for this recovery problem.

This is enough to justify one conservative M621 replay probe.  It is not enough
to justify native DB/plugin evaluation by itself.

## Decision

Run M621 as a bounded probe:

- preserve top head,
- use `atom_sign_conflict_rate`,
- admit only bottom-slot candidates from a fixed rank window,
- evaluate on full M604 replay before any native benchmark.

If M621 does not beat M619 by a meaningful margin, stop scorer-route tuning and
return to retrieval objective / teacher-signal redesign.

## Verification

Local verification passed:

- `python3 -m py_compile scripts/audit_m620_atom_interaction_separability.py`
- `pytest -q tests/test_audit_m620_atom_interaction_separability.py`
