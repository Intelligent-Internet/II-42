# SAE Milestone 3 Fusion Training Report

Date: 2026-05-12

## Purpose

Milestone 2.5 selected the current systems route:

```text
source-blind evidence atoms
+ impact-head candidate generation
+ exact rerank over the candidate pool
```

This milestone pushes two next-step questions:

1. technical fusion: what does a read-only native-like evidence-atom head
   payload need to store, and how does the quality/cost profile look?
2. training exploration: can pseudo-query-trained mixed token-latent atoms
   improve candidate recall or become a direct scoring signal?

Runner:

```text
scripts/research_sae_milestone3_fusion_training.py
```

Main artifacts:

```text
results/sae/milestone3/fusion-training/summary.md
results/sae/milestone3/fusion-training/milestone3_fusion_training.json
```

Broader mixed-atom sanity artifacts:

```text
results/sae/milestone3/fusion-training-broad/summary.md
results/sae/milestone3/fusion-training-broad/milestone3_fusion_training.json
```

## Main Configuration

```text
datasets = scifact, scidocs, nfcorpus, arguana, fiqa
run_name = sae_8192_64
pseudo_train_limit = 4000
negative_samples = 32
max_mixed_atoms = 8192
mixed_min_df = 4
mixed_target_df = 16
mixed_cost_alpha = 0.75
```

The available pseudo-query artifacts are currently about `2000` rows per
dataset, so this run consumes the current available pseudo-query scale.

## Mean Quality Matrix

| Run | Recall@100 | MRR@20 | Postings touched | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| `base_head8` | 0.7856 | 0.6791 | 705.4 | 516.8 |
| `base_head16` | 0.7964 | 0.6790 | 1381.7 | 852.9 |
| `base_head32` | 0.7948 | 0.6790 | 2652.3 | 1268.1 |
| `mixed_candidate_base_score_head8` | 0.7875 | 0.6791 | 751.6 | 518.7 |
| `mixed_expand_candidate_base_score_head8` | 0.7879 | 0.6790 | 779.4 | 525.4 |
| `mixed_candidate_base_score_head16` | 0.7964 | 0.6790 | 1439.5 | 853.5 |
| `mixed_expand_candidate_base_score_head16` | 0.7965 | 0.6790 | 1467.3 | 857.3 |
| `mixed_score_head16` | 0.7945 | 0.5657 | 1439.5 | 853.5 |
| `mixed_expand_score_head16` | 0.7942 | 0.3609 | 1467.3 | 857.3 |
| `mixed_suppress_score_head16` | 0.7917 | 0.5641 | 1439.5 | 853.5 |

## Technical Fusion Result

The native-shaped payload proxy confirms that `base_head16` remains the first
implementation target:

| Payload | Atoms | Head pairs | Full pairs | Bytes estimate |
| --- | ---: | ---: | ---: | ---: |
| `base_head8` | 20499.0 | 71121.0 | 349644.4 | 3858099.2 |
| `base_head16` | 20499.0 | 103786.6 | 349644.4 | 4119424.0 |
| `base_head32` | 20499.0 | 148904.4 | 349644.4 | 4480366.4 |
| `mixed_head16` | 23645.6 | 121328.6 | 367391.8 | 4477257.6 |

These byte estimates are layout proxies, not final storage claims. They model
the parts a read-only PostgreSQL/native payload needs:

```text
atom directory
+ impact-head candidate pairs
+ doc-side sparse pairs for exact candidate rerank
```

The important result is not the absolute bytes. The important result is the
contract:

```text
collect top impact postings per query atom
-> union candidate doc ids
-> exact rerank with base evidence atoms
```

This is the route to port into the native/read-only payload first. It keeps
the final ranking stable and makes the semantic SAE signal behave like a
sparse inverted-index workload.

## Mixed Atom Training Result

The pseudo-query mixed atom trainer produced real trained dictionaries:

| Dataset | Pseudo examples | Positive atoms | Selected mixed atoms | Selected DF mean | Selected DF p95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 2000 | 169672 | 3086 | 5.47 | 10.0 |
| `scidocs` | 2000 | 139344 | 2580 | 5.63 | 10.0 |
| `nfcorpus` | 2063 | 152448 | 4275 | 6.20 | 12.0 |
| `arguana` | 2000 | 142248 | 4054 | 5.83 | 11.0 |
| `fiqa` | 1998 | 108928 | 1790 | 6.20 | 13.0 |

