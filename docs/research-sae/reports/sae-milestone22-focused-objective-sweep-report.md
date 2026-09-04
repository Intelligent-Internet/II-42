# SAE Milestone 22 Focused Objective Sweep

## Scope

This sweep exhausts the current focused semantic-coverage direction. It tests whether the previously useful `student_encoder_loss` focus signal can be converted into a promotable checkpoint with full15 ranking quality and native active8 physical-cost evidence.

All runs use the BEIR-only rebuilt `shared_sae_8192_64` teacher under `/Volumes/Betty/Tmp/ii42_sae_beir15_shared`. This keeps the comparison apples-to-apples with the previous focus6 report.

Listwise candidate-budget training is materially more expensive than the other objectives, but training cost is not a promotion criterion. It remains in the sweep because a slower offline trainer is acceptable if it produces a better query-time index.

A coverage batch-size increase to 64 was also screened out because it triggered local MPS out-of-memory errors. The final sweep therefore keeps the trainer defaults for auxiliary batch sizes.

## Variants

| Variant | Role | Rationale |
| --- | --- | --- |
| `beironly-baseline` | reference | Same-teacher baseline after the stable scratch rebuild. |
| `focus6` | reference | Raw focused-repeat diagnostic from the previous pass. |
| `listwise-budget-f3` | candidate | Use focused failures in dense-teacher listwise training while keeping active query atoms budgeted. |
| `coverage-budget-f3` | candidate | Use focused failures in dense-teacher coverage training and increase active-query budget pressure. |
| `safe-exposure-f3` | candidate | Penalize exposure outside dense/qrel-safe neighborhoods instead of repeating all focused pairs blindly. |
| `safe-exposure-f2-lowfanout` | candidate | Stronger fanout/exposure pressure without the expensive listwise objective. |
| `safe-exposure-f1-strong` | candidate | No raw focus repeat; use stronger safe-exposure and budget losses to test whether the objective alone can reduce cost. |

## Full15 Quality

| Variant | Best Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Delta NDCG | Delta MAP |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `beironly-baseline` | `bm25_student_atoms_w0p25` | 0.7867 | 0.7899 | 0.6699 | 0.6282 | +0.0000 | +0.0000 |
| `focus6` | `bm25_student_atoms_w0p25` | 0.7865 | 0.7931 | 0.6734 | 0.6299 | +0.0035 | +0.0017 |
| `listwise-budget-f3` | `bm25_student_atoms_w0p25` | 0.7803 | 0.7900 | 0.6660 | 0.6205 | -0.0039 | -0.0077 |
| `coverage-budget-f3` | `bm25_student_atoms_w0p25` | 0.7849 | 0.7896 | 0.6691 | 0.6242 | -0.0008 | -0.0041 |
| `safe-exposure-f3` | `bm25_student_atoms_w0p25` | 0.7851 | 0.7881 | 0.6690 | 0.6249 | -0.0010 | -0.0033 |
| `safe-exposure-f2-lowfanout` | `bm25_student_atoms_w0p25` | 0.7794 | 0.7849 | 0.6636 | 0.6161 | -0.0063 | -0.0122 |
| `safe-exposure-f1-strong` | `bm25_student_atoms_w0p25` | 0.7847 | 0.7848 | 0.6678 | 0.6235 | -0.0021 | -0.0047 |

## Native Active8 Cost

