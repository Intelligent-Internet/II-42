# SAE M54 Stage-A Distillation Plan

## Summary

M54 is a Stage-A training stage. Its job is not to optimize BEIR ranking,
qrels, or final product scoring. Its job is to make the direct text encoder
preserve the Snowflake-SAE teacher's sparse semantic shape more faithfully.

```text
text
-> student atoms
-> same support/value/neighborhood shape as teacher atoms
-> hard exported atoms remain usable by the unified sparse engine
```

M54 uses the M53 lesson but changes the objective priority. M53 showed that a
train-only soft TopK retrieval surrogate can improve teacher-support overlap,
but the first smoke mixed too much ranking pressure and therefore degraded
MRR/NDCG/MAP. For Stage A, that ranking regression should not be treated as the
primary failure, but it is a sign that M54 must keep distillation dominant.

## Non-Goals

- Do not optimize qrel ranking as the main objective.
- Do not claim dense retrieval can be removed.
- Do not tune BM25/SAE final ranking weights.
- Do not freeze SQL/API or product runtime behavior.
- Do not use real workload quality claims without qrels/proxy-qrels.

## Teacher Shape To Preserve

M54 distills four teacher properties:

1. **Atom support**: the student should activate the teacher's active SAE
   dimensions.
2. **Atom values**: active atom weights should preserve teacher relative
   strength.
3. **Teacher neighborhood**: under the sparse dot-product retrieval shape, the
   student should preserve the teacher's top-k semantic neighborhood.
4. **Physical shape**: exported query/doc atoms should not increase posting
   fanout or candidate exposure materially.

Ranking metrics remain sanity checks only. Stage B will handle supervised
human relevance ranking.

## Loss Design

The total M54 loss is:

```text
L_m54 =
    w_support * L_support_bce
  + w_rank    * L_support_margin
  + w_value   * L_value_mse
  + w_softk   * L_teacher_soft_topk_recall
  + w_budget  * L_budget_shape
  + w_fanout  * L_fanout
  + w_expose  * L_safe_exposure
```

Recommended initial weights:

```text
w_support = 0.80
w_rank    = 0.10
w_value   = 4.00
w_softk   = 0.08
w_budget  = 0.04
w_fanout  = 0.002
w_expose  = 0.0005
```

### `L_support_bce`

Sampled BCE over teacher active atoms plus random inactive atoms.

Purpose: preserve teacher support first. This remains the dominant signal.

### `L_support_margin`

Teacher atoms should outrank the inactive atoms the student is most tempted to
export.

Purpose: improve exported support without making the full dense BCE too heavy.

### `L_value_mse`

MSE on teacher active atom values.

Purpose: preserve sparse dot-product magnitude enough for downstream
neighborhood scoring.

### `L_teacher_soft_topk_recall`

Train-only differentiable approximation of the candidate top-k selection.
Given a query and a candidate set, teacher-positive documents are trained as a
multi-positive set:

```text
scores_i = dot(student_query_atoms, student_doc_atoms_i)
mask_i   = sigmoid((scores_i - threshold(scores, k)) / tau)
L        = -log(sum(mask_i for i in teacher_positive_docs) / target_mass)
```

The threshold is computed by bisection so the soft mask has approximately `k`
selected documents. Runtime still uses hard TopK/exported atoms.

Purpose: make the exported sparse shape retrieval-aware without using qrel
ranking as the main objective.

### `L_budget_shape`

Same multi-positive ranking shape, but only the top online query atoms are
allowed to score candidates.

Purpose: match the inverted-index candidate-generation budget.

### `L_fanout`

Expected query/doc atom activation weighted by teacher-document atom DF.

Purpose: avoid solving support recall by activating broad high-fanout atoms.

### `L_safe_exposure`

Penalize candidate exposure only for documents outside qrels and dense-teacher
safe neighborhoods.

Purpose: keep physical cost under control without destroying teacher semantic
coverage.

## What Changes From M53

M53 smoke used:

```text
support=0.35
soft_topk=0.35
coverage=0.25
budget=0.20
asym=0.20
```

That was too much retrieval pressure for Stage A. It improved teacher support
recall on `scifact` and `nfcorpus`, but it also changed ranking shape too much.

M54 changes the balance:

- teacher support/value becomes dominant again;
- soft top-k becomes a small distillation-shape regularizer;
- candidate-budget loss becomes weak;
- asymmetric active ranking is disabled in the first Stage-A pass;
- qrel ranking is not a promotion gate.

## Small Smoke

Run against the same two small datasets first:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m54_stage_a_distill.py \
    --datasets scifact nfcorpus \
    --epochs 1 \
    --device mps \
    --execute
```

The smoke compares:

1. baseline checkpoint teacher-fidelity shape;
2. M54 distillation checkpoint teacher-fidelity shape;
3. BEIR ranking sanity check.

## Promotion Gate

Promote M54 to Spark/full Stage-A only if:

- doc and query teacher support recall improve on both smoke datasets;
- doc and query teacher Jaccard improve or remain flat;
- teacher-neighborhood overlap improves once the neighborhood metric is added;
- postings/fanout do not materially increase;
- ranking sanity check does not show severe collapse.

Ranking improvement is not required.

## Next Engineering Additions

M54's first implementation uses existing support overlap and index/fanout
diagnostics. The next useful metric is a teacher-neighborhood overlap report:

```text
teacher query atoms + teacher doc atoms top-k
vs
student query atoms + student doc atoms top-k
```

This is the correct Stage-A retrieval-shape metric. It should be added before
running large M54 variants.
