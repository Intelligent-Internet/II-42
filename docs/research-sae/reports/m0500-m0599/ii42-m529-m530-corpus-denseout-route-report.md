# M529-M530 Corpus Denseout Route Report

## Question

The route was revised around a stricter first-stage rule:

```text
corpus document text -> dense-derived posting product
```

This stage is query-independent.  It must not use qrels, BM25, route positives,
or ranking losses.  The teacher product is inferred from materialized dense
corpus embeddings, then learned from document text at corpus scale.

## M529 Practical Rerank Diagnostic

M529 keeps the M528 raw-denseout candidate generator and compares two candidate
rerankers on FiQA:

| Source | NDCG@10 | Recall@100 | Teacher O@100 | Candidate R@100 |
| --- | ---: | ---: | ---: | ---: |
| `m529_raw_denseout_self_rerank_p128` | 0.46612 | 0.82370 | 0.89219 | 1.00000 |
| `m529_raw_denseout_teacher_rerank_p128` | 0.47062 | 0.85703 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.47062 | 0.85703 | 1.00000 | 1.00000 |

Candidate generation is not the bottleneck on this smoke: candidate recall is
already 1.0.  The raw self score is still not the teacher dense score, so the
next useful objective is teacher-surface/product preservation, not another
admission/candidate experiment.

## M530 Corpus-Only Product Distillation

M530 trains only on document rows.  It encodes corpus text with the frozen PPLX
root, then trains a residual denseout adapter toward the materialized dense
teacher surface.  The script now keeps raw denseout as an epoch-0 floor: a
trained adapter is only promoted if it beats raw active recall.

### FiQA 4k Smoke

| Source | Active Recall | Cosine | Active Jaccard |
| --- | ---: | ---: | ---: |
| `raw_denseout` | 0.94241 | 0.98915 | 0.90397 |
| `adapted_denseout` | 0.93295 | 0.98888 | 0.88576 |

The naive small residual adapter regressed.  This confirms that a small
query-free smoke is not enough to learn a better surface and should not be used
for promotion.

### Broad4 8k Conservative

| Source | Active Recall | Cosine | Active Jaccard |
| --- | ---: | ---: | ---: |
| `raw_denseout` | 0.93867 | 0.99160 | 0.89350 |
| `adapted_denseout` | 0.93867 | 0.99160 | 0.89350 |

Training rows: 32768.  Best epoch: 0.  The conservative residual adapter did
not beat the raw floor.  The epoch trace decreased from 0.93786 to 0.93490, so
even the safer loss is not enough to improve over frozen raw denseout.

## Interpretation

The user hypothesis is correct at the route level: the first-stage encoder
training should be corpus-scale and query-independent.  The current evidence
also says the latest residual-adapter formulation is not the right large-scale
objective yet.  It either regresses or cannot beat the frozen PPLX denseout
surface.

Do not continue micro-tuning M530 residual weights.  The next useful step is a
data/system correction:

1. Add a persistent raw denseout corpus cache so large corpus experiments do
   not repeatedly pay the PPLX encode cost.
2. Train a real student/product encoder on cached corpus products with a
   hard raw-teacher floor and heldout corpus validation.
3. Use dense-product metrics first: cosine, active recall, signed active
   recall, and teacher top-k score reconstruction.
4. Only after the product encoder beats or matches raw denseout should it
   return to route/NDCG evaluation.

## Artifacts

- `scripts/research_sae_m529_raw_candidate_dense_rerank.py`
- `scripts/research_sae_m530_corpus_denseout_product_distill.py`
- `outputs/m529/raw_candidate_dense_rerank/m529_fiqa_self_rerank_smoke_seed5090.json`
- `outputs/m530/corpus_denseout_product_distill/m530_fiqa_4k_e12_smoke_seed5300.json`
- `outputs/m530/corpus_denseout_product_distill/m530_broad4_8k_conservative_seed5300.json`
