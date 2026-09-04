# SAE M43 Canonical Payload Allocation Results Report

Status: closed as the current canonical quality-cost profile.

## Summary

M43 successfully moved M42's utility/fanout-aware payload allocation from a
sweep script into the reusable query-latent export/evaluation path.

The integrated M43 path exactly reproduces the M42 sweep row:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M42 exact prune | 0.866596 | 0.899967 | 0.788874 | 0.759661 | 0.459507 | 2845.310 | 1388.685 |
| M43 integrated | 0.866596 | 0.899967 | 0.788874 | 0.759661 | 0.459507 | 2845.310 | 1388.685 |

Compared with M40 baseline:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M40 sweep | 0.867007 | 0.898518 | 0.788710 | 0.759559 | 0.470121 | 2948.465 | 2734.691 |
| M43 integrated | 0.866596 | 0.899967 | 0.788874 | 0.759661 | 0.459507 | 2845.310 | 1388.685 |

M43 keeps M40-level ranking quality and cuts SAE postings by roughly `49%`.

## Implementation

M43 updates:

```text
scripts/research_sae_m31_joint_final_ranking_train.py
```

New canonical export arguments:

```text
--export-active-dims
--export-pool-active-dims
--export-fanout-power
--content-high-df-feature-threshold
```

The query-latent export path now does:

```text
support-logit top-k pool
-> rank pool atoms by value / posting_df^fanout_power
-> export active query atoms
```

This matches the M42 sweep policy but runs inside the normal evaluation path.

## Important Fix: Threshold Semantics

M43 found and fixed a semantic issue in the research path.

M40/M42 query metadata used:

```text
content_high_df_feature_threshold = 0.10
```

The DF gate sweep then varied:

```text
df_gate_content_mean_df_threshold = 0.12 or 0.15
```

Before M43, the integrated path reused the gate threshold when recomputing
`content_high_df_share`, which changed broad-query classification and caused
integrated evaluation to diverge from the sweep. M43 separates these concepts:

- `content_high_df_feature_threshold` constructs runtime-safe lexical features;
- `df_gate_content_mean_df_threshold` controls the runtime gate.

After the split, M43 reproduces M42 exactly.

## Reproduction Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m31_joint_final_ranking_train.py \
  --checkpoint results/sae/m40/lexical-df-gate-train-eval-current/m31_joint_final_ranking_student.pt \
  --train-data-root /Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded \
  --eval-data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
  --train-datasets dbpedia-entity fever fiqa hotpotqa msmarco nfcorpus quora scifact m39-trec-covid-broad-neighborhood \
  --output-dir results/sae/m43/m40-export-active48-fp0p5-thr0p15-eval-current \
  --device mps \
  --epochs 0 \
  --calibration-feature-mode lexical_df \
  --enable-df-gate \
  --df-gate-low-sae-weight 0.35 \
  --df-gate-high-sae-weight 0.75 \
  --df-gate-content-mean-df-threshold 0.15 \
  --df-gate-content-high-share-threshold 0.25 \
  --content-high-df-feature-threshold 0.10 \
  --export-active-dims 48 \
  --export-fanout-power 0.5 \
  --collapse-aware-selection
```

## Decision

M43 replaces M42 sweep as the canonical research profile:

```text
M40-trained query atom pool
+ M43 utility/fanout-aware export allocation
+ M40 lexical-DF score gate
```

This still is not a dense-removal product gate pass because hard-dataset
collapses remain. But it is now the strongest physical quality-cost profile
and should be the baseline for the next training round.

## Next Direction

M44 should train for this export regime directly:

1. keep M43 export selection fixed in evaluation;
2. train a larger atom pool, for example pool 96 -> export 48;
3. add model selection on quality minus normalized postings;
4. verify whether training can improve `trec-covid` without losing the M43
   cost profile.