| Variant | Recall@100 | NDCG@10 | MAP@100 | Candidates | SAE Posts | BM25 Posts | Payload MB | Delta Candidates | Delta SAE Posts |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `beironly-baseline` | 0.7731 | 0.6575 | 0.6052 | 670.3 | 767.3 | 677.5 | 9.19 | +0.0 | +0.0 |
| `focus6` | 0.7761 | 0.6589 | 0.6023 | 768.1 | 809.6 | 677.5 | 9.26 | +97.8 | +42.3 |
| `listwise-budget-f3` | 0.7339 | 0.5871 | 0.5105 | 605.2 | 500.6 | 677.5 | 9.46 | -65.1 | -266.7 |
| `coverage-budget-f3` | 0.7595 | 0.5978 | 0.5286 | 565.2 | 452.8 | 677.5 | 9.52 | -105.1 | -314.6 |
| `safe-exposure-f3` | 0.7598 | 0.5976 | 0.5306 | 573.4 | 466.9 | 677.5 | 9.49 | -97.0 | -300.5 |
| `safe-exposure-f2-lowfanout` | 0.7226 | 0.5795 | 0.5011 | 609.4 | 511.8 | 677.5 | 9.48 | -60.9 | -255.5 |
| `safe-exposure-f1-strong` | 0.7624 | 0.6037 | 0.5383 | 559.4 | 446.9 | 677.5 | 9.47 | -110.9 | -320.5 |

## Miss Taxonomy

| Variant | Covered | Student Encoder Loss | Semantic Recovered | Native Traversal Loss | Hard Miss | Semantic-Heavy Student Loss | Long-Query Student Loss |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `beironly-baseline` | 1275 | 37 | 8 | 5 | 14 | 31 | 26 |
| `focus6` | 1280 | 30 | 7 | 4 | 16 | 26 | 20 |
| `listwise-budget-f3` | 1278 | 37 | 3 | 2 | 15 | 33 | 24 |
| `coverage-budget-f3` | 1280 | 32 | 9 | 1 | 16 | 27 | 21 |
| `safe-exposure-f3` | 1280 | 32 | 5 | 3 | 15 | 30 | 20 |
| `safe-exposure-f2-lowfanout` | 1278 | 35 | 7 | 3 | 16 | 31 | 23 |
| `safe-exposure-f1-strong` | 1281 | 34 | 3 | 5 | 16 | 31 | 20 |

## Promotion Screen

Candidate promotion requires no NDCG/MAP regression, no material active8 candidate/posting increase, and no increase in `student_encoder_loss` relative to `beironly-baseline`. Reference rows are shown for comparison only.

| Variant | Decision | Reason Signal |
| --- | --- | --- |
| `beironly-baseline` | `reference` | NDCG +0.0000, MAP +0.0000, candidates +0.0, SAE posts +0.0, student loss 37 |
| `focus6` | `reference` | NDCG +0.0035, MAP +0.0017, candidates +97.8, SAE posts +42.3, student loss 30 |
| `listwise-budget-f3` | `cost-only` | NDCG -0.0039, MAP -0.0077, candidates -65.1, SAE posts -266.7, student loss 37 |
| `coverage-budget-f3` | `cost-only` | NDCG -0.0008, MAP -0.0041, candidates -105.1, SAE posts -314.6, student loss 32 |
| `safe-exposure-f3` | `cost-only` | NDCG -0.0010, MAP -0.0033, candidates -97.0, SAE posts -300.5, student loss 32 |
| `safe-exposure-f2-lowfanout` | `cost-only` | NDCG -0.0063, MAP -0.0122, candidates -60.9, SAE posts -255.5, student loss 35 |
| `safe-exposure-f1-strong` | `cost-only` | NDCG -0.0021, MAP -0.0047, candidates -110.9, SAE posts -320.5, student loss 34 |

## Best Route Signal

- Best ranking signal: `focus6`.
- Best physical-cost signal: `safe-exposure-f1-strong`.
- No new focused/listwise/coverage/exposure candidate passes the promotion screen. `focus6` remains the best ranking signal, but it is a previous diagnostic/reference and increases active8 candidate/posting cost.
- The cost-oriented objectives reliably reduce candidate docs and SAE postings, but they do so by removing useful semantic coverage. This makes them useful as boundary evidence, not as the next checkpoint.
- This focused-objective direction is exhausted for now. The next useful training work should change the representation or teacher signal, rather than adding more pressure to the same coverage/exposure losses.

## Raw Artifacts

- Stable copied report root: `/Volumes/Betty/Tmp/ii42_sae_reports/m22-focused-objective-sweep`
- Tracked report: `sae-milestone22-focused-objective-sweep-report.md`
