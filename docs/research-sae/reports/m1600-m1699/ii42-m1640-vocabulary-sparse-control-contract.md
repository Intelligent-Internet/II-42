# M1640 Vocabulary-Sparse Control Contract

## Hypothesis

M1630 failed because signed PCA coordinates have the wrong posting/bound
distribution, not because inverted retrieval is impossible.  M1640 tests an
established learned-sparse representation before authorizing any new model
training.

The frozen external control is:

```text
naver/splade-v3-distilbert
revision 2db06b86d65e316e2ca9907aa1aa8be6f8c4e739
text -> MLM vocabulary impacts -> max/log1p pooling
     -> one 30,522-dimensional non-negative posting index
```

The checkpoint is from SPLADE-v3 and reports MS MARCO MRR@10 `0.387` and
BEIR-13 mean NDCG@10 `0.500`.  It is an external mechanism control, not a
clean-heldout project result and not the proposed final encoder.

## Why This Is New

M1510/M1518 used Gemma SAE latent features, not tokenizer vocabulary impacts.
M1520/M1530 used an independently constructed 32K BGE centroid vocabulary.
M1540 used reconstruction-trained latent terms.  All three sources lacked the
structural vocabulary/normalization compatibility of a retrieval-trained
SPLADE model.

M1640A asks whether the mature output shape fixes the exact failure observed
in M1630 before attempting dense-root distillation.

## Stage A: Shared Canary

Run frozen SPLADE-v3-DistilBERT on shared FiQA first, then NFCorpus and
SciFact only if the first row is healthy.  Report:

- exact inverted-index NDCG@10, MAP@100, Recall@100, MRR@20, and CUB;
- BGE dense overlap at 10 and 100;
- query/document nonzeros, total postings, maximum DF, and mean touched docs;
- exact block64 parity, decoded/full, bitset word/full, opened blocks, and
  metadata bytes.

Qrels are evaluation-only.  No TopK, DF cutoff, threshold, model choice, or
score scale may be selected from BEIR results.

### Shared Gate

Authorize a full-corpus FiQA engine control only when:

- exact block top100 parity is 1.0;
- NDCG@10 and Recall@100 each retain at least 85% of BGE dense;
- bitset word/full is at most 0.30;
- rankings are not underfilled.

Small-corpus decoded/full is diagnostic because top100 is 5% of 2,000 docs.

## Stage B: Full FiQA Engine Control

If Stage A passes, evaluate the fixed checkpoint on the complete 57,638-doc
FiQA corpus and compare directly with the historical M1015 surface.

Promotion requires:

- exact top100 parity for every query;
- decoded/full at most 0.60;
- bitset word/full at most 0.30;
- no post-hoc sparse pruning or approximate candidate admission.

## Independent Product-Compatible Control

If the Naver control is mechanism-positive but misses one shared quality row,
one independent Apache-2.0 control is allowed before closing the family:

```text
opensearch-project/opensearch-neural-sparse-encoding-v2-distill
revision 269e6638b2c4f648996691f6d751495285d8f330
```

This checkpoint is selected before evaluation because its published method
uses a heterogeneous dense plus sparse ensemble teacher and reports BEIR-13
mean NDCG@10 `0.528`.  It uses the same 30,522-token output and receives the
same gate.  No third checkpoint, model ranking, or BEIR-based model selection
is allowed.

If both public roots fail the shared gate, stop external-model testing.  The
only later training hypothesis may be Vocabulary Transfer-style semantic
initialization and activation calibration of the project's dense root; it
must begin with a capacity/initialization audit rather than a full run.

## Stage C: Dense-Root Training Decision

If the independent Apache control passes shared3 and complete-corpus FiQA,
M1640 closes as a positive external mechanism control. It authorizes M1641's
qrels-free dense-root vocabulary capacity audit, not immediate model training.
M1641 must test semantic initialization, activation calibration, dense
agreement, and exact-engine cost before any trainable continuation.

## Stop Rules

Stop without training when:

- the public SPLADE control lacks local retrieval quality;
- exact engine cost remains comparable to full accumulation;
- useful quality requires near-universal query/document support;
- the proposed repair is a threshold, block-size, TopK, or loss-weight grid;
- the next model would reproduce M1510, M1520, M1530, or M1540 rather than
  change the demonstrated failure mechanism.

## Evidence

- SPLADE v2: `arXiv:2109.10086`;
- SPLADE-v3: `arXiv:2403.06789`;
- SPLADE efficiency: `arXiv:2207.03834`;
- corpus-specific vocabulary: `arXiv:2401.06703`;
- Block-Max Pruning: `arXiv:2405.01117`;
- DF-FLOPS: `arXiv:2505.15070`.
