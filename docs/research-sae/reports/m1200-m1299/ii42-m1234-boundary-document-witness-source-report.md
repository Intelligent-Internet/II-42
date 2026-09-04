# M1234 Boundary Document Witness Source

## Purpose

M1234 tested a new source class after M1230-M1233 failed to produce a safe
deployable atom/event selector. Instead of using old action delta atoms, it
used oracle relevant boundary/tail documents as witnesses and extracted their
document atoms.

This is a source-feasibility audit only. It does not train and does not replay.

## Runs

- Initial smoke JSON: `runs/m1234_boundary_document_witness_source_smoke_v1/m1234_boundary_document_witness_source.json`
- Corrected smoke JSON: `runs/m1234_boundary_document_witness_source_smoke_v2/m1234_boundary_document_witness_source.json`

The initial run excluded base query atoms. That was too narrow because
CUB-specific target deltas may change existing query atoms. The corrected run
allows both existing and new atoms.

## Corrected Smoke Result

Surface: `cqadupstack`, `scidocs`, `webis-touche2020`.

Source baseline is the existing CUB-specific source-abs top8 selector:

| Baseline | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| source_abs top8 | 0.976959 | 0.184991 | 0.006108 | 0.178883 |

Witness source variants:

| Variant | TargetRecall | Precision | HarmPrecision | Gap |
| --- | ---: | ---: | ---: | ---: |
| rel_tail_top8 | 0.041475 | 0.011719 | 0.000000 | 0.011719 |
| rel_tail_minus_top_top8 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| rel_boundary_top8 | 0.046083 | 0.020161 | 0.000000 | 0.020161 |
| rel_boundary_minus_top_top8 | 0.000000 | 0.000000 | 0.000000 | 0.000000 |

## Decision

Stop this source at smoke. Do not run full shared15 and do not train or replay
from boundary-document witnesses.

Reason:

- Even with oracle qrels and relevant tail/boundary documents, witness atoms
  barely overlap the CUB-specific target atoms.
- Removing atoms that appear in nonrelevant top documents eliminates harm but
  also eliminates target coverage.
- The failure is upstream of qrels-free modeling; a weaker deployable proxy
  cannot recover a source that is already weak with oracle documents.

## Implication

This rules out a tempting explanation: the missing proposal source is not
simply "atoms from relevant missed documents." The CUB-specific movement atoms
appear to be action/compiler-surface atoms, not directly doc-witness atoms.

Combined recent evidence:

- M1230: static atom-doc context is not enough.
- M1231: directional atom-boundary movement features are not enough.
- M1232: harm-separated filtering of existing action atoms loses target
  coverage.
- M1233: action-event selection collapses to action prior.
- M1234: oracle relevant document atoms do not recover CUB-specific target
  atoms.

The next useful branch should inspect the target atoms themselves: where do
they come from, what support they have in query atoms, action deltas, and doc
atoms, and whether they form a small reusable compiler basis. Without that
source audit, further proposal models are likely blind.
