# SAE M43 Canonical Payload Allocation Plan

Status: completed; see `sae-m43-canonical-payload-allocation-results-report.md`.

## Summary

M42 showed that the strongest quality-cost point is not a new utility-loss
checkpoint. It is the existing M40 query atom pool with a better physical
payload selector:

```text
selected_score = query_atom_weight / df(atom)^alpha
```

M43 turns that sweep-only result into a canonical query-latent export path.

## Goals

1. Add payload allocation to the reusable M31/M40/M42 trainer/evaluator.
2. Preserve default behavior when allocation args are not provided.
3. Reproduce the M42 sweep result exactly from the integrated path.
4. Separate lexical feature construction from runtime gate thresholds.

## Key Design

The export path now has two stages:

```text
support-logit top-k pool
-> value / posting_df^fanout_power selection
-> exported query payload
```

This matters because the model can emit a richer latent atom pool, while the
runtime layer chooses the atoms that are worth touching physically.

## New Arguments

```text
--export-active-dims
--export-pool-active-dims
--export-fanout-power
--content-high-df-feature-threshold
```

Defaults preserve prior behavior:

- `export_active_dims=0` means use checkpoint `student_active_dims`;
- `export_pool_active_dims=0` means use checkpoint `student_active_dims`;
- `export_fanout_power=0.0` means no fanout discount;
- `content_high_df_feature_threshold=0.10` matches the metadata semantics used
  by M40/M42.

## Canonical M43 Profile

The current best profile is:

```text
checkpoint = results/sae/m40/lexical-df-gate-train-eval-current/m31_joint_final_ranking_student.pt
export_active_dims = 48
export_pool_active_dims = 64
export_fanout_power = 0.5
content_high_df_feature_threshold = 0.10
df_gate_content_mean_df_threshold = 0.15
df_gate_content_high_share_threshold = 0.25
df_gate_low_sae_weight = 0.35
df_gate_high_sae_weight = 0.75
```

## Acceptance Criteria

M43 is accepted if the integrated path exactly reproduces the M42 sweep row:

```text
active=48
fanout_power=0.5
threshold=0.15
low=0.35
high=0.75
```

The expected metrics are:

```text
Recall@100 0.866596
MRR@20     0.899967
NDCG@10    0.788874
MAP@100    0.759661
SAE postings 1388.685
```
