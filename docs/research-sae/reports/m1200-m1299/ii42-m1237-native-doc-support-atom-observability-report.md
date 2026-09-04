# M1237 Native Doc-Support Atom Observability Report

M1237 tested whether CUB-specific added target/harm atoms become separable
when each atom is described by its query-time support in native candidate
documents.  This addresses a missing feature class from M1220/M1236: previous
audits used action deltas, global atom priors, and query-level native summary
features, but did not inspect how an atom is distributed across top/boundary
native documents for the current query.

## Inputs

- Label surface: M1224 CUB-specific target/harm atoms.
- Candidate atoms: same action-delta candidate universe as M1224/M1236.
- Features: per-query document support windows over native top256
  (`top10`, `top50`, `top100`, `boundary95_100`, `tail100_256`, `top256`).
- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.
- Corrected output:
  `runs/m1237_native_doc_support_atom_observability_smoke_v2/m1237_native_doc_support_atom_observability.json`

## Important Correction

The first smoke counted only queries with generated support rows for support
variants.  This made the support variants use `243` queries while the source
baseline used `249`.  The script was corrected so all variants share the same
target/harm query universe; queries with no selected proposal are counted with
empty predictions.

## Corrected Smoke Result

Source atom baseline:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |

Best support-conditioned variants:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `support_only_rule_top100_impact_top8` | 0.9816 | 0.1859 | 0.0061 | 0.1798 | 4.602 |
| `support_only_model_logistic_top8` | 0.9816 | 0.1859 | 0.0061 | 0.1798 | 4.602 |
| `support_only_rule_top100_impact_top12` | 1.0000 | 0.1828 | 0.0059 | 0.1769 | 4.767 |
| `support_only_model_logistic_top12` | 0.9954 | 0.1820 | 0.0059 | 0.1761 | 4.767 |

## Interpretation

Native document-support features are not enough to justify full shared15 or
native replay.  The best top8 variants have a tiny gain over source baseline:
target recall increases by `0.0046`, while gap improves by less than `0.001`.
The top12 variants recover all target atoms on the smoke surface, but precision
falls enough that the separation gap is worse than source.

This is not a deployable selector/generator signal.  It is useful evidence that
query-time support geometry has some weak information, but not enough to solve
target/harm separation by itself.

## Decision

Stop M1237 at corrected smoke.

- Do not run full shared15 for this exact feature shape.
- Do not launch native replay.
- Preserve the feature class as a possible auxiliary signal, not a main
  proposal source.

The next route must change either the label construction or the proposal source
so that harm separation is created before scoring.  Existing added-atom
features can recover target atoms, but precision collapses or gains remain too
small when harm is counted correctly.
