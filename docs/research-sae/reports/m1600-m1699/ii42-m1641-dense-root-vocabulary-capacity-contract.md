# M1641 Dense-Root Vocabulary Capacity Contract

## Question

Can the frozen P1 dense root be compiled into one practical vocabulary-sparse
posting vector before retrieval supervision is introduced?

M1641 does not train another selector, query router, centroid codebook, or
ranking adapter. It first tests whether the missing interface itself has
measurable capacity:

```text
text -> frozen BGE pooled vector -> geometry-preserving vocabulary bridge
     -> calibrated non-negative vocabulary impacts -> one inverted index
```

## Evidence Behind The Change

Vocabulary Transfer (`2607.00004`) identifies vocabulary compatibility rather
than backbone capacity as a structural sparse-retrieval bottleneck. Its useful
components for this project are semantic initialization, prior alignment, and
activation-potential calibration. Its ablation also warns that head-only
adaptation is weaker than aligning the complete input/output vocabulary.

Dense-to-sparse projection with probabilistic expansion control
(`2402.17535`) independently shows that a frozen dense encoder can seed a
sparse lexical head, but naive projection creates dimension co-activation and
semantic deviation. The projection therefore needs a measured initialization
and activation regime before ranking training.

The M1640 OpenSearch control (`2411.04403`) shows that a normalized WordPiece
output, heterogeneous teacher, IDF-aware objective, and mature SPLADE shape can
produce useful exact inverted retrieval. It is a mechanism control, not the
final P1 encoder.

## Why This Does Not Repeat Prior Work

- M1520 clustered contextual BGE token states into an independent 32K
  codebook. M1641 uses BGE's own normalized WordPiece vocabulary and embedding
  geometry.
- M1530 trained token-level codebook projections against a cross-encoder
  surface that a factorized query/document model could not realize. M1641
  starts from the final pooled dense vector and does not use ranking labels in
  its capacity audit.
- M1540 used reconstruction-trained SAE latent terms. M1641 uses a
  representation-compatible vocabulary and an explicit activation-cost gate.
- M1630 used signed PCA coordinates whose global block engine touched nearly
  all work. M1641 must inherit the exact M1640 engine gate before training.

## S0: External Engine Prerequisite

M1641 is authorized only if the fixed Apache-2.0 OpenSearch control passes the
complete FiQA engine gate:

- exact block top100 parity is 1.0;
- bitset word/full is at most 0.30;
- decoded/full is at most 0.60;
- no post-hoc TopK, DF cutoff, or approximate admission is used.

Failure closes M1641 training. It would show that even the established output
shape does not satisfy this project's exact-engine requirement.

## S1: Qrels-Free Capacity Audit

Use the pinned `BAAI/bge-base-en-v1.5` dense root and its existing
`bert-base-uncased`-compatible 30,522-token vocabulary. The backbone remains
frozen.

Two declared representations answer one causal question:

1. `identity`: project the normalized pooled dense vector directly onto the
   normalized input-embedding vocabulary;
2. `procrustes`: fit one orthogonal map from pooled vectors to corpus lexical
   centroids, then use the same vocabulary projection.

The orthogonal map is fit from text alone on the existing MS MARCO corpus
sample. It preserves dense pairwise dot products before sparsification. No
qrels, BM25 score, BEIR row, or teacher ranking enters this fit.

Apply one corpus-calibrated scalar bias after projection. The fixed engineering
target is mean document nonzeros `224`, selected before evaluation from the
two mature M1640 controls. Query activation is not separately calibrated.
Special tokens are excluded.

Report on locked shared FiQA, NFCorpus, and SciFact:

- exact sparse NDCG@10, MAP@100, Recall@100, MRR@20, and CUB;
- dense overlap at 10/100/256;
- Pearson and Spearman agreement on dense top256 scores;
- mean and p95 query/document nonzeros, active vocabulary, maximum DF, and
  touched-document ratios;
- exact block parity, decoded/full, bitset word/full, and metadata bytes;
- lexical fidelity: input-token recall among active vocabulary impacts.

### S1 Gate

Authorize trainable M1641B only if `procrustes`:

- improves identity overlap@100 by at least `0.05` absolute on at least two of
  three rows;
- reaches overlap@100 at least `0.25` and positive dense-score Spearman on at
  least two rows;
- retains at least `50%` of dense NDCG@10 and Recall@100 on at least two rows;
- has no dead-vocabulary or near-universal-DF collapse;
- preserves exact block top100 parity.

These are capacity gates, not product promotion gates. Failure means that a
small learned head is not supported by the frozen representation and stops the
route before expensive training.

## S2: Limited Training If Authorized

M1641B keeps the backbone frozen and trains only the orthogonal/low-rank bridge
and vocabulary bias on a large qrels-free corpus-derived sample.

The objective is staged rather than mixed from step zero:

1. preserve dense candidate distributions and pairwise margins;
2. use a Bernoulli expansion curriculum to prevent co-activation collapse;
3. add corpus DF/IDF cost only after dense agreement is measurable.

Checkpoint selection is conjunctive: dense overlap and score agreement cannot
fall while retrieval fit or sparsity improves. BEIR qrels remain evaluation
only. BM25, residual rescue, and final ranking optimization are outside M1641.

Only after a frozen-backbone checkpoint passes shared3 may one low-learning-
rate final-layer continuation be considered. End-to-end backbone training is
not authorized by this contract.

## Stop Rules

Stop without another variant when:

- the M1640 complete-corpus exact engine fails;
- semantic alignment does not improve the identity capacity surface;
- useful overlap requires near-universal posting touch;
- a checkpoint lowers training loss but violates any dense-agreement floor;
- the proposed repair is another threshold, target-nnz, TopK, loss-weight, or
  selector grid;
- gains require BEIR-specific or dataset-specific choices.

## Literature Cutoff

The evidence search used II-Commons arXiv coverage through `2026-07-10`.
Primary references are `2607.00004`, `2402.17535`, `2411.04403`,
`2109.10086`, `2403.06789`, `2401.06703`, and `2505.15070`.
