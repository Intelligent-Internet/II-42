# SAE M98 Stage-B Ranking Calibration Results Report

Date: 2026-05-22

## Decision

M98A is the first positive Stage-B ranking result after M97. It uses the full M81 candidate surface, freezes the promoted `m96_k512` Stage-A atoms, and trains only a runtime-safe BM25+SAE scoring calibrator.

The result is positive: calibrated BM25+SAE beats fixed BM25+SAE and BM25+dense on validation, holdout, and combined eval. This means Stage-B ranking adjustment is worth continuing before product engineering.

This is still not product readiness. The model is a scoring calibrator over the existing candidate surface, not a full-corpus native retrieval path.

## Inputs

- Run root: `/home/huoju/leask/runs/m98-stage-b-ranking-calibration-v0`
- Candidate surface: `/home/huoju/leask/data/ii42_sae_m81/large-stage-a-v0-stable`
- Checkpoint: `/home/huoju/leask/runs/m96-k512-checkpoint-interp-v0/checkpoints/interp_alpha_0.35.pt`
- Rows: `4806`
- Candidate documents: `77089`
- Queries: `4806`

| Split | Rows |
| --- | ---: |
| train | 3920 |
| validation | 486 |
| holdout | 400 |
| eval = validation + holdout | 886 |

## Model

M98A keeps dense out of runtime:

```text
score = BM25_norm
      + query_scale(runtime_features) * SAE_norm
      + cross_weight * BM25_norm * SAE_norm
```

Dense is used only during training as the `BM25+dense` teacher/control. Runtime features are query/candidate distribution features derived from BM25 and SAE scores, such as concentration and entropy.

Best checkpoint selection:

- `best_epoch`: `70`
- `best_validation_score`: `0.917484`
- `teacher_weight`: `0.35`
- `dense_teacher_weight`: `2.0`

The validation curve peaked around epoch 70 and then drifted down slightly, which confirms that best-validation selection is necessary.

## Main Matrix

| Split | Run | Recall@10 | MRR | NDCG@10 |
| --- | --- | ---: | ---: | ---: |
| validation | `BM25+dense w2` | 0.3360 | 0.6642 | 0.5050 |
| validation | `BM25+SAE w2` | 0.3435 | 0.6680 | 0.5119 |
| validation | `M98 calibrated` | 0.3436 | 0.6765 | 0.5150 |
| holdout | `BM25+dense w2` | 0.3735 | 0.6617 | 0.5242 |
| holdout | `BM25+SAE w2` | 0.3721 | 0.6611 | 0.5247 |
| holdout | `M98 calibrated` | 0.3817 | 0.6687 | 0.5304 |
| eval | `BM25+dense w2` | 0.3530 | 0.6631 | 0.5137 |
| eval | `BM25+SAE w2` | 0.3564 | 0.6649 | 0.5177 |
| eval | `M98 calibrated` | 0.3608 | 0.6729 | 0.5219 |

## Eval By Family

| Family | Run | Recall@10 | MRR | NDCG@10 |
| --- | --- | ---: | ---: | ---: |
| `beir15_current_eval_surface` | `BM25+dense w2` | 0.5875 | 0.7512 | 0.7094 |
| `beir15_current_eval_surface` | `BM25+SAE w2` | 0.5889 | 0.7375 | 0.7010 |
| `beir15_current_eval_surface` | `M98 calibrated` | 0.6017 | 0.7409 | 0.7104 |
| `broad_generated_query_surface` | `BM25+dense w2` | 0.2779 | 0.6450 | 0.4679 |
| `broad_generated_query_surface` | `BM25+SAE w2` | 0.2783 | 0.6364 | 0.4628 |
| `broad_generated_query_surface` | `M98 calibrated` | 0.2807 | 0.6433 | 0.4651 |
| `large_supervised_split_surface` | `BM25+dense w2` | 0.9259 | 0.7600 | 0.7937 |
| `large_supervised_split_surface` | `BM25+SAE w2` | 0.9683 | 0.9030 | 0.9206 |
| `large_supervised_split_surface` | `M98 calibrated` | 0.9841 | 0.9320 | 0.9407 |

Interpretation:

- M98 calibrated closes the BEIR NDCG gap against BM25+dense and beats BM25+dense on Recall@10.
- Broad generated queries still have a small NDCG/MRR gap versus BM25+dense, but calibrated BM25+SAE improves over fixed BM25+SAE.
- Large supervised remains the strongest SAE advantage.

## What This Means

M98A confirms the route:

```text
Stage A: preserve/compress representation -> M96 k512
Stage B: ranking-aware BM25+SAE calibration/training -> M98 positive
```

The next model step should not return to Stage-A compression. It should deepen Stage-B:

- train a stronger query-side ranking model over the same larger surface;
- add pairwise/listwise losses for broad-query gaps;
- keep BM25+dense as a teacher/control, not runtime dependency;
- keep validation/holdout/family reporting as the promotion gate.

## Remaining Blockers

- This is candidate-surface ranking, not full-corpus retrieval.
- Full-root physical execution still needs fanout-aware candidate control.
- M20 full-root payload build still needs streaming/postings-driven work.
- Broad generated query NDCG remains slightly below BM25+dense.

