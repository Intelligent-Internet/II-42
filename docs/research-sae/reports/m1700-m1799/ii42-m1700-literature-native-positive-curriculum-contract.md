# M1700 Literature-Native Positive Curriculum Contract

## Objective

Train one vocabulary-sparse encoder for one native inverted index by restoring
the positive-pair retrieval curriculum that is absent from M1690/M1691.

M1700 does not add an ANN field, BM25 candidate union, reranker, post-hoc TopK,
or dataset-specific policy. It starts from the fixed OpenSearch sparse root
validated by M1660 and changes the training curriculum, not the product shape:

```text
text -> one vocabulary-sparse encoder -> one 30,522-term posting map
     -> one exact BMP/native inverted index
```

## Why This Is A New Route

M1690/M1691 use eight score-spectrum candidates per query but have no explicit
positive label. Under the repaired author optimizer schedule, both teachers can
improve full-candidate quality. The stored-score branch passes every quality
gate at step 1,000 but reaches `1.469x` document max DF. The ensemble branch at
step 1,000 also passes KL, top1, pairwise, and Spearman gates but reaches
`1.339x` maximum sparse cost. A safe step-500 ensemble checkpoint keeps cost at
`1.098x` but regresses pairwise and Spearman ordering.

This is evidence for a quality/cost Pareto conflict on the no-positive KD
surface. It is not evidence that a mature sparse foundation cannot learn
retrieval. The next intervention must alter supervision and data scale, not LR,
temperature, candidate count, threshold, or ordinary FLOPS coefficients.

## Literature Basis