This confirms the trainer is selecting relatively narrow token-latent atoms.
That is good for fanout, but not sufficient for recall.

## Key Finding: Mixed Atoms Are Not A Final Score Signal Yet

Direct mixed-atom scoring is unsafe:

```text
base_head16 MRR@20          = 0.6790
mixed_score_head16 MRR@20   = 0.5657
mixed_expand_score_head16   = 0.3609
```

The failure mode is calibration. The trained mixed atoms carry useful
candidate evidence, but their score scale is not comparable to token and SAE
base atoms. Suppressing parent SAE atoms does not fix this.

Decision:

```text
Do not put mixed atom scores into the final ranker yet.
```

## Safe Use: Candidate-Only Mixed Expansion

Mixed atoms are safe when they are only used to widen the candidate set and the
final score remains the base evidence-atom score:

```text
base_head8 Recall@100                         = 0.7856
mixed_candidate_base_score_head8 Recall@100   = 0.7875
mixed_expand_candidate_base_score_head8       = 0.7879
```

This is a small gain, not enough to replace `base_head16`, but it is useful as
a low-latency profile:

```text
base_head16:
  Recall@100       0.7964
  postings touched 1381.7

mixed_expand_candidate_base_score_head8:
  Recall@100       0.7879
  postings touched 779.4
```

Interpretation: mixed atom expansion can partially recover head8 recall while
remaining much cheaper than head16. It should be treated as an optional
candidate booster, not as the main scoring model.

## Broader Mixed Atom Sanity Check

A broader run used:

```text
mixed_min_df = 8
mixed_target_df = 64
mixed_cost_alpha = 0.25
mixed_query_atoms = 64
mixed_doc_atoms = 32
```

Summary:

| Run | Recall@100 | MRR@20 | Postings touched | Candidate docs |
| --- | ---: | ---: | ---: | ---: |
| `mixed_expand_candidate_base_score_head8` default | 0.7879 | 0.6790 | 779.4 | 525.4 |
| `mixed_expand_candidate_base_score_head8` broad | 0.7877 | 0.6791 | 757.1 | 521.5 |
| `mixed_expand_candidate_base_score_head16` default | 0.7965 | 0.6790 | 1467.3 | 857.3 |
| `mixed_expand_candidate_base_score_head16` broad | 0.7965 | 0.6790 | 1453.5 | 855.4 |

The broader setting is slightly cheaper but does not improve quality. It also
selects far fewer atoms:

```text
default selected mixed atoms mean = 3157.0
default selected DF mean          = 5.87

broad selected mixed atoms mean   = 535.6
broad selected DF mean            = 10.70
```

Decision: keep the default trainer for diagnostics. Do not spend the next
iteration on hand-tuning mixed atom DF thresholds.

## Updated Direction

### Native/SQL Integration

Move forward with the base evidence-atom head payload:

```text
base evidence atoms
+ impact-head candidate directory
+ exact sparse rerank over candidate docs
```

Do not require mixed atoms for the first native integration. The base route is
already strong and simpler.

### Training Exploration

The next training attempt should not keep adding score weights to fixed mixed
atoms. The more promising route is:

```text
train or generate retrieval atoms directly
+ explicit candidate-budget/fanout objective
+ final score calibrated back to base evidence atoms
```

In concrete terms, the next learner should optimize candidate coverage and
calibration separately:

- candidate objective: recover relevant docs missed by `head8` or `head16`;
- fanout objective: avoid broad SAE-like postings;
- calibration objective: do not let learned atoms dominate first-page ranking;
- integration objective: learned atoms may first be candidate-only.

## Decision

Milestone 3 closes with:

```text
Mainline:
  implement source-blind base evidence-atom impact-head payload.

Optional:
  keep mixed token-latent atoms as candidate-only booster experiments.

Do not:
  use mixed atom scores directly in final ranking yet.

Next training:
  retrieval-aware atom generation with candidate-budget and calibration losses,
  not more manual threshold tuning over current mixed atoms.
```
