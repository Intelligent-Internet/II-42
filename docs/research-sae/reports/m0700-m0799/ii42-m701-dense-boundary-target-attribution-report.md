# M701 Dense-Boundary Target Attribution

## Purpose

M700 showed that the current atom-admission interface had an eval
label-visible ceiling of `0.686382`, below the `0.80` gate needed before
starting deeper dense-equivalence compiler training.

M701 attributes that ceiling. It asks whether missing dense-boundary target
atoms are lost because of source-document breadth, doc-atom head width, or the
candidate-atom cap.

This is still a first-stage audit:

- no qrels objective
- no BM25 objective
- no reranker
- no learned gate
- frozen doc posting/index geometry

## Inputs

- Dense-boundary rows:
  `runs/m653_dense_boundary_training_rows_canary_v1/m653_dense_boundary_training_rows.jsonl`
- Native PostgreSQL path over the M653 canary surface.
- Baseline mode equivalent to M700:
  `source384_head12_cap384`.

Full run outputs:

- JSON: `runs/m701_dense_boundary_target_attribution_v1/m701_summary.json`
- Markdown: `runs/m701_dense_boundary_target_attribution_v1/m701_report.md`

## Aggregate Result

| Mode | Visibility | Negative-only share | Full-hit rows |
| --- | ---: | ---: | ---: |
| `source384_head12_cap384` | 0.689370 | 0.061182 | 5 |
| `source384_head12_cap1536` | 0.944352 | 0.029628 | 101 |
| `source384_head48_cap1536` | 0.983472 | 0.022284 | 242 |
| `source384_head48_cap4096` | 0.996268 | 0.019152 | 352 |
| `source1000_head48_cap1536` | 0.983539 | 0.022292 | 235 |
| `source1000_head48_cap4096` | 0.999467 | 0.017492 | 393 |

The baseline ceiling is reproduced: `source384_head12_cap384` reaches
`0.689370`, matching M700's `0.686382` eval ceiling.

Expanding candidate atom cap alone helps strongly. Keeping source size and
head fixed but changing `cap384 -> cap1536` raises visibility from `0.689370`
to `0.944352`.

Increasing doc-atom head from `12 -> 48` then raises the practical
`source384/cap1536` mode to `0.983472`.

Increasing source size from `384 -> 1000` is not the main driver once head and
cap are fixed: `source384_head48_cap1536` and `source1000_head48_cap1536` are
effectively tied.

## Source-Doc Coverage

| Source size | Positive docs in support | Negative docs in support |
| ---: | ---: | ---: |
| 384 | 655 / 1490 | 647 / 1449 |
| 768 | 847 / 1490 | 852 / 1449 |
| 1000 | 862 / 1490 | 865 / 1449 |

More source docs do improve document coverage, but the atom visibility gain is
small compared with widening atom head and candidate cap.

## Interpretation

M701 reverses the M700 concern. The first-stage dense-boundary atoms are not
mathematically unavailable, and this is not mainly a training-depth issue.

The issue is the atom candidate interface:

1. M700 used a narrow `head12/cap384` interface.
2. That interface capped target atom visibility around `0.69`.
3. A qrels-free expanded interface reaches `0.98+` visibility on the same
   dense-boundary target.

This is a real positive signal for the current first-stage route. It just means
the compiler should not be trained from the M700 interface.

## Decision

Proceed to M702: expanded-interface atom admission / compiler feasibility.

The recommended practical starting mode is:

- `source384_head48_cap1536`

It reaches `0.983472` visibility without requiring wider source-document
breadth. This is cheaper and cleaner than `source1000_head48_cap4096`.

M702 should test whether a global admission model can select a much smaller
support-safe subset from this expanded interface while preserving dense target
visibility and keeping negative-only risk low. Only after that should we launch
longer dense-equivalence compiler training.

## Training-Depth Assessment

This result answers the earlier concern more precisely:

- It would have been premature to abandon the route after M700.
- It would also have been wrong to simply train longer on M700.
- The right correction is to expand the atom interface first, then train.

The current exploration ratio is acceptable when these audits are used as
necessary-condition gates before expensive training. M701 shows the next
expensive step now has a better-defined interface and a stronger justification.
