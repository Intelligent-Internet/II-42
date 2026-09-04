# SAE Text-To-Sparse-Atoms Five-Step Training Report

Date: 2026-05-14

## Summary

This pass tested the five training directions identified after the first T5
generalization pass:

1. candidate-coverage training;
2. query/document hard negatives from both BM25 and dense near-misses;
3. teacher top-k coverage loss instead of static listwise score matching;
4. a stronger transformer text encoder;
5. larger cross-domain teacher data readiness.

The result is mixed. The new coverage objective is mechanically valid and
stronger than the failed listwise score-distribution loss, but it still does not
beat the current best frontier. The strongest current training result remains
the earlier dense-teacher pair run for first-page metrics, while retrieval-aware
token-char remains best for Recall@100.

## Implemented Training Controls

Added to:

```text
scripts/research_sae_text_atom_train.py
```

Coverage objective:

```text
--coverage-teacher-loss-weight
--coverage-teacher-positive-k
--coverage-teacher-bm25-negative-k
--coverage-teacher-dense-negative-k
--coverage-teacher-batch-size
--coverage-teacher-margin
```

The coverage examples treat dense top-k documents as the positive set and mix
two kinds of hard negatives:

- BM25 high-ranking documents outside qrels and dense positives;
- dense near-miss documents after the positive cutoff.

The loss pushes every dense-teacher positive above the negative pool:

```text
mean_pos softplus(margin + logsumexp(negative_scores) - positive_score)
```

This is closer to candidate generation than the previous listwise softmax
distribution because the positive top-k documents are all coverage targets.

Stronger encoder controls:

```text
--encoder-mode mean|transformer
--transformer-layers
--transformer-heads
--dropout
```

The default remains `mean`, so existing runs and baselines are unchanged.

## Matrix

All rows below are sampled five-dataset BEIR means.

| Run | Best/Eval Weight | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `t3-retrieval-aware-token-char` | 0.750 | 0.7735 | 0.6483 | 0.5549 | 0.4539 |
| `t3-dense-teacher-token-char` | 0.750 | 0.7611 | 0.6642 | 0.5713 | 0.4684 |
| `t3-listwise-token-char` | 0.500 | 0.7527 | 0.6408 | 0.5473 | 0.4498 |
| `t4-coverage-token-char` | 0.750 | 0.7540 | 0.6433 | 0.5551 | 0.4522 |
| `t4-dense-coverage-token-char` | 0.500 | 0.7591 | 0.6472 | 0.5659 | 0.4606 |
| `t4-transformer-dense-coverage-token-char` | 0.500 | 0.6949 | 0.5758 | 0.4918 | 0.4055 |

## Step Results

### 1. Candidate-Coverage Training

Run:

```text
results/sae/text-atoms/t4-coverage-token-char/
```

Result:

```text
Recall@100 = 0.7540
MRR@20     = 0.6433
NDCG@10    = 0.5551
MAP@100    = 0.4522
```

This beats the failed listwise distribution run on MRR/NDCG/MAP and is close to
the earlier retrieval-aware run on NDCG/MAP. It does not recover the
retrieval-aware Recall@100 frontier.

### 2. Coverage-Aware Hard Negatives

Run:

```text
results/sae/text-atoms/t4-dense-coverage-token-char/
```

This combines dense-teacher pair supervision with a weak coverage regularizer.
The best swept student weight was `0.5`:

```text
Recall@100 = 0.7591
MRR@20     = 0.6472
NDCG@10    = 0.5659
MAP@100    = 0.4606
```

This is the strongest T4 coverage result, but it still trails the dense-pair
run on MRR/NDCG/MAP and trails retrieval-aware token-char on Recall@100.

### 3. Teacher Top-K Coverage Loss

The top-k coverage loss is useful as an objective shape, but the current
implementation does not yet improve the frontier. It likely needs a more native
candidate-budget approximation:

```text
selected query atoms
  -> opened candidate docs under a fixed postings budget
  -> coverage of dense-teacher positives
```

The current differentiable pairwise approximation still scores all candidate
documents in the batch, so it does not fully train the query-side atom selection
policy that the physical index needs.

### 4. Stronger Encoder

Run:

```text
results/sae/text-atoms/t4-transformer-dense-coverage-token-char/
```

Result:

```text
Recall@100 = 0.6949
MRR@20     = 0.5758
NDCG@10    = 0.4918
MAP@100    = 0.4055
```

A small transformer trained from scratch is worse than the mean encoder and
even below BM25. The likely cause is not that sequence modeling is useless; it
is that this setup lacks pretrained retrieval-language features and needs more
epochs/data. The next stronger-encoder attempt should use a pretrained encoder
or a SPLADE-style backbone, not a tiny randomly initialized transformer.

### 5. Larger Cross-Domain Teacher Data

Local available quality-matrix data currently contains only:

```text
scifact
scidocs
nfcorpus
arguana
fiqa
```

No local arxiv/pubmed/policy teacher snapshots with qrels or proxy qrels were
available in this pass. This step remains data-preparation blocked. It should
not be reported as a quality result until we have defensible product-query
labels or dense-teacher proxy labels for those corpora.

## Decision

Do not promote coverage training as the new default yet.

Current best choices remain:

- use `t3-retrieval-aware-token-char` when optimizing Recall@100;
- use `t3-dense-teacher-token-char` when optimizing first-page ranking;
- keep T4 coverage code as opt-in research infrastructure.

The next promising training direction is not another static loss-weight sweep.
It should implement a closer approximation of the physical candidate generator:

```text
query text
  -> query atom scores
  -> top atom budget
  -> opened document set from atom postings
  -> coverage loss against dense-teacher positives
```

That would train the actual query atom selection policy instead of only training
document-pair scores after all candidate documents are already present.
