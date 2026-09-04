# M634-A / Artifact Family Audit

## Objective

M634 tests compiler-basis repair after M633 showed inverted support geometry.
The audit groups representation signals into artifact families and asks whether
false-head documents are separable from dense-tail positives before any
suppression is applied.

- Contrast rows: `3420`
- Dense-tail positives: `1710`
- False-head negatives: `1710`

| Family | AUC | Cohen d | Positive mean | Negative mean |
| --- | ---: | ---: | ---: | ---: |
| `basis_artifact` | 0.199981 | -1.062699 | 3.446650 | 3.949593 |
| `margin_artifact` | 0.204097 | -1.055476 | 3.656790 | 4.184281 |
| `overlap_artifact` | 0.222286 | -0.988187 | 2.980440 | 3.327616 |
| `support_artifact` | 0.278792 | -0.818561 | 2.660079 | 3.077551 |
| `negative_low_conflict_artifact` | 0.458232 | -0.139369 | 0.208741 | 0.274078 |

## Top Feature Families

| Feature | AUC | Cohen d | Positive mean | Negative mean |
| --- | ---: | ---: | ---: | ---: |
| `signed_margin_qz` | 0.186308 | -1.125409 | 1.209578 | 1.391795 |
| `atom_signed_dot_qz` | 0.186308 | -1.125409 | 1.209578 | 1.391795 |
| `atom_conflict_rate_qz` | 0.712450 | +0.739109 | -1.237634 | -1.400692 |
| `atom_negative_mass_qz` | 0.585197 | +0.310042 | -1.138833 | -1.270259 |
| `conflict_penalty_qz` | 0.585197 | +0.310042 | -1.138833 | -1.270259 |
| `support_energy_qz` | 0.456344 | -0.151087 | 0.439761 | 0.574577 |
| `atom_shared_doc_frac` | 0.469173 | -0.107927 | 0.674740 | 0.676309 |
| `atom_shared_query_frac` | 0.469173 | -0.107927 | 0.337370 | 0.338154 |
| `atom_weighted_overlap` | 0.478548 | -0.069901 | 0.560401 | 0.563028 |

## Interpretation

The artifact families are clearly separable. `basis_artifact` and
`margin_artifact` have AUC near 0.20 when dense-tail is the positive class,
which means the feature is higher on false-head negatives. This confirms the
M633 finding: the representation problem is over-strong false-head artifact
support, not a simple absence of dense-tail support.

This audit is only a necessary condition. A useful compiler-basis repair still
must suppress those artifact families without losing dense overlap.
