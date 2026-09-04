# M680 Support Selector Multiseed Report

## Scope

M680 tests whether the M679 `support_floor_overlap` checkpoint selector is
stable beyond seed `6547`.

The run keeps the first-stage constraints:

- no BM25
- no reranker
- no learned gate
- no qrels-driven loss
- no qrels-driven checkpoint selection
- frozen document posting/index geometry
- query-side/output compiler only

M680 runs seeds `6545`, `6546`, and `6548`.  Seed `6547` is reused from M679
because it used the same configuration.

## Selection Results

| Seed | Selected Epoch | Selected Step | Gate |
| ---: | ---: | ---: | --- |
| 6545 | `5` | `45` | `dense_equivalence_gate_passed` |
| 6546 | `5` | `45` | `dense_equivalence_gate_passed` |
| 6547 | `5` | `45` | `dense_equivalence_gate_passed` |
| 6548 | `5` | `45` | `dense_equivalence_gate_passed` |

The support-aware selector consistently selects the early support-preserving
checkpoint.  This fixes the M677 seed6547 failure mode, where the legacy
selector chose epoch `32`.

## Full Shared15 Matrix

| Seed | O@10 | O@50 | O@100 | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | Support |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000052296` | `0.000000000` | `+0.000000632` | `0.000000000` | `0.000000000` | `0.000000000` | `-0.000000159` |
| 6546 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000062606` | `0.000000000` | `-0.000226969` | `-0.000172551` | `-0.000330556` | `0.000000000` | `-0.000000167` |
| 6547 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000084287` | `0.000000000` | `+0.000164443` | `+0.000150863` | `+0.000333333` | `0.000000000` | `-0.000000163` |
| 6548 | `0.000000000` | `0.000000000` | `0.000000000` | `+0.000086785` | `0.000000000` | `-0.000060343` | `-0.000000799` | `0.000000000` | `0.000000000` | `-0.000000163` |

Full aggregate:

- pass-like seeds: `4/4`
- minimum O@10/O@50/O@100: `0`
- minimum O@256: `+0.000052296`
- mean O@256: `+0.000071494`
- mean Recall@100: `0`
- mean CUB: `0`
- mean MAP@100: `-0.000030559`
- mean NDCG@10: `-0.000005622`
- mean MRR@20: `+0.000000694`

## Test Split Matrix

| Seed | O@256 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Support |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6545 | `+0.000046286` | `0.000000000` | `+0.000000211` | `0.000000000` | `0.000000000` | `-0.000000119` |
| 6546 | `+0.000055795` | `0.000000000` | `-0.000008110` | `0.000000000` | `+0.000005342` | `-0.000000119` |
| 6547 | `+0.000101568` | `0.000000000` | `+0.000371414` | `+0.000335251` | `+0.000740741` | `-0.000000159` |
| 6548 | `+0.000086195` | `0.000000000` | `-0.000107804` | `-0.000039433` | `0.000000000` | `-0.000000163` |

Test aggregate:

- mean O@256: `+0.000072461`
- mean Recall@100: `0`
- mean MAP@100: `+0.000063928`
- mean NDCG@10: `+0.000073955`
- mean MRR@20: `+0.000186521`

## Comparison With M675

M675 legacy selection used different epochs for several seeds:

| Seed | M675 Epoch | M680 Epoch | M675 Full MAP@100 | M680 Full MAP@100 |
| ---: | ---: | ---: | ---: | ---: |
| 6545 | `11` | `5` | `-0.000000278` | `+0.000000632` |
| 6546 | `8` | `5` | `-0.000169416` | `-0.000226969` |
| 6547 | `5` | `5` | `+0.000164443` | `+0.000164443` |
| 6548 | `9` | `5` | `+0.000003947` | `-0.000060343` |

The support-aware selector is safer for support drift, but it is too blunt as
a universal multiseed selector.  It improves or preserves seed `6545` and
`6547`, but degrades full MAP on `6546` and `6548` compared with M675's legacy
selection.

## Decision

Do not promote `support_floor_overlap` as the final default selector.

M679/M680 still provide useful progress:

1. They prove the M677 failure is selection-related, not a hard training
   instability.
2. They add an opt-in dense-only selector that can prevent support-drifted
   checkpoints from being selected.
3. They show support cosine alone is not enough; future selection needs
   dense-only rank/score geometry features, not qrels and not BM25.

Keep M676 seed `6547` as the current first-stage candidate.  The next
experiment should add dense rank-margin or score-calibration deformation to
checkpoint selection and possibly training diagnostics, while preserving the
same hard dense-equivalence gates.
