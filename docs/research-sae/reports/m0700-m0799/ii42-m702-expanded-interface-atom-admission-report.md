# M702 Expanded-Interface Atom Admission

## Purpose

M701 found that M700's failure was caused by a narrow atom candidate interface:
`source384_head12_cap384` capped dense-boundary target visibility around
`0.69`, while `source384_head48_cap1536` reached `0.983472`.

M702 tests the next question: once the dense-boundary atoms are visible, can
the current linear atom-admission model compress the expanded candidate set
back into a smaller selected set?

This remains first-stage only:

- no qrels objective
- no BM25 objective
- no reranker
- no learned gate
- frozen doc posting/index geometry

## Run

Command shape:

```bash
python3 scripts/audit_m700_dense_boundary_atom_admission.py \
  --doc-atom-head 48 \
  --max-atom-candidates-per-query 1536 \
  --corpus-source-size 384 \
  --output-root runs/m702_expanded_interface_atom_admission_v1
```

Outputs:

- JSON: `runs/m702_expanded_interface_atom_admission_v1/m700_summary.json`
- Markdown: `runs/m702_expanded_interface_atom_admission_v1/m700_report.md`

## Atom Model

| Split | Rows | Positives | Positive share | AUC |
| --- | ---: | ---: | ---: | ---: |
| train | 388608 | 9537 | 0.024541 | 0.816099 |
| eval | 224256 | 5220 | 0.023277 | 0.816738 |

The expanded interface materially improves separability versus M700
(`eval AUC 0.710223 -> 0.816738`). So the features are not useless.

## Eval Selection Surface

| Mode | Visibility | Negative-only share | Full-hit rows |
| --- | ---: | ---: | ---: |
| raw_top48 | 0.244867 | 0.162100 | 0 |
| model_top48 | 0.246186 | 0.162671 | 0 |
| raw_top96 | 0.367301 | 0.122860 | 0 |
| model_top96 | 0.367678 | 0.125642 | 0 |
| raw_top192 | 0.522509 | 0.089219 | 0 |
| model_top192 | 0.523262 | 0.091039 | 0 |
| raw_top384 | 0.691091 | 0.061465 | 0 |
| model_top384 | 0.707666 | 0.062857 | 0 |
| label_visible_all | 0.983236 | 0.000000 | 87 |

## Interpretation

M702 confirms two separate facts:

1. Expanded interface visibility is sufficient.
2. The current linear admission model does not compress it into top384.

The model improves top384 visibility only from `0.691091` to `0.707666`, while
negative-only selected share rises from `0.061465` to `0.062857`. This is not a
good enough admission layer for compiler training.

M701 also showed that raw `source384_head48_cap768` already reaches
`0.864445`, and raw `source384_head48_cap1536` reaches `0.983472`. Therefore
the issue is specifically compression/admission, not dense-boundary atom
availability.

## Decision

Do not launch a long compiler training run using the current linear admission
selector.

The next useful step is M703:

1. Treat `source384_head48_cap768` and `source384_head48_cap1536` as expanded
   first-stage candidate interfaces.
2. Test whether the native query/posting path can tolerate a larger query atom
   budget without breaking dense overlap/support.
3. In parallel, design a stronger admission objective that optimizes
   set-coverage under a selected atom budget, rather than independent atom
   logistic scores.

The route is still alive. The bottleneck has moved from atom visibility to
budgeted set selection.
