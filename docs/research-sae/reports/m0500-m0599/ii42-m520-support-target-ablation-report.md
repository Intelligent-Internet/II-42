# M520 Support Target Ablation

M520 tests whether the recent text-to-posting failures are caused by route
threshold tuning or by the first-stage support target shape.  It keeps the same
PPLX-LoRA encoder, the same M514 baseline route phase, and the same M511 route
subset evaluator, then swaps only the support target.

Training remains dense-only: no BM25 and no qrels in the loss.  Qrels are used
only by the route evaluator.

## Run

- Host: `spark-1`
- Output: `outputs/m520/fiqa_support_target_ablation/`
- Remote run dir: `/home/huoju/leask/runs/ii42-m520-support-target-ablation-v1`
- Task: `FiQA2018`
- Route subset: `4096/57638` docs, `64` heldout queries
- Prefixes: `64,96,128`
- Target modes: `m508_support`, `dense_signed`, `structural_coord`
- Elapsed: `814.88s`

The run used the NVIDIA PyTorch 26.03 container with temporary Python
dependencies under the run directory.  The PPLX model remote code was fetched
during the run; the next scale-up should pin the model revision to remove this
source of drift.

## Best Learned Route Per Target

| Target Mode | Best Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m508_support` | `m511_lora_support_candidates_p96` | 0.46755 | 0.88188 | 0.57180 | 0.41953 | 0.88656 | 0.60019 |
| `dense_signed` | `m511_lora_support_candidates_p64` | 0.47050 | 0.88787 | 0.57467 | 0.41988 | 0.86047 | 0.52328 |
| `structural_coord` | `m511_lora_support_candidates_p96` | 0.45396 | 0.85923 | 0.55910 | 0.40472 | 0.83672 | 0.60094 |

Dense references on the same route subset:

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `route_subset_materialized_dense` | 0.47054 | 0.86224 | 0.56380 | 0.41764 | 1.00000 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 0.47062 | 0.85703 | 0.56380 | 0.41763 | 1.00000 | 1.00000 |

## Interpretation

The useful result is `dense_signed`: direct row-int8 dense coordinates as the
support target reach dense-level NDCG on this route gate while using much less
fanout than dense.  It also beats both dense references on R@100, MRR@20, and
MAP@100 in this FiQA subset, though the candidate upper bound is still below
the full dense reference.

This changes the diagnosis.  The route-threshold branch was not the main
blocker; the first-stage target shape matters.  M508 support is a reasonable
fallback but appears too indirect.  M506 structural rotated coordinates are not
a good text-encoder target in this setup despite being a strong non-learned
route reference.

The result does not prove full-corpus replacement of dense yet.  It is a route
subset gate.  The next promotion must check whether `dense_signed` survives
larger route subsets and broader tasks without overfitting FiQA.

## Next Step

M521 should promote only the `dense_signed` target and drop `structural_coord`
from the immediate training loop.

Recommended M521 plan:

1. Pin the PPLX model revision and remove `TRANSFORMERS_CACHE` usage.
2. Run `dense_signed` on broad4 with the same FiQA gate settings first.
3. Add deeper training variants only after broad4 preserves the FiQA signal:
   more epochs, larger route groups, and a second seed.
4. Compare against M519 baseline and the same route-subset dense references.
5. Promote to bire/full matrix only if broad4 keeps dense-level NDCG with lower
   touch and does not collapse candidate recall.

Current decision: continue this line.  The best evidence now points to
direct dense-coordinate target distillation, not learned M508 support targets
or rotated structural coordinate targets.
