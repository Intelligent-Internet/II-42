# M519 TopK-Preserve Route LoRA Report

M519 returns to the encoder/support route objective after M516-M518 rejected
simple adaptive prefix controllers.  The hypothesis was that the route phase
may be under-preserving dense teacher candidates because M512/M514 trained with
only top8 positive docs per query.

M519 keeps the same base PPLX LoRA encoder and same route evaluator, but
expands route-phase dense positives to top32/top64.

## Run Surface

- Script: `scripts/research_sae_m519_topk_preserve_route_lora.py`
- Local output:
  `outputs/m519/fiqa_topk_preserve_route_lora/m519_fiqa_topk_preserve_route_lora.json`
- Remote host: `spark-1`
- Task: `FiQA2018`
- Eval queries: `64`
- Route docs: `4096 / 57638`
- Route prefixes: `64, 96, 128`
- Presets: `baseline_pos8, preserve_pos32, preserve_pos64, preserve_pos32_e2`

## FiQA Matrix

| Preset | Prefix | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline_pos8` | 64 | 0.46533 | 0.84751 | 0.57371 | 0.41287 | 0.82344 | 0.51522 |
| `baseline_pos8` | 96 | 0.46636 | 0.87786 | 0.57180 | 0.41645 | 0.89047 | 0.60588 |
| `baseline_pos8` | 128 | 0.46747 | 0.88620 | 0.57175 | 0.41862 | 0.92859 | 0.67009 |
| `preserve_pos32` | 64 | 0.44066 | 0.82054 | 0.55498 | 0.39131 | 0.78500 | 0.48908 |
| `preserve_pos32` | 96 | 0.44779 | 0.83371 | 0.55490 | 0.39776 | 0.86219 | 0.58066 |
| `preserve_pos32` | 128 | 0.44621 | 0.85714 | 0.55229 | 0.39647 | 0.89734 | 0.64155 |
| `preserve_pos64` | 64 | 0.44699 | 0.82496 | 0.55682 | 0.39382 | 0.83484 | 0.52586 |
| `preserve_pos64` | 96 | 0.45303 | 0.86090 | 0.55682 | 0.39947 | 0.89625 | 0.62003 |
| `preserve_pos64` | 128 | 0.45456 | 0.87016 | 0.55644 | 0.40172 | 0.92812 | 0.68493 |
| `preserve_pos32_e2` | 64 | 0.42007 | 0.74888 | 0.53014 | 0.36773 | 0.67359 | 0.46734 |
| `preserve_pos32_e2` | 96 | 0.44404 | 0.82746 | 0.56299 | 0.39032 | 0.76813 | 0.55883 |
| `preserve_pos32_e2` | 128 | 0.44999 | 0.84699 | 0.55865 | 0.40023 | 0.82688 | 0.62573 |

Dense reference on the same route subset:

- `route_subset_teacher_row_int8_dense`: NDCG@10 `0.47062`,
  Recall@100 `0.85703`, MAP@100 `0.41763`.
- `route_subset_materialized_dense`: NDCG@10 `0.47054`,
  Recall@100 `0.86224`, MAP@100 `0.41764`.

## Interpretation

M519 does not improve the route frontier.

The same-run `baseline_pos8` remains the best learned support route:

- Best NDCG: `baseline_pos8` p128 at `0.46747`.
- Best Recall@100: `baseline_pos8` p128 at `0.88620`.
- Best MAP@100: `baseline_pos8` p128 at `0.41862`.

Expanding positives did not solve candidate preservation:

- `preserve_pos32` p128 drops candidate recall from `0.92859` to `0.89734`
  and drops NDCG from `0.46747` to `0.44621`.
- `preserve_pos64` p128 nearly preserves candidate recall
  (`0.92812` vs `0.92859`) but still drops NDCG to `0.45456` and increases
  touch to `0.68493`.
- `preserve_pos32_e2` over-trains the route phase: candidate recall and qrels
  metrics both collapse.

This suggests the problem is not simply that route training saw too few
dense-positive docs.  Broadening the positive set makes the support surface
less rank-discriminative even when candidate recall is preserved.

## Decision

Stop the simple route-phase objective-grid branch:

- M514 showed load balancing changes coordinates but hurts useful candidates.
- M516-M518 showed adaptive prefix policies cannot recover enough quality from
  the current support surface.
- M519 shows larger dense-positive route groups do not improve the support
  route and can damage ranking shape.

The next productive direction should return to the encoder target itself:
distill a support/posting representation that preserves dense geometry before
route compression, rather than trying to fix the route phase with sampled
contrastive objectives.

Concretely, the next gate should compare a geometry-preserving support target
against the current M510/M512 target on the same FiQA route subset before any
new fanout or controller work.
