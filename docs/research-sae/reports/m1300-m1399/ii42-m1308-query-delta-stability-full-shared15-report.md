# M1308 Query-Delta Stability Full Shared15

## Question

M1307 closed the M1288/M130x pair-witness post-hoc compiler family.  Before
starting a larger objective redesign, M1308 validates the retained query-delta
signals on the full `shared15` surface:

- `signed_sum`
- `mean_teacher`
- scales `0.5`, `0.75`, `1.0`

The goal is not to promote a fixed policy.  The goal is to decide whether
`signed_sum_s1` or `mean_teacher_s0.75/s1` is strong enough to serve as the
next training objective/source target.

## Run

```bash
python3 scripts/audit_m1293_query_delta_stability_grid.py \
  --datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,cqadupstack,webis-touche2020,climate-fever,dbpedia-entity,fever,hotpotqa,msmarco,nq,quora \
  --modes signed_sum,mean_teacher \
  --scales 0.5,0.75,1.0 \
  --output-root runs/m1308_query_delta_stability_full_shared15_v1
```

Artifacts:

- JSON:
  `runs/m1308_query_delta_stability_full_shared15_v1/m1293_stability.json`
- Markdown:
  `runs/m1308_query_delta_stability_full_shared15_v1/m1293_stability.md`

Surface:

- datasets: full `shared15`
- queries: `1342`
- baseline:
  - Recall@100 `0.864763`
  - MAP@100 `0.667856`
  - NDCG@10 `0.737286`
  - MRR@20 `0.820878`
  - CUB `0.947106`

## Macro Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `signed_sum_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | 0.030620 |
| `mean_teacher_s1` | 7.697 | 0 | +0.001653 | +0.002902 | +0.002442 | +0.002807 | +0.000017 | 0.027485 |
| `signed_sum_s0.75` | 7.975 | 0 | +0.000943 | +0.003131 | +0.002927 | +0.003098 | +0.000145 | 0.026302 |
| `signed_sum_s0.5` | 7.975 | 0 | +0.001448 | +0.001783 | +0.001633 | +0.001728 | +0.000236 | 0.019547 |
| `mean_teacher_s0.75` | 7.697 | 1 | +0.001527 | +0.002151 | +0.001861 | +0.001309 | -0.000063 | 0.018801 |
| `mean_teacher_s0.5` | 7.697 | 1 | +0.001369 | +0.001056 | +0.000515 | +0.000646 | -0.000239 | 0.006112 |

The full surface preserves the main M1251/M1253 signal: `signed_sum_s1` is
macro-clean across all five metrics and is the strongest fixed query-delta
target.

## Row-Level Shape

Negative dataset counts:

| Variant | NegativeDatasets | Datasets |
| --- | ---: | --- |
| `signed_sum_s1` | 7 | `nfcorpus`, `fiqa`, `scidocs`, `trec-covid`, `cqadupstack`, `webis-touche2020`, `dbpedia-entity` |
| `mean_teacher_s1` | 5 | `nfcorpus`, `trec-covid`, `cqadupstack`, `webis-touche2020`, `dbpedia-entity` |
| `signed_sum_s0.75` | 7 | `nfcorpus`, `fiqa`, `scidocs`, `trec-covid`, `cqadupstack`, `webis-touche2020`, `dbpedia-entity` |
| `signed_sum_s0.5` | 4 | `fiqa`, `trec-covid`, `cqadupstack`, `webis-touche2020` |
| `mean_teacher_s0.75` | 5 | `scidocs`, `trec-covid`, `cqadupstack`, `webis-touche2020`, `dbpedia-entity` |
| `mean_teacher_s0.5` | 7 | `nfcorpus`, `fiqa`, `scidocs`, `trec-covid`, `cqadupstack`, `webis-touche2020`, `dbpedia-entity` |

Important per-dataset deltas:

| Dataset | `signed_sum_s1` shape | `mean_teacher_s0.75` shape |
| --- | --- | --- |
| `scifact` | all positive, Recall +0.010000 | all positive, Recall +0.010000 |
| `arguana` | MAP/NDCG/MRR positive | MAP/NDCG/MRR positive |
| `climate-fever` | all positive | all positive |
| `msmarco` | Recall/MAP/NDCG/CUB positive | Recall/MAP/NDCG/CUB positive |
| `cqadupstack` | MAP/NDCG/MRR negative | Recall/MAP/NDCG/MRR negative |
| `webis-touche2020` | NDCG negative | NDCG negative |
| `trec-covid` | NDCG negative | NDCG/CUB negative |
| `dbpedia-entity` | Recall negative | CUB negative |

The fixed policies are therefore useful objective signals but still fail the
row-safe deployment requirement.

## Interpretation

M1308 gives a positive retained signal after M1307 closed the pair/witness
post-hoc family.

What is now clearer:

- `signed_sum_s1` is the strongest full-surface fixed query-delta target.
- The signal is not a small hard-row artifact; it survives full `shared15`.
- `mean_teacher_s1` is also macro-clean and may be useful as a smoother
  auxiliary target.
- `mean_teacher_s0.75` is not the right full-surface default because it loses
  CUB macro, despite being clean on the earlier risk7 shape.

What remains unsolved:

- row-level harm persists across several datasets;
- the harm is not solved by fixed scale;
- this still cannot be promoted as a native policy;
- objective training must internalize row safety instead of using
  `signed_sum_s1` directly.

## Decision

Keep `signed_sum_s1` as the primary movement target for the next branch.

Use `mean_teacher_s1` as a secondary/smoothing target, not as the main policy.

Do not continue fixed replay-policy tuning:

- no more scale grids over the same source;
- no threshold guards unless a prior observability audit shows real separation;
- no promotion of `signed_sum_s1` as default.

## Next Valid Branch

M1309 should be a constrained objective/source construction probe, not another
selector family.

Minimum shape:

1. Movement target: match or distill `signed_sum_s1` / `mean_teacher_s1`.
2. Safety target: penalize query/dataset rows where M1308 shows Recall, NDCG,
   MRR, or CUB degradation.
3. Source constraint: avoid the M1288/M130x pair/witness post-hoc rows unless
   a new source changes the candidate construction.
4. Gate: first test separability/objective fit, then native smoke; only scale
   if the smoke preserves macro lift and reduces the named row harms.

The retained hypothesis is:

> A coordinated query-delta objective can improve the native unified-posting
> surface, but the objective must learn row-safety directly.  Post-hoc
> selection/calibration has been exhausted.
