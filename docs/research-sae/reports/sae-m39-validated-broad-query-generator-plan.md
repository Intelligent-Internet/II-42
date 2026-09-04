# SAE M39 Validated Broad-Query Generator Plan

Status: executed.

## Motivation

M36 showed a distribution gap: current evaluation has many hard
`broad_high_df + many_positive` queries, while the clean M32 training surface
has very few. M38 then showed that simply upweighting the current training
surface does not solve this.

M39 therefore tests a different hypothesis:

```text
validated broad-query generation
+ teacher-neighborhood pseudo labels
  -> better broad-query query-side atoms
```

This is not a weight sweep. The goal is to change the training distribution
itself.

## Generator Design

M39 adds `scripts/research_sae_m39_broad_query_generator.py`.

The generator differs from M35 in one important way:

```text
M35 selected very high-DF phrases.
M39 selects phrases whose content DF matches the audited hard-query target.
```

The audited `trec-covid` hard-query target is:

| Metric | Target |
| --- | ---: |
| Mean content DF | 0.1959 |
| Long-query share | 0.8163 |
| Duplicate-like rate | 0.0204 |
| Top-term share | 0.1037 |

M39 controls:

- target content mean DF;
- min/max content mean DF;
- token-signature diversity;
- projected top-term dominance;
- teacher qrel depth.

Generated labels are still teacher-neighborhood pseudo labels, not quality
evidence:

```text
quality_claim_allowed = false
```

## Artifact Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m39_broad_query_generator.py \
  --artifact m39-broad-neighborhood-q200 \
  --datasets trec-covid \
  --queries-per-dataset 200 \
  --teacher-qrels-k 100 \
  --device mps
```

Artifact root:

```text
/Volumes/Betty/Tmp/ii42_sae_m39_broad_query_generator/m39-broad-neighborhood-q200
```

## Validation Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m36_synthetic_query_validation.py \
  --synthetic-root m39=/Volumes/Betty/Tmp/ii42_sae_m39_broad_query_generator/m39-broad-neighborhood-q200 \
  --output-dir results/sae/m39/broad-query-validation
```

Validation must pass before training. If it fails, M39 should tune generator
selection, not train on unsafe synthetic data.

## Training Command

M39 is merged with the M36 expanded real-query training root:

```text
/Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded
```

Training command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m31_joint_final_ranking_train.py \
  --train-data-root /Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded \
  --eval-data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
  --train-datasets dbpedia-entity fever fiqa hotpotqa msmarco nfcorpus quora scifact m39-trec-covid-broad-neighborhood \
  --output-dir results/sae/m39/broad-query-distill-train-eval-current \
  --device mps \
  --learning-rate 1.0e-4 \
  --head-learning-rate 8.0e-4 \
  --epochs 4 \
  --teacher-loss-weight 2.0 \
  --qrel-loss-weight 0.10 \
  --bm25-preserve-weight 0.35 \
  --fanout-loss-weight 0.03 \
  --scale-prior-weight 0.05 \
  --collapse-aware-selection
```

Hard-bucket reweighting is intentionally off. This isolates the value of the
new query distribution.

## Decision Rule

M39 is promoted only if it improves `trec-covid` without causing aggregate or
other hard-dataset regressions.

Partial positive signal is allowed, but it must be reported as partial. In
particular, a lower fixed SAE weight improving `trec-covid` while hurting
other datasets is evidence for scale/calibration work, not a promotable model.
