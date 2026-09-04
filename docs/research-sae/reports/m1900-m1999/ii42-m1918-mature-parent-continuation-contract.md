# II-42 M1918 Mature Sparse Parent Continuation Contract

Date: 2026-07-13

## Objective

Determine whether a mature learned-sparse parent can absorb additional PPLX
semantic and ranking signal without losing its native inverted-index quality,
row stability, or corpus-wide cost shape.

M1918 starts only after M1917 completes the exact PostgreSQL comparison of P1,
M1914, and OpenSearch sparse-v2. It is not another output-calibration search and
does not assume that the best frozen route is also the best trainable parent.

## Evidence Behind The Design

The local evidence closes three tempting shortcuts:

- M1914 global power is useful, but support expansion and per-term calibration
  are exhausted. Further progress requires changing learned impacts.
- M1820 showed that a small joint router/payload head on a frozen PPLX root can
  improve local ordering while leaving full-corpus admission almost unchanged.
  PPLX head-only training is therefore closed.
- M1903-M1905 showed that additional training depth can improve ranking, but
  ordinary token-level sparsity does not control corpus-wide document
  frequency. Training depth is not a substitute for a DF-aware cost gate.

The external evidence provides a positive, reproducible alternative:

- OpenSearch sparse-v2 uses heterogeneous dense and sparse supervision plus an
  IDF-aware sparsity objective. Its released model is already a production
  learned-sparse system, not an untrained projection.
- Granite sparse uses retrieval-oriented pretraining, contrastive training,
  knowledge distillation, and model compression. M1914 preserves this mature
  support while improving its impact calibration.
- LACONIC reaches dense-level sparse retrieval through two distinct phases:
  broad weak supervision followed by curated hard-negative training. Its public
  recipe uses millions of pairs rather than a small local head surface.
- SPLARE fine-tunes a pretrained backbone with LoRA while keeping a pretrained
  SAE fixed. It does not learn both the semantic basis and retrieval geometry
  from a random output layer.
- DF-FLOPS periodically estimates corpus-wide document frequency because batch
  FLOPS cannot observe universal terms. PairDistill combines pointwise and
  pairwise distillation and refreshes candidates after training changes their
  ordering.

Primary references:

