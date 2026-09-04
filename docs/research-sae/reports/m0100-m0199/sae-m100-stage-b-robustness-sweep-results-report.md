# SAE M100 Stage-B Robustness Sweep Report

Date: 2026-05-22

## Decision

M100 promotes the M99 candidate-level residual direction as robust enough to
become the current Stage-B mainline. The sweep shows that the improvement is
not a single-seed accident:

- `baseline_seed123` reaches eval Recall@10/MRR/NDCG@10
  `0.4036/0.6901/0.5594`, essentially matching or slightly improving M99's
  eval NDCG.
- `no_broad_weight` reaches `0.4025/0.6940/0.5570`, so broad-family
  reweighting is useful but not required for the core gain.
- `conservative_residual` reaches `0.3949/0.6796/0.5479`, weaker but still
  clearly above M98.

This is still candidate-surface ranking, not full-corpus retrieval or SQL/API
product readiness.

M100 checks whether the M99 candidate-level residual ranker is robust 
across seed and conservative-capacity variants.

## Summary Matrix

| Variant | Best Epoch | Eval Recall@10 | Eval MRR | Eval NDCG@10 | Holdout NDCG@10 | Broad MRR | Broad NDCG@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline_seed123` | 210 | 0.4036 | 0.6901 | 0.5594 | 0.5715 | 0.6627 | 0.5084 |
| `conservative_residual` | 120 | 0.3949 | 0.6796 | 0.5479 | 0.5575 | 0.6517 | 0.4960 |
| `no_broad_weight` | 210 | 0.4025 | 0.6940 | 0.5570 | 0.5708 | 0.6660 | 0.5050 |

## Controls

| Run | Eval Recall@10 | Eval MRR | Eval NDCG@10 |
| --- | ---: | ---: | ---: |
| `bm25_dense_w2` | 0.3530 | 0.6631 | 0.5137 |
| `bm25_sae_w2` | 0.3564 | 0.6649 | 0.5177 |
| `m98_calibrated` | 0.3608 | 0.6729 | 0.5219 |

## Decision Notes

- This sweep is still candidate-surface ranking.
- Dense scores remain training/control signals only.
- If multiple variants stay above M98 on holdout and broad-generated 
  metrics, M99 is robust enough to become the Stage-B mainline.