- [LACONIC](https://arxiv.org/abs/2601.01684) first trains on 9.26 million
  weak positive pairs with in-batch negatives, then finetunes on 648K curated
  positive/hard-negative examples. It reports that this two-phase curriculum is
  central to adapting a sparse retriever.
- [Scaling Sparse and Dense Retrieval](https://arxiv.org/abs/2502.15526)
  shows that KD alone scales poorly, while contrastive plus KD training gives
  the best sparse effectiveness/generalization trade-off.
- [Beyond Hard Negatives](https://arxiv.org/abs/2604.04734) performs
  contrastive adaptation before score-spectrum KD and includes one explicit
  positive in every KD candidate set. M1691 reproduces neither condition.
- The pinned OpenSearch implementation provides a positive/negative InfoNCE
  path for the same vocabulary-sparse model family.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) explains why ordinary FLOPS can
  leave a small set of terms with long posting lists. Its experiment uses
  explicit positives, in-batch negatives, seven hard negatives, 50K steps, and
  periodically refreshed corpus DF estimates. It is therefore a later
  curriculum component, not an M1691 post-hoc rescue.
- [SPLADE-v3](https://arxiv.org/abs/2403.06789) and
  [BMP](https://arxiv.org/abs/2405.01117) provide the mature training and exact
  engine references.

This is a controlled composition of published mechanisms. It is not a claim
that any author published this exact root/data/engine combination.

## Fixed Artifacts

- Initial model:
  `opensearch-project/opensearch-neural-sparse-encoding-v2-distill` at
  `269e6638b2c4f648996691f6d751495285d8f330`.
- Broad weak pairs: `utahnlp/nomic-embed-pretrain-lite` at
  `dfea387946f33796beee9b71caef61e091cfa2bf`, 9,259,451 rows and
  13,478,134,157 compressed bytes.
- Curated hard negatives: `rlhn/rlhn-680K` at
  `d5323fad7aea636e633be28722ad15aafa6cbd54`, 648,766 rows and
  6,447,294,919 compressed bytes.
- Strict hard-negative training initially uses only the RLHN
  `msmarco_passage` subset. BEIR-overlapping subsets are excluded from model
  selection.
- Candidate teachers, if S3 is authorized, remain the pinned M1690 dense and
  sparse teachers.
- Engine: the patched official BMP source and quantization contract from
  M1660.

BEIR qrels are forbidden in training, source filtering, checkpoint selection,
DF calibration, and sparsity-policy selection.

## S0: Data And Leakage Audit

1. Record repository revisions, file sizes, LFS object hashes, schemas,
   licenses, and source counts.
2. Build deterministic query-disjoint train/validation manifests. Sample across
   all source shards rather than taking a contiguous prefix.
3. Hash normalized query and document text against every locked shared15
   evaluation query. Remove exact query overlap and report document overlap;
   do not silently filter by dataset score.
4. Retain source/subset provenance wherever available. Report results on
   seen-domain and strict heldout-domain rows separately.
5. Run a forward/backward batch-128 memory canary at query length 64 and
   document length 192. Peak reserved memory must remain below 85% with finite
   loss and gradients.

Failure of provenance, query disjointness, or the resource canary stops before
training.

## S0D: Signal Observability Gate

The batch-128 resource canary is not a training authorization. The mature
sparse root obtains positive top1 `1.0` and InfoNCE below `2e-5` on that weak
pair batch, so a batch-128 broad update would be nearly gradient-free.

Before S1, run one fixed inference-only comparison:

- four deterministic broad validation windows at negative-pool sizes 128,
  512, and 2,048;
- 1,024 deterministic RLHN `msmarco_passage` rows with one positive and 15
  cleaned hard negatives;
- positive top1, MRR, InfoNCE, positive-to-hardest-negative margin, pairwise
  accuracy, and the share of rows with a negative at least 95% of the positive
  root score.

The broad route is informative when the 2,048 pool has top1 below `0.995` or
InfoNCE at least `0.01`. If broad pairs remain saturated but RLHN meets the
same top1/loss condition or has pairwise accuracy below `0.999`, skip broad
weak-pair training and authorize only an RLHN mechanism canary. If both
surfaces are saturated, stop M1700 before training.

Large-pool training, when authorized, must use an exact GradCache-equivalent
update. Ordinary gradient accumulation is not equivalent because it does not
expose the same in-batch negatives. This gate compares mechanisms; it is not a
batch-size or threshold grid.

## S1: Positive-Pair Mechanism Gate

Use 102,400 deterministic broad weak pairs for exactly 50 non-repeating
effective-batch updates and 10,000 disjoint pairs for validation only when S0D
authorizes the broad route. Each row has one query and one positive document;
all other documents in the effective batch are negatives.

Fixed mechanism:

- full sparse root is trainable;
- InfoNCE dot-product retrieval loss;
- physical microbatch determined by the passed resource canary, with an exact
  effective negative pool of 2,048 through GradCache;
- query/document maximum lengths 64/192;
- AdamW at `2e-5`, 5% warmup, linear schedule;
- the pinned OpenSearch InfoNCE document-FLOPS schedule: target `0.05`,
  quadratic warmup over 200 updates; no query regularizer is added;
- no KD, hard-negative mining, BM25, qrels, or dataset-specific feature.

Evaluate at step zero and predeclared checkpoints on:

- heldout positive Recall@1, MRR, margin, and InfoNCE;
- M1691 full 2,048x100 stored-reference top1, pairwise, Spearman, and KL;
- query/document nnz, FLOPS, maximum DF, and active vocabulary.

Evaluate fixed checkpoints at updates 10, 25, and 50. S1 passes only when one
checkpoint improves heldout broad InfoNCE, does not reduce heldout Recall@1 or
MRR by more than `0.001`, keeps RLHN top1/MRR within `0.01` and pairwise within
`0.005`, keeps M1691 pairwise and Spearman within `0.005` absolute of the root,
and keeps every sparse-cost ratio within `1.15x`. Training loss alone cannot
pass.

## S2: Broad Curriculum Scale Gate

Only an S1 joint pass authorizes a deterministic 1M-pair continuation. Keep the
same objective and optimizer semantics. Do not add KD or hard negatives yet.

S2 passes when positive-pair gains grow or remain stable, M1691 ordering stays
inside its floor, and cost remains within `1.15x`. Only then may the full 9.26M
author dataset be considered. A 100K failure does not authorize deeper
training; a 100K pass followed by a 1M regression stops broad scaling.

## S3: Curated Hard-Negative Gate

Start from the selected S2 checkpoint. Use one RLHN positive plus 15 cleaned
hard negatives from `msmarco_passage`, following the LACONIC data shape.

Run one paired comparison from identical bytes:

1. contrastive hard-negative training;
2. the same contrastive loss plus the pinned dense+sparse score-distribution
   teacher, but only after teacher observability is verified on the explicit
   positive candidate set.

The paired test answers whether KD adds information after relevance geometry
exists. It does not tune teacher weight. The KD branch must beat the
contrastive control on at least two of positive MRR, full-candidate pairwise,
and native Recall without losing another or exceeding the cost floor.

## S4: DF-Aware Cost Gate

DF-FLOPS is authorized once, with the paper's fixed generalized-logistic shape
and periodically refreshed corpus DF estimates, only when all quality gates
pass and measured failure anatomy is specifically high document max DF or BMP
posting traversal. It is not authorized for an ordering failure.

No alpha, beta, lambda, pruning, or threshold grid is permitted. The branch
must preserve the selected checkpoint's effectiveness while bringing measured
BMP latency, index bytes, and maximum DF within `1.15x` of the root.

## S5: Native Retrieval Ladder

Only a conjunctive checkpoint enters the exact M1660 BMP path:

1. MS MARCO dev sanity without selection on BEIR;
2. locked shared NFCorpus, SciFact, and FiQA;
3. shared15 only after every shared3 row remains safe.

Promotion requires macro NDCG@10, MAP@100, Recall@100, and MRR@20 to improve or
remain unchanged, no shared3 row to lose more than `0.01` NDCG@10 or
Recall@100, exact BMP top100 parity, and engine cost within `1.15x` of the
frozen sparse root.

## Stop Conditions

Stop M1700 when any condition occurs:

- heldout positive retrieval does not improve at the 100K mechanism gate;
- gain requires exact evaluation-query overlap or BEIR-specific data;
- full-candidate ordering or sparse cost fails before scale;
- hard-negative/KD gains vanish through native BMP retrieval;
- only a teacher, lambda, batch, threshold, or pruning grid is proposed next;
- a quality checkpoint requires post-hoc TopK or an external candidate union.

Failure retains the unmodified OpenSearch root plus M1660 BMP as the final
single-index learned-sparse baseline. Success promotes one encoder checkpoint,
one posting representation, and one native inverted index.
