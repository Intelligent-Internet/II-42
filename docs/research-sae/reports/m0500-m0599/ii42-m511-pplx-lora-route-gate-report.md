# M511 PPLX-LoRA Route Gate Report

M511 moves the M510 PPLX-LoRA direct-posting encoder from representation
diagnostics into the real posting route/qrels evaluator.

The evaluated path is:

```text
text -> PPLX + LoRA dense/support heads -> support top-k candidates
     -> M506 structural scorer -> qrels metrics
```

Training remains first-stage dense/posting distillation only: no BM25, no qrels
loss, and no dataset-specific relevance optimization.

## Runs

| Run | Host | Task | Route docs | Prefixes | Output |
| --- | --- | --- | ---: | --- | --- |
| `fiqa_lora_route_subset64` | `spark-1` | `FiQA2018` | `4096/57638` | `256,512,768` | `outputs/m511/fiqa_lora_route_subset64/m511_fiqa_lora_route_subset64.json` |
| `fiqa_lora_route_tight` | `spark-1` | `FiQA2018` | `4096/57638` | `32,64,128` | `outputs/m511/fiqa_lora_route_tight/m511_fiqa_lora_route_tight.json` |

Both route subsets include qrels-positive docs and row-int8 dense teacher
nearest neighbours for the heldout eval queries.  These are route gates, not
full-corpus BEIR scores.

## Wide Prefix Gate

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `route_subset_teacher_row_int8_dense` | 0.47062 | 0.85703 | 0.56380 | 0.41763 | 1.00000 | 1.00000 |
| `m506_structural_candidates_p256` | 0.46704 | 0.87786 | 0.57153 | 0.41681 | 1.00000 | 0.99797 |
| `m511_lora_support_candidates_p256` | 0.46704 | 0.87786 | 0.57153 | 0.41681 | 0.99922 | 0.98870 |
| `m511_lora_support_candidates_p512` | 0.46704 | 0.87786 | 0.57153 | 0.41681 | 1.00000 | 0.99956 |
| `m511_lora_support_candidates_p768` | 0.46704 | 0.87786 | 0.57153 | 0.41681 | 1.00000 | 0.99997 |

The LoRA support surface can be used as the posting candidate surface without
breaking the scorer: p256 exactly preserves the structural route metrics on
this subset and only loses 0.00078 candidate recall.  However, p256/p512/p768
still touch nearly the whole route subset, so this is not yet an efficient
posting encoder.

## Tight Prefix Gate

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `route_subset_teacher_row_int8_dense` | 0.47062 | 0.85703 | 0.56380 | 0.41763 | 1.00000 | 1.00000 |
| `m506_structural_candidates_p128` | 0.46704 | 0.87266 | 0.57235 | 0.41674 | 0.99359 | 0.95531 |
| `m511_lora_support_candidates_p128` | 0.46717 | 0.86693 | 0.57186 | 0.41586 | 0.97891 | 0.91362 |
| `m506_structural_candidates_p64` | 0.46041 | 0.85443 | 0.55695 | 0.41216 | 0.92750 | 0.79585 |
| `m511_lora_support_candidates_p64` | 0.46082 | 0.85391 | 0.57315 | 0.40795 | 0.86328 | 0.73024 |
| `m506_structural_candidates_p32` | 0.43788 | 0.79204 | 0.53383 | 0.39410 | 0.74953 | 0.55809 |
| `m511_lora_support_candidates_p32` | 0.44845 | 0.74475 | 0.56567 | 0.39719 | 0.65047 | 0.50278 |

The best current compression point is `m511_lora_support_candidates_p128`.
It keeps NDCG@10 at 0.46717, essentially matching structural p128 and staying
within 0.00345 of the route-subset dense teacher, while reducing touched docs
from structural p128 0.95531 to 0.91362.

Lower prefixes show the bottleneck.  p64 improves touched ratio to 0.73024, but
candidate recall falls to 0.86328.  p32 reaches 0.50278 touch, but candidate
recall collapses to 0.65047.  The route is no longer representation-limited
only; it needs explicit fanout-aware support shaping.

## Interpretation

M511 supports continuing the PPLX-root route, but not by simply adding LoRA
epochs.  The current model can produce a support surface that is safe enough
for routing, yet its coordinate sharing is too broad.  The main gap is
candidate fanout control under preserved dense recall.

This separates the problem into two proven facts:

- Dense/quality survival is plausible: LoRA support p128 nearly matches the
  structural route and dense subset quality.
- Efficient posting survival is not solved: useful quality still requires
  touching more than 90% of the route subset.

## M512 Recommendation

The next version should train the support head with an explicit sparse route
objective, not just cosine/MSE/support active overlap:

1. Add a route fanout pressure term on query/doc active coordinate sharing.
2. Preserve dense teacher top-k candidate recall with a differentiable or
   sampled contrastive candidate loss.
3. Keep scoring separate: continue using the structural scorer while only
   training the candidate support surface.
4. Gate on FiQA route subset first, with promotion only if p64/p96 can keep
   candidate recall above roughly 0.95 while reducing touch materially below
   p128.

Until M512 passes that gate, this line should not be scaled to Broad4/full
corpus as a final retrieval result.
