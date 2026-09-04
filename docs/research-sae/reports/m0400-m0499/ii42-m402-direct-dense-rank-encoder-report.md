# II-42 M402 Direct Dense-Rank Encoder Report

Date: 2026-06-27

## Goal

M402 tests the direct dense-rank version proposed after M401:

- initialize from deterministic M392-style structural coordinates and tail
  sketch;
- train a residual calibrated encoder against exact dense top-k score groups;
- use no qrels during training;
- evaluate held-out FiQA queries with learned-only and BM25-blended routes.

The key gate is strict: learned-only must beat deterministic structural tail
before this line is worth scaling as a dense-faithful text-to-posting encoder.

## Run Surfaces

Remote host: `spark-2`

Remote working directory:
`/home/huoju/leask/runs/mteb-m393-tail-bm25-v1`

Remote output root:
`/home/huoju/leask/runs/ii42-m402-direct-dense-rank-encoder-v1`

Local code:
`scripts/research_sae_m402_direct_dense_rank_encoder.py`

Local plan:
`docs/research-sae/reports/m0400-m0499/ii42-m402-direct-dense-rank-encoder-plan.md`

Execution was CPU-only and low priority:

```bash
CUDA_VISIBLE_DEVICES= OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 \
OPENBLAS_NUM_THREADS=4 nice -n 10 ionice -c2 -n7 ...
```

This avoided interfering with the active `spark-2` GPU training.

## FiQA Canary Matrix

Task: `FiQA2018`

Split: held-out query canary, seed `402`.

Primary metric: `NDCG@10`.

| Run | Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Touch ratio | Dense O@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| exact | dense teacher | 0.49417 | 0.81962 | 0.57332 | 0.43737 | 1.00000 | 1.00000 |
| exact | dense top-k oracle | 0.49417 | 0.81962 | 0.57332 | 0.43737 | 0.01735 | 1.00000 |
| deterministic | structural tail | 0.38753 | 0.57313 | 0.48170 | 0.33186 | 0.08002 | 0.53957 |
| deterministic | structural tail + BM25 a0.10 | 0.49149 | 0.79465 | 0.57833 | 0.43377 | 0.14514 | 0.80037 |
| deterministic | structural tail + BM25 a0.18 | 0.49192 | 0.78772 | 0.58411 | 0.43589 | 0.14514 | 0.74691 |
| initialized | linear base | 0.38145 | 0.57344 | 0.47473 | 0.32826 | 0.08002 | 0.53531 |
| sanity 8 groups, e1 | calibrated linear | 0.38256 | 0.57344 | 0.47475 | 0.32819 | 0.08002 | 0.53534 |
| all groups, e1 | calibrated linear | 0.38467 | 0.57534 | 0.48034 | 0.33051 | 0.08002 | 0.53710 |
| sanity 8 groups, e2 | calibrated MLP | 0.38284 | 0.57844 | 0.47424 | 0.32634 | 0.08002 | 0.53460 |
| all groups, e1 | calibrated MLP | 0.36854 | 0.53936 | 0.46464 | 0.31189 | 0.08002 | 0.51361 |
| sanity 8 groups, e2 | calibrated MLP + BM25 a0.10 | 0.49661 | 0.80246 | 0.58002 | 0.43664 | 0.14517 | 0.78167 |
| all groups, e1 | calibrated MLP + BM25 a0.10 | 0.49227 | 0.78693 | 0.58102 | 0.43098 | 0.14558 | 0.73935 |
| all groups, e1 | calibrated MLP + BM25 a0.18 | 0.49080 | 0.78241 | 0.57857 | 0.43076 | 0.14558 | 0.70352 |

Raw outputs:

- sanity linear:
  `/home/huoju/leask/runs/ii42-m402-direct-dense-rank-encoder-v1/sanity/m402_sanity.json`
- full-query linear e1:
  `/home/huoju/leask/runs/ii42-m402-direct-dense-rank-encoder-v1/linear_all_e1/m402_linear_all_e1.json`
- sanity MLP e2:
  `/home/huoju/leask/runs/ii42-m402-direct-dense-rank-encoder-v1/mlp_sanity_e2/m402_mlp_sanity_e2.json`
- full-query MLP e1:
  `/home/huoju/leask/runs/ii42-m402-direct-dense-rank-encoder-v1/mlp_all_e1/m402_mlp_all_e1.json`

## Interpretation

M402 is healthier than M401, but it still fails the dense-faithful gate.

The learned-only routes do not beat deterministic structural tail:

- teacher structural tail: `0.38753`;
- best learned-only full-query result: calibrated linear e1 `0.38467`;
- calibrated MLP e1 is worse at `0.36854`.

This means direct dense top-k rank training plus residual initialization is not
enough to create a better standalone dense-faithful posting encoder.

The BM25-aware branch has a weak positive signal:

- teacher structural + BM25 a0.18: `0.49192`;
- full-query calibrated MLP + BM25 a0.10: `0.49227`;
- sanity MLP + BM25 a0.10: `0.49661`.

The sanity MLP result exceeds exact dense on this split, but it does not hold
under the full-query MLP run.  Therefore it should be treated as a clue, not a
breakthrough.

## Conclusion

M402 should not be scaled to BEIR15 or MTEB as a dense-faithful encoder.

The line made progress over M401 in stability:

1. no collapse from random initialization;
2. residual calibration preserved the deterministic route;
3. BM25 blend occasionally improved over deterministic structural blend.

But the primary goal is not met:

1. learned-only does not beat deterministic structural tail;
2. Dense O@100 does not improve;
3. MLP capacity does not fix the dense-only gap.

## Recommended Next Step

The next version should split the objective into two explicit branches.

M403-A: dense-faithful branch

- keep deterministic structural target;
- train only a query-conditioned admission correction;
- gate on learned-only NDCG@10 and Dense O@100;
- no BM25 in the pass condition.

M403-B: BM25-aware branch

- make BM25 a training-time input or teacher signal;
- train the model to choose which dense-tail candidates should survive lexical
  fusion;
- evaluate against teacher structural + BM25, not against dense-only;
- treat this as a hybrid retrieval model, not proof that text-to-posting is
  dense-faithful.

The practical signal currently points more strongly to M403-B.  The scientific
dense-faithfulness question remains unsolved by M402.
