# II-42 M506b Candidate-Support Head

## Purpose

M506 showed that a learned candidate/support head plus a fixed structural
dense-tail scorer is stronger than a single learned posting scorer. M506b tests
that split directly.

Training objective:

```text
PPLX dense -> candidate/support coordinates
```

The structural dense-tail scorer stays fixed. Training uses no BM25 and no
qrels; qrels remain evaluation-only.

## Implementation

Script:

- `scripts/research_sae_m506b_candidate_support_head.py`

Artifacts:

- `outputs/m506b/fiqa_prefix256_gate/`
- `outputs/m506b/fiqa_query_only_prefix256_gate/`
- `outputs/m506b/fiqa_score_membership_prefix256_gate/`
- `outputs/m506b/fiqa_score_membership_prefix384_gate/`
- `outputs/m506b/broad4_score_membership_prefix256_gate/`

Key design choices:

- initialize from the deterministic PCA structural rotation;
- train support coordinates with dense TopK membership and dense score
  distillation;
- evaluate with real posting heads and the fixed structural dense-tail scorer.

## FiQA Ablations

### Membership-Only, Prefix 256

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.54526 | 1.00000 | 1.00000 | 1.00000 |
| `m506b_rotation_linear_support_structural_score` | 0.48926 | 0.72672 | 0.76219 | 0.39114 |
| `m506b_rotation_residual_support_structural_score` | 0.48748 | 0.72758 | 0.76398 | 0.39091 |
| `m506b_structural_compiler` | 0.40225 | 0.53414 | 0.54453 | 0.38542 |

Membership-only helps, but it does not match the M506 score-distilled support
signal.

### Query-Only Support, Prefix 256

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.54526 | 1.00000 | 1.00000 | 1.00000 |
| `m506b_structural_compiler` | 0.40225 | 0.53414 | 0.54453 | 0.38542 |
| `m506b_query_rotation_linear_support_structural_score` | 0.37265 | 0.42398 | 0.42820 | 0.35089 |

Query-only adaptation is rejected for now. Keeping document postings fixed
removes useful flexibility and lowers candidate recall.

### Score + Membership, Prefix 256

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.54526 | 1.00000 | 1.00000 | 1.00000 |
| `m506b_rotation_residual_support_structural_score` | 0.51325 | 0.78117 | 0.83641 | 0.37831 |
| `m506b_rotation_linear_support_structural_score` | 0.50877 | 0.78328 | 0.83867 | 0.37813 |
| `m506b_structural_compiler` | 0.40225 | 0.53414 | 0.54453 | 0.38542 |

This is the first clean M506b win. It improves quality, dense overlap, and
candidate recall while slightly reducing touched documents versus the structural
compiler.

### Score + Membership, Prefix 384

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.54526 | 1.00000 | 1.00000 | 1.00000 |
| `m506b_rotation_residual_support_structural_score` | 0.52483 | 0.82984 | 0.91023 | 0.49673 |
| `m506b_structural_compiler` | 0.44734 | 0.67508 | 0.70148 | 0.51112 |

Prefix 384 closes more of the dense gap, but touch rises to roughly 50%. This
is useful as an upper operating point, not yet the efficiency target.

## Broad4 Sample

Tasks: ArguAna, FiQA2018, SCIDOCS, TRECCOVID.

Prefix 256, score + membership, `rotation_residual`.

| Source | NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.50301 | 1.00000 | 1.00000 | 1.00000 |
| `m506b_rotation_residual_support_structural_score` | 0.46254 | 0.72215 | 0.77424 | 0.54764 |
| `m506b_structural_compiler` | 0.42930 | 0.61583 | 0.65372 | 0.55497 |

Per-task:

| Task | Teacher NDCG@10 | M506b NDCG@10 | Dense O@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| ArguAna | 0.46113 | 0.44032 | 0.93094 | 0.99984 | 0.96978 |
| FiQA2018 | 0.51013 | 0.47542 | 0.77531 | 0.82781 | 0.38964 |
| SCIDOCS | 0.25129 | 0.25238 | 0.84156 | 0.92812 | 0.67334 |
| TRECCOVID | 0.78947 | 0.68205 | 0.34080 | 0.34120 | 0.15779 |

TRECCOVID remains the visible blocker: low touched ratio keeps candidate recall
too low. This points to query-adaptive fanout rather than a scorer redesign.

## Decision

Promote:

- `rotation_residual` support head;
- combined dense-score + TopK membership support loss;
- fixed structural dense-tail scorer;
- prefix 256 as the current efficiency point and prefix 384 as the higher
  quality point.

Reject for now:

- membership-only support loss;
- query-only support adaptation;
- learned scorer replacement;
- early RL/ranking-aware optimization.

## Next Step

M506c should target candidate coverage under explicit fanout control:

1. Add query-adaptive prefix/fanout using qrels-free signals such as support
   entropy, score margin, candidate count, and structural/dense-tail agreement.
2. Add a support false-negative loss that upweights dense Top100 documents not
   reached by the structural compiler.
3. Run broad10 sampled after FiQA/TRECCOVID both improve.
4. Only after broad10 support coverage is stable, consider ranking-aware or
   hybrid/BM25-aware fine tuning.

The route is still alive. The dense root is preserved; the remaining problem is
candidate coverage at product-like fanout, not semantic representation.
