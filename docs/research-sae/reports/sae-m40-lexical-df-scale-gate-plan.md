# SAE M40 Lexical-DF Scale Gate Plan

Status: executed.

## Motivation

M39 showed a useful but non-promotable signal:

```text
lower SAE scale improves broad trec-covid queries
global lower SAE scale hurts full15 and other hard datasets
```

M40 therefore treats the next step as a query-type-aware calibration problem,
not as a larger model or another synthetic-volume run.

## Hypothesis

Runtime-safe lexical DF features can identify broad high-DF queries well enough
to adjust semantic intervention strength:

```text
query text
-> BM25 token posting DF features
-> choose lower SAE scale for broad queries
-> keep higher SAE scale for ordinary semantic queries
```

This should recover some `trec-covid` quality without applying the `w0p25`
penalty globally.

## Implementation

M40 extends the M31 trainer with opt-in features:

```text
--calibration-feature-mode lexical_df
--enable-df-gate
--broad-scale-guard-weight
--broad-sae-scale-target
```

The learned calibration head can receive:

```text
query length
BM25 score concentration
query atom entropy
query top atom mass
predicted SAE fanout
content token mean DF
content token high-DF share
```

The lexical DF features are runtime-safe because a sparse index can compute
them from BM25 posting lengths for query tokens.

M40 also adds `scripts/research_sae_m40_df_gate_sweep.py`, an evaluation-only
sweep over:

```text
content mean DF threshold
low broad-query SAE weight
high ordinary-query SAE weight
```

This separates two questions:

- can training with lexical DF features produce better query atoms?
- can a runtime-safe gate combine the M39 broad-query signal with normal-query
  quality?

## Training Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m31_joint_final_ranking_train.py \
  --train-data-root /Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded \
  --eval-data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
  --train-datasets dbpedia-entity fever fiqa hotpotqa msmarco nfcorpus quora scifact m39-trec-covid-broad-neighborhood \
  --output-dir results/sae/m40/lexical-df-gate-train-eval-current \
  --device mps \
  --learning-rate 1.0e-4 \
  --head-learning-rate 8.0e-4 \
  --epochs 4 \
  --teacher-loss-weight 2.0 \
  --qrel-loss-weight 0.10 \
  --bm25-preserve-weight 0.35 \
  --fanout-loss-weight 0.03 \
  --scale-prior-weight 0.05 \
  --calibration-feature-mode lexical_df \
  --enable-df-gate \
  --broad-scale-guard-weight 0.20 \
  --broad-sae-scale-target 0.25 \
  --collapse-aware-selection
```

## Gate Sweep Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m40_df_gate_sweep.py \
  --run-root results/sae/m40/lexical-df-gate-train-eval-current \
  --output-dir results/sae/m40/df-gate-sweep
```

## Decision Rule

M40 is not expected to pass the full no-collapse product gate immediately. It
is useful if it forms a clearer Pareto point than M39:

| Gate | Requirement |
| --- | --- |
| TREC | improve `trec-covid` MAP versus M36/M39 `w0p5` |
| Aggregate | keep full15 NDCG/MAP near or above M36 |
| Isolation | broad gate should mostly affect `trec-covid`, not `msmarco` |
| Cost | track SAE postings and candidate docs separately |
