# M1235 CUB Target Atom Anatomy

## Purpose

M1230-M1234 rejected several candidate/source shapes. M1235 stops proposing
new policies and inspects the CUB-specific target atoms themselves.

The question is whether the target atoms look like:

- existing query atoms needing weight movement,
- new added atoms needing a proposal source,
- globally safe atoms that can be filtered by prior,
- or context-dependent atoms that require a stronger compiler interface.

## Runs

- Smoke JSON: `runs/m1235_cub_target_atom_anatomy_smoke_v1/m1235_cub_target_atom_anatomy.json`
- Corrected full JSON: `runs/m1235_cub_target_atom_anatomy_v2/m1235_cub_target_atom_anatomy.json`
- Corrected full Markdown: `runs/m1235_cub_target_atom_anatomy_v2/m1235_cub_target_atom_anatomy.md`

The v2 run fixes macro unique-atom accounting by aggregating full atom counts
instead of only per-dataset top atoms.

## Full Shared15 Anatomy

| Label | Rows | Queries | UniqueAtoms | BasePresent | AddedShare | AbsDelta | PosCount | NegCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| target | 2936 | 420 | 635 | 0.0000 | 1.0000 | 0.009076 | 10.0361 | 2.6499 |
| harm | 919 | 61 | 343 | 0.0000 | 1.0000 | 0.008550 | 12.2470 | 2.5680 |

Action distribution:

| Label | high | mid | low |
| --- | ---: | ---: | ---: |
| target | 0.6250 | 0.0899 | 0.2851 |
| harm | 0.3025 | 0.2492 | 0.4483 |

Top target atoms:

| Atom | TargetCount | InHarm |
| ---: | ---: | --- |
| 65 | 76 | yes |
| 154 | 52 | yes |
| 37 | 49 | yes |
| 39 | 39 | yes |
| 10 | 38 | yes |
| 33 | 30 | yes |
| 60 | 29 | yes |
| 55 | 27 | yes |

Unique overlap:

- target unique atoms: `635`
- harm unique atoms: `343`
- overlap: `257`
- target-side overlap share: `0.4047`
- harm-side overlap share: `0.7493`

## Interpretation

This is the clearest structural evidence from the recent sequence:

1. The target is an added-atom problem, not an existing-weight movement problem.
   `base_present_share=0` for both target and harm.
2. Global atom safety is weak. Harm atoms are not a separate atom family:
   `74.93%` of harm unique atoms also appear in target, and the top target
   atoms mostly also appear in harm.
3. Action context matters. Target rows are dominated by `high`, while harm rows
   skew toward `low` and `mid`, but action prior alone was already exposed as a
   false-positive in M1233.
4. Relevant document atoms are not the target source. M1234 showed oracle
   relevant tail/boundary documents barely recover these target atoms.

## Decision

Do not continue global atom filters, boundary-document witness sources, or
action-only event selection.

The next viable line is a context-conditioned added-atom compiler:

- input: query native rank context + action/source context + current query atom
  surface
- output: added atoms only
- constraint: target/harm overlap requires contextual safety, not global atom
  whitelist
- first gate: added-atom target/harm separability on held-out datasets
- second gate: bounded native replay only after separability improves

## Stop Rule For Next Branch

The next branch must beat the current source baseline on target coverage or
precision-minus-harm gap before replay. If it only changes action priors,
global thresholds, or source_abs ranking without new context-conditioned
evidence, stop at smoke.
