# M1690 Paper-Native Sparse Foundation Contract

## Objective

Build a scientifically controlled learned-sparse foundation that can later
absorb dense-root retrieval knowledge while deploying as one encoder and one
inverted index.

M1690 does not start from a random output head and does not continue M1680 VT.
It starts from the fixed Apache-2.0 OpenSearch vocabulary-sparse root already
validated by M1640, then compares stored cross-score distillation with the
published dense+sparse ensemble-distillation mechanism at meaningful data
scale. This is a controlled composition of two author-supported training modes,
not a claim that the authors published this exact paired experiment.

Deployment remains:

```text
text -> one learned-sparse encoder -> one 30,522-term posting map
     -> one exact BMP/native inverted index
```

Dense and sparse teachers are training-only. No ANN field, external BM25
candidate union, post-hoc reranker, or dataset-specific policy is permitted.

## Stable Evidence Base

- [SPLADE-v3](https://arxiv.org/abs/2403.06789) establishes mature
  distillation, hard-negative, and FLOPS training for vocabulary-sparse
  retrieval.
- [OpenSearch inference-free sparse retrieval](https://arxiv.org/abs/2411.04403)
  establishes per-candidate score normalization, heterogeneous dense+sparse
  ensemble distillation, and IDF-aware sparse training.
- [Vocabulary Transfer](https://arxiv.org/abs/2607.00004) shows vocabulary
  normalization is structural, but M1680-M1682 show that our transferred
  ModernBERT interface is not yet a reliable retrieval foundation.
- [Effective LSR](https://arxiv.org/abs/2505.01452) shows modern engines can
  support relaxed sparsity, so model quality must not be destroyed to satisfy
  a Python proxy.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) links model regularization to
  production posting-list cost.
- [Beyond Hard Negatives](https://arxiv.org/abs/2604.04734) shows that
  preserving the teacher score spectrum is more robust than repeatedly mining
  only top hard negatives.
- [LACONIC](https://arxiv.org/abs/2601.01684) provides strong evidence that
  learned-sparse capacity depends on a two-stage curriculum: broad weak-pair
  pre-finetuning followed by curated hard-negative finetuning. A short KD smoke
  cannot by itself falsify that larger curriculum.
- [DiSCo](https://arxiv.org/abs/2410.14609) supports distilling candidate
  similarity scores instead of forcing representation identity, and provides
  an independent multi-teacher sparse-retrieval precedent. Its conversational
  setting does not authorize copying its data recipe into ad-hoc retrieval.
- [Scaling Sparse and Dense Retrieval](https://arxiv.org/abs/2502.15526)
  reports that KD alone gains little from model scaling, while combined
  contrastive and KD training is the scalable sparse route. Therefore a failed
  paired-KD gate authorizes no deeper KD continuation; at most it authorizes
  one separately contracted broad contrastive-curriculum control.
- [Probabilistic Expansion Control](https://arxiv.org/abs/2402.17535) shows
  that a frozen dense root plus a learned sparse projection can work when
  expansion is introduced through a curriculum. Its multimodal evidence is a
  structural fallback, not evidence for changing M1690 mid-run.
- [BMP](https://arxiv.org/abs/2405.01117) and M1660 provide the exact engine
  and latency gate.

The author-provided OpenSearch training repository is pinned at commit
`7cbe00469c09f3b2e233c28f307f653cdf96ecb0`. The Naver SPLADE reference is
pinned at `8dcd33a054d790e74aceda25b128c1b188c5d9c1`.

## Fixed Artifacts

- Student and sparse anchor:
  `opensearch-project/opensearch-neural-sparse-encoding-v2-distill`, revision
  `269e6638b2c4f648996691f6d751495285d8f330`.
- Dense ensemble teacher: `Alibaba-NLP/gte-large-en-v1.5`, revision
  `104333d6af6f97649377c2afbde10a7704870c7b`, matching the pinned author
  configuration. Its remote model implementation is separately pinned as
  `Alibaba-NLP/new-impl` revision
  `40ced75c3017eb27626c9d4ea981bde21a2662f4`.
- Sparse ensemble teacher:
  `opensearch-project/opensearch-neural-sparse-encoding-v1`, revision
  `708f7e68f4aada15c2d59144a5187c521da6d8db`, matching the pinned author
  configuration.
- Training data: `opensearch-project/msmarco-hard-negatives`, revision
  `e93a36373924962ea09aef0b5e3ae553c042d19b`.
- Query/document text: `BeIR/msmarco`, revision
  `a918e0d11a77ed33f42f29d98340b655593b96ad`.
- Data size: 502,939 rows, two Parquet shards, 917,108,315 compressed bytes.
- Final engine: the audited official BMP source and patch from M1660.

BEIR qrels are forbidden in representation training, checkpoint selection,
and cost-policy selection.

## S0: Immutable Data And Recipe Audit

1. Download the two exact Parquet shards to Betty and record byte size and
   SHA-256.
2. Verify that the hard-negative source contains MS MARCO identifiers, then
   materialize them through the exact author-provided preparation contract and
   the pinned BEIR query/corpus revision. Identifier-only rows cannot authorize
   teacher inference.
3. Make deterministic query-disjoint train, validation, and smoke manifests.
4. Verify every row contains one query, at least two candidates, finite scores,
   and a non-constant teacher distribution.
5. Record candidate-count and score-range quantiles before training.
6. Verify the local loss, pooling, FLOPS ramp, and teacher normalization
   formulas against the two pinned repositories.

S0 fails on artifact drift, malformed rows, train/validation query overlap, or
an unpinned model/data revision.

## S1: Step-Zero Identity And Teacher Observability

Use 8,192 train and 2,048 disjoint validation rows only for the mechanism
smoke. Evaluate the frozen root on all stored candidates, not a first-three or
top-three subset.

Report:

- top-1 agreement with the stored cross-encoder teacher;
- pairwise ordering agreement, row Spearman, and candidate-set KL against the
  stored cross-encoder score distribution;
- dense/sparse teacher agreement, sparse-root error recovery, and collateral
  errors measured against the same stored cross-encoder reference;
- query/document nonzeros, FLOPS, maximum batch DF, and active vocabulary;
- exact step-zero score identity between both paired branches.

The source contains KD scores but no explicit positive label. Therefore no
positive-at-index-zero metric or InfoNCE claim is valid on this surface. The
ensemble branch is authorized only if the normalized dense+sparse teacher has
net-positive heldout agreement with the stored cross-encoder reference, and
either improves top-1 agreement or recovers at least 5% of sparse-root top-1
errors while creating no more new errors than it recovers. This repeats
M1650's causal requirement on the full candidate surface.

## S2A: Paired Optimization-Identifiability Gate

Before the 100,000-row run, precompute and hash both teacher targets for the
8,192 materialized training rows with the locked `K=8` score-spectrum source.
Run `stored_cross_kd` and `ensemble_kd` from the same v2-distill bytes for
2,000 steps, evaluating at steps 0, 500, 1,000, and 2,000 on all 2,048 heldout
rows and all 100 candidates.

Before either branch starts, run one forward/backward-only memory canary with
the locked batch and sequence lengths. It must produce finite loss and
gradients while keeping peak reserved CUDA memory below 85% of device memory.
The canary must not update or save model weights.

S2A authorizes S2 only when a trained checkpoint improves heldout
stored-reference KL by at least 2% and improves Spearman, does not regress
top-1 or pairwise ordering, and keeps document nonzeros, FLOPS, and max DF
within 1.15 times step zero.
The ensemble branch must beat the stored-score branch on at least two ordering
metrics without losing another. The stored-score branch is a completed causal
control and is not required to pass the promotion gate. Its diagnostic
comparator is the trained evaluation with lowest heldout stored-reference KL,
even when that checkpoint is unsafe. The ensemble branch itself must pass every
joint checkpoint gate. Training-loss reduction alone is a stop.

S2A is an optimization-identifiability test, not a retrieval result. Do not run
BEIR or promote a checkpoint from this surface.

## S2: Paired 10K-Step Training Gate

Both branches start from the identical root, use the same 100,000 deterministic
training rows, 5,000 disjoint validation rows, candidate source, optimizer,
effective batch, absolute FLOPS ramp, and seed. Training candidates are a
deterministic score-spectrum subset selected from each 100-candidate source
row. Lock `K=8`, the paper's default, using eight evenly spaced quantile
anchors over the stored teacher scores. Do not run a candidate-count grid.
Evaluation always uses all 100 heldout candidates.

- `stored_cross_kd`: the author-provided `data_type: kd` path with the stored
  cross-encoder scores and official KLDiv objective.
- `ensemble_kd`: the paper-native per-row min-max normalized BGE dense plus
  frozen sparse-root teacher, scale 30, and KLDiv.

Use the official absolute schedule. Do not compress the FLOPS ramp. Evaluate
steps 0, 1,000, 5,000, and 10,000 on every heldout candidate.

A branch passes only if:

- KL against the stored cross-encoder distribution improves by at least 5%;
- pairwise ordering and top-1 agreement with that reference do not regress;
- row Spearman against that reference improves;
- query and document representations remain non-degenerate;
- document nonzeros, FLOPS, and max DF remain at most 1.15 times the root;
- selected checkpoint is trained and passes every gate jointly.

The ensemble branch must additionally dominate `stored_cross_kd` on at least
two of heldout stored-reference KL, pairwise agreement, and top-1 agreement
without losing the third. Comparing each branch only with its own training
teacher is forbidden. One metric improvement purchased by worse ordering is
not a pass.

## S3: Native Shared3 Gate

Only a conjunctive S2 pass authorizes exact sparse retrieval on locked shared
NFCorpus, SciFact, and FiQA. Compare the selected model with the frozen sparse
root and BGE dense. Use the official BMP engine, not the old fixed-block Python
proxy.

Promotion requires:

- no row loses more than 0.01 NDCG@10 or Recall@100 from the sparse root;
- macro NDCG@10, MAP@100, Recall@100, and MRR@20 do not regress;
- dense overlap@100 improves by at least 0.01 macro;
- BMP exact top100 parity is 1.0;
- measured latency and index bytes remain within 1.15 times the root.

## S4: Scale Ladder

Scale only after each previous gate passes:

1. 50K steps on all 502,939 rows;
2. shared15/native evaluation with a frozen checkpoint;
3. 100K official completion only if broader rows remain safe;
4. add IDF-aware or DF-FLOPS in one paired correction only when measured BMP
   anatomy identifies high-DF posting lists as the remaining bottleneck.

There is no direct jump from a 10K optimization signal to official BEIR15 or
product promotion.

## Stop Conditions

Stop this route when any condition occurs:

- the large-data ensemble does not beat the standard paired control;
- training loss improves while full-candidate ordering does not;
- only query/document scale asymmetry creates the gain;
- sparse cost requires post-hoc TopK, threshold, or dataset-specific pruning;
- shared3 gains disappear through the official BMP/native path;
- the next proposed change is a teacher-weight, temperature, lambda, negative,
  or threshold grid rather than a measured failure mechanism.

If S2 fails, retain the unmodified OpenSearch root plus M1660 BMP as the final
single-index learned-sparse baseline and stop this KD continuation. A failure
with an observable teacher does not falsify the separate LACONIC-style broad
pre-finetuning curriculum; it authorizes at most one later data-curriculum
probe, not more loss, temperature, or threshold variants. If S2 passes but S3
fails, conclude that MS MARCO transfer does not generalize and do not scale
training depth.