- [OpenSearch inference-free sparse retrieval](https://arxiv.org/abs/2411.04403)
- [Granite Embedding Models](https://arxiv.org/abs/2502.20204)
- [LACONIC](https://arxiv.org/abs/2601.01684)
- [SPLARE](https://arxiv.org/abs/2603.13277)
- [DF-FLOPS](https://arxiv.org/abs/2505.15070)
- [PairDistill](https://arxiv.org/abs/2410.01383)

## Parent Roles

M1918 keeps the roles separate until evidence supports convergence:

| Route | Initial role | Reason |
| --- | --- | --- |
| OpenSearch sparse-v2 | product parent | mature broad training and native sparse quality |
| M1914/Granite | compact research parent | 30M model, fixed 50/192 support, low native cost |
| P1/M549U | dense-faithfulness control | strongest local dense-root preservation |
| frozen PPLX | semantic teacher | strong dense geometry, not yet a safe sparse parent |

No route is promoted solely from macro quality. Parent selection must consider
per-row quality, positive rescue, normalized PostgreSQL bytes, query latency,
mean and maximum DF, and responsiveness to the same heldout training signal.

## Experiment Ladder

### S0: M1917 observability gate

Use exact positive-document ranks to measure, at 100 and 1,000:

- PPLX-only rescue;
- sparse-parent-only rescue;
- shared positives;
- candidate miss versus under-ranking.

Do not train if PPLX supplies no material unique positives. Reject pure PPLX
imitation when a sparse parent supplies more than 1% sparse-only positive rescue
at top100. In that case, any continuation must use a heterogeneous teacher that
preserves the sparse parent's unique behavior.

### S1: public-data and teacher audit

Build a query-disjoint public pilot with at least 100,000 query and candidate
sets. The first locked source is `rlhn/rlhn-400K`, revision
`6286889b5d2c389e466a1b7bb922d28dbd2ade5a`. Retain only NQ, HotpotQA, and
FEVER. A complete streamed scan of this fixed revision observed 170,803
eligible source rows; strict normalized-query deduplication retained 170,249.
Exclude FiQA, ArguAna, SciDocsRR, and MS MARCO from this pilot so that no M1917
evaluation row or parent-familiar MS MARCO mixture can dominate the response
test.

The union candidate pool must contain:

- parent sparse top candidates;
- PPLX dense top candidates;
- hard negatives near each parent's top100 boundary;
- positives or pseudo-positives from public training data only.

The audit must record source mixture, duplicate leakage, query/corpus overlap,
candidate-source coverage, term coverage, source terms, and teacher
disagreement. It must
demonstrate materially broader coverage than M1518 and M1820 before GPU
training is authorized.

### S2: paired parent response pilot

Run the same query-disjoint pilot on OpenSearch and M1914. Change only a small
LoRA/final-layer parameter set; keep each released sparse head and vocabulary
initialization. Use the same candidate sets and teacher targets.

The objective is:

```text
L = L_parent_pointwise
  + lambda_dense * L_pplx_listwise
  + lambda_pair * L_boundary_pairwise
  + lambda_sparse * L_df_flops
```

`L_parent_pointwise` protects the released parent's useful score geometry and
sparse-only positives. `L_pplx_listwise` transfers dense semantic competition.
`L_boundary_pairwise` focuses on nearby positive/negative ordering rather than
score-shape imitation. `L_df_flops` uses periodically refreshed corpus-level DF
estimates, not batch activation alone.

The pilot is a signal test, not a final run. Select checkpoints on a disjoint
public heldout split. Do not select on BEIR.

### S3: PPLX-root diagnostic

This stage is conditional. It is authorized only when S2 shows that PPLX adds
useful signal which a mature sparse parent can absorb, and when the measured
remaining gap justifies a larger model.

The PPLX branch must differ structurally from M1820:

- train LoRA over a meaningful upper portion of the backbone;
- initialize the sparse target from a mature parent's support or a pretrained
  SAE, never from a random standalone routing head;
- use the S1 public mixture and hard negatives;
- preserve parent-only positives through heterogeneous distillation;
- enforce explicit inference support and corpus-DF budgets.

A frozen-PPLX head-only run, another 10,000-row canary, or reinforcement
learning without a passing supervised observability gate is prohibited.

### S4: scale and native closure

Scale only the winning S2/S3 route. The next rung is at least one million public
pairs with refreshed hard negatives; a multi-million-pair pre-finetuning stage
is required before claiming that a 0.6B PPLX root has been fairly tested.

Every promoted checkpoint must be published to the same PostgreSQL normalized
posting backend used by M1917. Report exact full-corpus retrieval and relation
cost; offline candidate scores are not an acceptance surface.

## Gates

The S2 pilot passes only if a non-initial checkpoint simultaneously:

- improves heldout teacher top1 and pairwise accuracy by at least 0.005;
- does not reduce heldout positive candidate upper bound by more than 0.002;
- preserves at least 99% of parent top100 positive membership;
- keeps document support and normalized posting bytes within 1.05x parent;
- keeps maximum DF below the parent's value and below 0.95;
- does not trade one source or one heldout subgroup for the macro gain.

Native promotion additionally requires:

- positive macro NDCG@10, MAP@100, Recall@100, and MRR@20 against its parent;
- no dataset Recall regression below -0.01;
- no pair of head metrics below -0.02 on any row;
- candidate upper bound and dense overlap consistent with the intended role;
- no dataset-specific threshold, alpha, or checkpoint selection.

## Stop Conditions

- Stop a parent after two independent seeds fail the S2 heldout gate.
- Stop if lower training loss only increases high-DF terms or normalized bytes.
- Stop if PPLX improvements require suppressing material sparse-only rescue.
- Stop PPLX-root training if it cannot exceed mature-parent response at the
  100,000-row rung; do not scale a weaker student because the backbone is
  larger.
- Stop if exact native replay removes the offline gain.

## Expected Decision

M1918 is designed to return one of four explicit outcomes:

1. OpenSearch is the best product and trainable parent.
2. M1914 is the better compact continuation despite lower frozen quality.
3. PPLX contributes teacher value but should not become the sparse encoder.
4. A deep PPLX-root continuation is justified by both observability and parent
   response, authorizing a separately budgeted large training program.

This contract does not promise a breakthrough. It prevents the project from
calling calibration, a lower distillation loss, or a PPLX-sized model a
breakthrough without native quality and cost evidence.
