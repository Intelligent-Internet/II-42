# M700 Dense-Boundary Atom Admission

## Purpose

M700 tests whether the current first-stage bottleneck is just insufficient
training depth, or whether the dense-boundary atom interface itself lacks
enough recoverable target atoms before any deeper compiler training.

The audit uses M653 qrels-free dense-boundary rows. It does not optimize qrels,
BM25, reranker signals, or learned gates. The goal is to decide whether a
support-safe dense-equivalence compiler is worth launching from the current
atom candidate interface.

## Method

- Baseline surface: `P1.3 / M549U native signed-dot`.
- Training rows: `runs/m653_dense_boundary_training_rows_canary_v1/m653_dense_boundary_training_rows.jsonl`.
- Positive target atoms: atoms present in dense-positive boundary docs and not
  dominated by false-P1 negative boundary docs.
- Negative/risk atoms: atoms present only in false-P1 negative boundary docs.
- Candidate atoms: expanded corpus-support source pool, frozen doc posting
  geometry, no qrels-driven selection.
- Model: global linear atom-admission diagnostic model over existing native atom
  features.

## Results

Full run output:

- JSON: `runs/m700_dense_boundary_atom_admission_v1/m700_summary.json`
- Markdown: `runs/m700_dense_boundary_atom_admission_v1/m700_report.md`

Atom model:

| Split | Rows | Positives | Positive share | AUC |
| --- | ---: | ---: | ---: | ---: |
| train | 97152 | 6700 | 0.068964 | 0.706494 |
| eval | 56064 | 3644 | 0.064997 | 0.710223 |

Eval selection surface:

| Mode | Visibility | Negative-only share | Full-hit rows |
| --- | ---: | ---: | ---: |
| raw_top48 | 0.244490 | 0.157820 | 0 |
| model_top48 | 0.246562 | 0.160959 | 0 |
| raw_top96 | 0.360143 | 0.120291 | 1 |
| model_top96 | 0.364852 | 0.125856 | 0 |
| raw_top192 | 0.508947 | 0.089755 | 1 |
| model_top192 | 0.517612 | 0.092930 | 0 |
| raw_top384 | 0.686382 | 0.061626 | 2 |
| model_top384 | 0.686382 | 0.061626 | 2 |
| label_visible_all | 0.686382 | 0.000000 | 2 |

## Interpretation

M700 finds a candidate-interface ceiling. The label-visible upper bound on eval
is only `0.686382`, below the `0.80` gate required before launching dense
equivalence compiler training.

The atom model does learn a weak useful signal: top192 visibility improves from
`0.508947` to `0.517612`. However, negative-only risk also rises from
`0.089755` to `0.092930`, and the top384 result is capped by the candidate pool
itself.

This means deeper training on the current interface is not justified yet. The
first-stage bottleneck is not simply insufficient optimization steps. The
current atom target construction or candidate features fail to expose enough
dense-boundary target atoms.

## Decision

Do not launch compiler training from this M700 interface.

The next useful step is to redesign the first-stage atom interface before any
longer training:

1. Add target construction that captures dense-boundary atoms not visible in the
   current corpus-support candidate pool.
2. Add features that distinguish dense-positive support atoms from false-P1
   negative support atoms without increasing negative-only risk.
3. Re-run the admission audit and only launch deeper compiler training if
   label-visible eval visibility reaches at least `0.80` and model selection
   does not increase protected negative-only risk.

## Training Depth Assessment

The current fast iteration strategy is reasonable only because it is being used
as a feasibility filter, not as final training. A route should be expanded when
loss is still improving and dense overlap/support gates remain intact. A route
should not be expanded when the audit shows an upper-bound or interface ceiling.

M700 is the latter case. More training is unlikely to solve the observed gap
until the atom candidate interface changes.
