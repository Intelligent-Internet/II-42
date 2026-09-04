# M550 BM25-Aware Trained Fusion Report

## Purpose

M550 starts from the M549 dense-equivalent surface and asks whether BM25 can
be added without breaking the dense-faithful route.  The experiment keeps the
evaluation query-heldout and avoids dataset-id features.

The route tests three families:

- Fixed z-score blends: `m549_z + alpha * bm25_z`.
- A constrained trained query gate: choose whether to use `alpha=0.10`.
- A free learned residual scorer over document-level runtime features.

## Runs

- Smoke:
  `outputs/m550/bm25_aware_trained_fusion/smoke_gate_fiqa_scidocs_trec_seed550/`
- Full broad10:
  `outputs/m550/bm25_aware_trained_fusion/broad10_bm25_trained_gate_seed550/`
- Remote full broad10:
  `/home/huoju/leask/runs/ii42-m550-bm25-aware-trained-fusion-v1/broad10_bm25_trained_gate_seed550/`

## Broad10 Heldout Macro

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57220 | 0.43642 | 0.75068 | 0.64444 | 0.00000 | 0.00000 |
| `m549_tail768` | 0.57243 | 0.43659 | 0.75050 | 0.64458 | 0.00023 | -0.00018 |
| `m549_bm25_zblend_a005` | 0.58310 | 0.44382 | 0.75406 | 0.64984 | 0.01090 | 0.00338 |
| `m549_bm25_zblend_a010` | 0.58735 | 0.44793 | 0.75692 | 0.65682 | 0.01515 | 0.00624 |
| `m549_bm25_zblend_a015` | 0.58613 | 0.44818 | 0.75567 | 0.65455 | 0.01393 | 0.00499 |
| `m550_trained_gate_a010` | 0.58705 | 0.44758 | 0.75643 | 0.65657 | 0.01485 | 0.00575 |
| `m550_trained_residual` | 0.52763 | 0.40635 | 0.72487 | 0.60581 | -0.04457 | -0.02581 |
| `bm25` | 0.40374 | 0.28775 | 0.59426 | 0.48279 | -0.16846 | -0.15642 |

Heldout promotion winner: `m549_bm25_zblend_a010`.

Against exact dense, it improves:

- NDCG@10: `+0.01515`.
- MAP@100: `+0.01151`.
- Recall@100: `+0.00624`.
- MRR@20: `+0.01238`.

## Broad10 All Macro

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 0.00000 | 0.00000 |
| `m549_bm25_zblend_a010` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.01782 | 0.00691 |
| `m549_bm25_zblend_a015` | 0.59179 | 0.44822 | 0.75487 | 0.66425 | 0.01916 | 0.00684 |
| `m550_trained_gate_a010` | 0.59044 | 0.44782 | 0.75462 | 0.66404 | 0.01781 | 0.00659 |

The `all` split slightly favors `alpha=0.15`, but heldout promotion favors
`alpha=0.10`.  M550 therefore keeps `alpha=0.10` as the conservative promoted
surface.

## Interpretation

M550 proves the dense-equivalent M549 route can be strengthened by a small
BM25 contribution.  This is not BM25 rescue of a weak dense proxy: `m549_tail768`
already matches exact dense, and the BM25 blend adds a stable heldout gain on
top of that surface.

The trained query gate is close to the fixed blend but does not beat it on
heldout.  It remains useful as evidence that constrained training can stay
safe, but it is not the promoted route.

The free residual scorer is a negative result.  Its pairwise loss can train,
but the learned weights distort the ranking shape and lose heavily on both
train and heldout.  This route should not be promoted without a much stricter
monotonic or alpha-selection constraint.

## Current Best Surface

For this stage, the best robust surface is:

```text
M549 tail768 gamma=1.02562527
+ z-score BM25 blend alpha=0.10
```

This is the surface to compare against any later BM25-aware learned optimizer.

## Next Work

The next meaningful training direction is not a free document-level residual.
It should be constrained to preserve the dense-equivalent ordering shape:

- Train query-level alpha selection over a small alpha set, not arbitrary
  document residuals.
- Select alpha using train queries only, with heldout promotion.
- Add monotonic constraints or fallback floors so learned behavior cannot fall
  below `m549_tail768` on query groups where BM25 is harmful.
- Optimize the M550 evaluator by reusing score arrays/top-k work; full broad10
  currently spends most time in repeated per-source ranking.
