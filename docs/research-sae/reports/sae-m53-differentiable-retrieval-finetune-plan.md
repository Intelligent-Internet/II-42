# SAE M53 Differentiable Retrieval Fine-Tuning Plan

## Summary

M53 resets the next training step around a more precise objective:

```text
large-corpus teacher imitation checkpoint
-> low-LR retrieval fine-tuning
-> hard exported atoms
-> unified BM25+SAE sparse ranking
```

The goal is not to replace M52 semantic pretraining. M52 still builds the
base `text -> atoms` semantic memory. M53 adds a small, controlled
retrieval fine-tuning stage that tests whether ranking quality improves
when the deployed top-k behavior is represented during training.

## Why This Change

The `diffsae` experiment highlights three useful principles:

1. TopK selection can be replaced by a differentiable soft surrogate during
   training while keeping hard TopK at inference.
2. Retrieval tasks with multiple valid positives should not be trained as
   single-positive InfoNCE. Unchosen positives must not become negatives.
3. When a teacher embedding geometry is useful, distillation should remain
   as an anchor, but task supervision should drive the retrieval behavior.

Our previous M29-M52 results point in the same direction. Pure atom imitation
improves semantic fidelity, but does not fully close the ranking gap on hard
datasets. Pure qrel/ranking pressure can over-promote high-impact atoms and
damage robustness. M53 therefore combines both signals explicitly.

## Training Flow

### Stage A: semantic teacher imitation

Continue using M52:

- broad corpus: BEIR non-heldout text, Wikipedia, arXiv, PubMed, and product
  corpora when available;
- objective: teacher atom support/value imitation;
- output: a stable checkpoint with good teacher atom fidelity;
- no product quality claim from this stage alone.

### Stage B: retrieval fine-tuning

Start from the Stage-A checkpoint and fine-tune on a small supervised shard.

Losses:

- teacher anchor: sampled support BCE plus value regression;
- support ranking: teacher atoms must outrank tempting inactive atoms;
- multi-positive coverage: qrels and dense-teacher positives are trained as
  a set;
- soft top-k recall: candidate-level top-k is made differentiable during
  training, but runtime still exports hard atoms;
- candidate-budget ranking: query atoms are scored under the same small
  posting-opening budget used by the unified sparse engine;
- asymmetric active ranking: query/doc active budgets match the runtime
  profile;
- safe exposure penalty: penalize exposure only outside qrels and
  dense-teacher safe neighborhoods.

The fine-tuning learning rate must be lower than Stage A. The aim is
calibration, not relearning the encoder from scratch.

## Small-Data Smoke

Canonical first run:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m53_refined_flow.py \
    --datasets scifact nfcorpus \
    --epochs 2 \
    --device mps \
    --execute
```

This runs:

1. baseline checkpoint evaluation;
2. refined fine-tuning with teacher anchor, multi-positive soft top-k recall,
   candidate-budget loss, asymmetric active loss, and safe exposure penalty.

The default checkpoint is the current 8192/token-lse M22-style checkpoint so
the smoke can run without waiting for M52F/M52E. Once M52 selects a better
semantic-pretraining checkpoint, pass it with `--checkpoint`.

## Promotion Gate

Promote to larger training only if the small run satisfies all of the
following:

- at least one supervised dataset improves NDCG@10 or MAP@100 versus the
  baseline checkpoint without a larger regression on the other dataset;
- support overlap does not collapse materially;
- SAE postings and candidate docs remain in the same physical cost regime;
- the best student weight does not require an extreme one-off setting.

If the smoke fails, do not scale. The next adjustment should be loss balance
or data construction, not more epochs.

## Scale-Up Path

If the smoke passes:

1. run the same flow on `scifact nfcorpus fiqa scidocs`;
2. add hard datasets `trec-covid msmarco dbpedia-entity`;
3. rerun full15 quality and physical matrices;
4. only then use Spark for larger Stage-B fine-tuning from the selected M52
   Stage-A checkpoint.

## Current Implementation

- `scripts/research_sae_text_atom_train.py` now supports
  `--soft-topk-recall-loss-weight`.
- `scripts/research_sae_m53_refined_flow.py` runs the small-data baseline
  evaluation and refined fine-tuning recipe.

The soft TopK loss is train-only. Exported query/doc atoms and runtime sparse
ranking remain hard and unchanged.
