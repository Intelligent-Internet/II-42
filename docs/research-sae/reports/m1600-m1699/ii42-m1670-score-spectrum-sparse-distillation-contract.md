# M1670 Score-Spectrum Sparse Distillation Contract

## Route Reset

M1660 establishes a stable product substrate: one vocabulary-sparse encoder,
one posting map, and one exact inverted index. M1650/M1651 then locate the
remaining model frontier: dense-plus-sparse supervision improves ranking, but
the 4K-row, three-negative continuation cannot meet teacher fit and sparse cost
simultaneously.

M1670 tests one structural explanation before scaling: the old training rows
observe too little of the teacher score distribution.

This is not a return to pooled SAE reconstruction, P1 codebooks, BM25 fusion,
or query-time selectors.

## Evidence

The pinned 10K MS MARCO surface contains one positive and eight stored
cross-encoder-scored negatives per query. M1650 retained only the first three.
An all-row audit found:

- only `0.01%` of negative lists are already score-sorted;
- first-three mean score-range coverage is `0.5811`;
- deterministic min/median/max stratified-three coverage is `1.0`;
- the positive is cross-teacher top1 on `78.82%` of rows.

This means M1650's candidate slice was neither the hard-top set nor a
representative teacher-distribution set.

The design is supported by four independent results:

1. OpenSearch sparse training uses normalized heterogeneous dense+sparse
   teachers at multi-million-query scale, then cross-encoder fine-tuning.
2. Stratified distillation (`2604.04734`) finds that hard/top candidates alone
   omit preference structure; deterministic score-quantile samples improve
   in-domain and BEIR generalization across KL and MarginMSE.
3. DF-FLOPS (`2505.15070`) shows that production cost is governed by corpus DF,
   not row nnz alone, and uses long schedules rather than a short fixed-lambda
   mutation.
4. SAE-SPLADE (`2604.21511`) finds joint reconstruction and retrieval training
   detrimental; sparse preconditioning followed by IR distillation is the
   stable two-stage form.

Vocabulary Transfer (`2607.00004`) is a high-upside independent fallback. It
changes backbone vocabulary and requires semantic initialization/APC, so it is
not mixed into this candidate-composition test.

## S0: Teacher Observability

Use 2,048 deterministic MS MARCO rows, one positive, and all eight negatives.
Encode every query/document with the frozen BGE dense teacher and frozen
OpenSearch sparse root. Stored cross scores are offline supervision only.

Compare three candidate sources:

- `legacy_first3`: the M1650 slice;
- `cross_top3`: three highest cross-teacher negatives;
- `cross_stratified3`: distinct negatives nearest min/median/max score
  quantiles.

For both the old dense+sparse teacher and a normalized
dense+sparse+cross teacher, report score-range coverage, entropy, pair/top1
accuracy, sparse-error recovery, dense-new-error count, and teacher agreement
on a disjoint 512-row heldout set.

S1 is authorized only if stratified-three covers at least 85% of the selected
teacher range, exceeds legacy coverage by at least 20 percentage points, and
the three-teacher signal does not lower heldout pair accuracy relative to the
old teacher.

## S1: Paired 500-Step Mechanism Canary

Run two sequential branches from the identical OpenSearch root, seed, rows,
optimizer, and normalized three-teacher objective:

1. `legacy_first3`;
2. `cross_stratified3`.

Keep M1651 root-relative query/document mass and FLOPS constraints unchanged.
Evaluate both branches on the same full nine-document heldout candidate sets,
not only on their training slices.

The stratified branch passes only if:

- full-set heldout teacher KL improves at least 5% from step 0;
- full-set pair accuracy is no lower than step 0;
- sparse-anchor Spearman is at least 0.95;
- query/document nnz, FLOPS, and max DF are each at most 1.10x root;
- its KL improvement exceeds the paired legacy branch without exchanging away
  pair accuracy or sparse cost.

No checkpoint is selected by training loss alone.

## S2: Medium Scale

Only a passing S1 configuration may use all 10K pinned rows and a longer
schedule. Scale one axis at a time:

- 2,000 steps first;
- 4,000 only if the 2,000-step validation frontier is still improving without
  cost drift.

Do not tune candidate count, quantiles, teacher weights, dual step, or loss
coefficients on BEIR. Track every run in ClearML.

## S3: Independent Retrieval Gate

Only a conjunctive S2 checkpoint may run complete NFCorpus, SciFact, and FiQA
with unchanged sparse encoding and official BMP-compatible 8-bit impacts.

Require:

- no row below 98% of the frozen OpenSearch root Recall@100;
- macro NDCG@10 and MAP@100 no lower than the root;
- BMP exactness unchanged;
- document/query nnz, max DF, index bytes, and p95 latency reported;
- gains on at least two of three rows.

Passing shared3 authorizes a broader BEIR run. It does not yet promote a
default model.

## Stops

- Stop before training if S0 cannot distinguish score-spectrum coverage.
- Stop if stratified sampling changes loss but not full-set heldout ranking.
- Stop if cost can pass only by post-hoc TopK, threshold, or pruning.
- Stop if improvement exists only on the sampled training candidate slice.
- Stop after two scale checkpoints if broader quality does not improve.

A stop returns the project to full Vocabulary Transfer/APC reproduction, not
another selector, gate, or output-head mutation.
