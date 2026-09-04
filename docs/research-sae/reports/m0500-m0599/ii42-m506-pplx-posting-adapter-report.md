# II-42 M506 PPLX Posting Adapter Gate

## Purpose

M506 tests the first real transfer step after M505:

```text
official/materialized PPLX dense surface -> indexable posting surface
```

Training uses no BM25 and no qrels. Qrels are used only for held-out reporting.
The gate asks whether posting conversion can keep dense-teacher neighborhoods
before we attempt text-to-posting, LoRA, RL, or ranking-aware tuning.

## Implementation

Script:

- `scripts/research_sae_m506_pplx_posting_adapter.py`

Artifacts:

- `outputs/m506/smoke_fiqa_linear/`
- `outputs/m506/smoke_fiqa_linear_cross/`
- `outputs/m506/fiqa_linear_mlp_gate/`
- `outputs/m506/fiqa_linear_prefix256_gate/`
- `outputs/m506/fiqa_linear_prefix384_gate/`
- `outputs/m506/broad4_linear_prefix256_gate/`

Teacher surface:

- `row_int8` PPLX dense output, validated in M505 as the correct official
  model-side surface.

Evaluated sources:

- `exact_materialized_dense`
- `teacher_row_int8_dense`
- `m506_structural_compiler`
- `m506_linear_adapter`
- `m506_linear_candidates_structural_score`
- `m506_structural_candidates_linear_score`
- MLP variants in the FiQA gate

The important cross-route is:

```text
linear learned candidate/support head -> structural dense-tail scorer
```

## Key Results

### FiQA Smoke, Prefix 128

64 held-out queries, 64 train groups, 2 epochs.

| Source | NDCG@10 | R@100 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.47040 | 0.80960 | 1.00000 | 1.00000 | 1.00000 |
| `m506_linear_candidates_structural_score` | 0.39842 | 0.68557 | 0.66984 | 0.69484 | 0.22812 |
| `m506_structural_compiler` | 0.28474 | 0.39658 | 0.30469 | 0.30547 | 0.22357 |
| `m506_linear_adapter` | 0.11728 | 0.54356 | 0.31422 | 0.69484 | 0.22812 |

Interpretation: the learned linear head is useful for candidate support, but
bad as a scorer. The structural scorer is dense-faithful, but its candidate
head is weak. Separating candidate generation from scoring is the right move.

### FiQA Linear/MLP Gate, Prefix 128

128 held-out queries, 128 train groups, 4 epochs.

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.51134 | 1.00000 | 1.00000 | 1.00000 |
| `m506_linear_candidates_structural_score` | 0.45710 | 0.69008 | 0.71695 | 0.22157 |
| `m506_structural_compiler` | 0.33318 | 0.31586 | 0.31719 | 0.22373 |
| `m506_linear_adapter` | 0.20127 | 0.38250 | 0.71695 | 0.22157 |
| `m506_mlp_candidates_structural_score` | 0.09850 | 0.09820 | 0.09844 | 0.13867 |

MLP is rejected for this configuration. It under-learns support and destroys
the candidate gate.

### FiQA Prefix Sweep

Same 128-query/128-group gate, linear candidate head, structural scorer.

| Prefix | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| ---: | ---: | ---: | ---: | ---: |
| 128 | 0.45710 | 0.69008 | 0.71695 | 0.22157 |
| 256 | 0.48516 | 0.82656 | 0.90188 | 0.37445 |
| 384 | 0.48943 | 0.86180 | 0.96234 | 0.48823 |

Prefix 256 is the current efficiency point. Prefix 384 gains only 0.00427
NDCG@10 while increasing touched documents from 37.4% to 48.8%.

### Broad4 Sample, Prefix 256

Tasks: ArguAna, FiQA2018, SCIDOCS, TRECCOVID.

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.49871 | 1.00000 | 1.00000 | 1.00000 |
| `m506_linear_candidates_structural_score` | 0.45643 | 0.71888 | 0.78130 | 0.52382 |
| `m506_structural_compiler` | 0.42366 | 0.62315 | 0.66192 | 0.55517 |
| `m506_linear_adapter` | 0.12841 | 0.31444 | 0.78130 | 0.52382 |

Per-task signal:

| Task | Teacher NDCG@10 | Cross NDCG@10 | Cross Dense O@100 | Cross Candidate R@100 | Cross Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| ArguAna | 0.50561 | 0.49134 | 0.92656 | 0.99953 | 0.92460 |
| FiQA2018 | 0.47040 | 0.44714 | 0.82359 | 0.88797 | 0.37285 |
| SCIDOCS | 0.27895 | 0.25737 | 0.86297 | 0.97250 | 0.62857 |
| TRECCOVID | 0.73987 | 0.62988 | 0.26240 | 0.26520 | 0.16927 |

TRECCOVID exposes the remaining blocker: the support head is not uniformly
strong across corpora. The scoring side is not the first bottleneck there;
candidate coverage is.

## Decision

M506 confirms the transfer route is viable, but not as a single learned posting
scorer.

Promote:

- learned linear candidate/support head;
- structural dense-tail scorer;
- prefix 256 as the current FiQA efficiency point.

Reject for now:

- learned linear scorer as the main scorer;
- MLP adapter with the current objective;
- early RL/ranking-aware tuning before support coverage is stable.

## Next Step

M506b should train candidate support directly:

1. Keep the structural dense-tail scorer fixed.
2. Train the candidate/support head against dense teacher TopK membership,
   structural false negatives, and fanout/load balance.
3. Optimize candidate recall under a fixed touched-document budget, not direct
   NDCG.
4. Add query-adaptive prefix/fanout after the fixed-budget gate passes.
5. Re-run broad10 sampled; only then promote to full task matrices.

The right target is now clear: improve candidate coverage while preserving the
dense-faithful scorer. This is different from the earlier failed
student-encoder attempts, which tried to learn semantics, support, scoring, and
fanout in one mixed objective.
