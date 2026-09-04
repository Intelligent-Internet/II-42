# M1701 Listwise Hard-Negative Curriculum Contract

## Decision Boundary

M1700 tests broad positive-only InfoNCE on a mature OpenSearch learned-sparse
root. Its heldout positive loss improves, but full-candidate ordering and
posting cost drift before native retrieval. M1701 does not tune that loss. It
changes the supervision to the published strong-retriever adaptation shape:

```text
RLHN query + one positive + 15 relabeled hard negatives
    -> pinned cross-encoder score distribution
    -> listwise KL + 0.1 contrastive loss
    -> one vocabulary-sparse encoder
    -> one exact BMP/native inverted index
```

The dense or cross-encoder teacher is training-only. M1701 adds no ANN field,
BM25 union, reranker, post-hoc TopK, or dataset-specific policy.

## Evidence Basis

- [Conventional Contrastive Learning Often Falls Short](https://arxiv.org/abs/2505.19274)
  reports that InfoNCE frequently degrades already strong retrievers even
  after hard-negative filtering. Its stable recipe uses cross-encoder listwise
  KL over one positive plus up to 19 retrieved candidates, with student and
  teacher temperatures `0.05/0.3` and contrastive weight `0.1`.
- [Scaling Sparse and Dense Retrieval](https://arxiv.org/abs/2502.15526)
  finds that sparse `CL + KD` is more robust than either objective alone.
- [SPLADE-v3](https://arxiv.org/abs/2403.06789) establishes multiple hard
  negatives and cross-encoder distillation as the mature sparse recipe.
- [RLHN](https://arxiv.org/abs/2505.16967) shows that relabeling false hard
  negatives improves out-of-domain transfer and that data quality can matter
  more than raw data scale.
- [Beyond Hard Negatives](https://arxiv.org/abs/2604.04734) requires a broad
  teacher score spectrum rather than only the hardest binary negatives.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) is reserved for a later measured
  high-DF failure; it is not allowed to mask an ordering failure.
- [Rescaling MLM-Head for Neural Sparse Retrieval](https://arxiv.org/abs/2606.18811)
  identifies unnormalized sparse-dot scale as an optimization variable that can
  inflate ranking gradients and destabilize their interaction with FLOPS.
- [Gradient Episodic Memory](https://arxiv.org/abs/1706.08840) supplies the
  half-space projection used when a retrieval update conflicts with the frozen
  root's sparse-cost gradient. The cost constraint is not selected by a scalar
  sweep.

The CADET reference repository is pinned at
`96a8768cc7a1bdea57f426ba05ffd0e8e6fc120b`. The initial teacher is
`cross-encoder/ms-marco-MiniLM-L6-v2` at
`c5ee24cb16019beea0893ab7796b1df96625c6b8`. This model matches the teacher
family used by the sparse CL+KD scaling study and is small enough for a strict
observability gate. A larger teacher is not authorized unless this teacher is
informative but demonstrably too weak.

If that exact condition occurs, one RankT5-3B escalation is permitted using
`Soyoung97/RankT5-3b` at
`40e2b98fcbc6d60457c88508bd775fcb8395a5b0`, matching the CADET reference
implementation. Run 512 rows as a compute/mechanism canary first; only a pass
expands the unchanged teacher to the locked 2,048-row surface. No third teacher
or ensemble is allowed.

## S0: Teacher Observability

Use 2,048 deterministic, shared15-disjoint RLHN `msmarco_passage` rows. Each
row contains one positive followed by exactly 15 cleaned negatives.

1. Score all 32,768 query-document pairs with the pinned cross-encoder.
2. Apply CADET's global 1st/99th percentile clipping and min-max normalization.
3. Evaluate the frozen sparse root on the identical rows.
4. Report positive top1/MRR, pairwise accuracy, score variance, ties, and the
   exact eligible set where the teacher ranks the positive first.
5. Persist all and eligible Parquet files with source/model revisions and
   hashes.

Training is authorized only when the teacher has positive top1 at least
`0.80`, pairwise accuracy at least `0.95`, improves root top1 by `0.05` and
pairwise by `0.01`, and has non-constant per-row scores. This is one causal
teacher check, not a teacher search.

## S1: Fixed Mechanism Canary

Only S0 pass authorizes an objective-gradient audit from the unmodified sparse
root. Use one positive plus 15 negatives and query/document lengths `64/192`.
The first audit must evaluate the published dense-retriever parameters without
silently adapting them:

```text
loss = listwise_KL(student_tau=0.05, teacher_tau=0.3)
     + 0.1 * contrastive_InfoNCE
     + inherited_sparse_cost_constraint
```

CADET applies these temperatures to L2-normalized dense embeddings. They are
not assumed to transfer to unnormalized sparse impacts. If the locked audit
shows loss saturation or gradient dominance, one scale-adapted M1701A audit is
permitted using only the following deterministic transformation:

```text
student_tau = median(row_std(student_raw_dot))
              / median(row_std(teacher_score)) * teacher_tau
contrastive = InfoNCE(L2_normalize(query), L2_normalize(document), tau=0.01)
cost = parameter-space GEM constraint against document FLOPS
```

This is a score-scale correction, not a temperature grid. It keeps native
inference as the original unnormalized sparse dot product. Four exact GradCache
updates are authorized only when all M1701A gradient terms are finite and
nonzero, the selected/teacher entropy gap is at most `0.15`, retrieval-term
dominance is at most `20x`, and projection preserves primary and cost descent.

The cost constraint must preserve the mature root's pressure from update one;
restarting a long quadratic warmup from zero is forbidden because M1700 shows
that it releases document density before the first quality gate. Before the
canary, measure the three representation-gradient norms and pairwise cosine
conflicts. If one term dominates by more than `20x`, stop and use a
constraint/projection formulation rather than choosing a scalar grid.

The locked row allocation is audit `0-31`, training `32-159`, and disjoint
teacher validation `160-671` within the eligible RankT5 artifact. S1 then
evaluates disjoint broad pairs and the complete M1691
`2,048 x 100` ordering/cost surface. A checkpoint passes only when it improves
teacher listwise KL and RLHN MRR, keeps broad top1/MRR within `0.001`, keeps
M1691 top1/pairwise/Spearman within `0.005`, and keeps every sparse-cost ratio
within `1.15x`.

If the fixed step-4 checkpoint passes teacher and broad validation but misses
the M1691 ordering gate, one deterministic depth audit may persist and evaluate
steps `1-4` from the identical trajectory. This is the only permitted training
depth check. Selection requires the same conjunctive gates and uses no qrels.
If no earlier checkpoint passes, M1701 stops; changing learning rate,
temperature, projection weight, or a gate floor is forbidden.

## S2: Native Ladder

Only a conjunctive S1 checkpoint enters exact retrieval:

1. MS MARCO dev sanity;
2. locked NFCorpus, SciFact, and FiQA shared3;
3. shared15 only after every shared3 row is safe;
4. official BMP exact top100 parity and index/latency cost.

Promotion requires non-negative macro NDCG@10, MAP@100, Recall@100, and MRR@20
against the frozen sparse root, no shared3 row loss above `0.01`, exact BMP
parity, and cost within `1.15x`.

## Stop Conditions

Stop this route when the teacher is not observably better than the root, the
combined objective still trades ordering for positive loss, the sparse-cost
gate fails before native retrieval, gains require evaluation-query overlap, or
the next proposal is a teacher/temperature/lambda/threshold grid. A failure
retains the unmodified OpenSearch root plus M1660 BMP.
