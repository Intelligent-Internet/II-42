# M1317 Two-Stage Native Replay

## Question

M1316 showed that a single query vector cannot currently satisfy both needs:

- full fill pressure is needed to preserve candidate upper bound;
- lower fill pressure is needed to avoid NDCG harm.

M1317 tests the structural fix: keep one unified-posting index, but split query
usage into two native stages:

1. candidate stage: M1314 `uniform_l1_head50_minus_tail_fill4_s1`;
2. ranking stage: a separate rank-safe query vector over that candidate pool.

This is not a new selector and not a dataset-specific threshold. It checks
whether a single-pass score path is the bottleneck.

## Run

```bash
python3 scripts/replay_m1317_two_stage_native.py \
  --datasets cqadupstack,scidocs,webis-touche2020 \
  --output-root runs/m1317_two_stage_native_smoke_v1
```

Artifacts:

- Script: `scripts/replay_m1317_two_stage_native.py`
- JSON: `runs/m1317_two_stage_native_smoke_v1/m1317_native.json`
- Markdown: `runs/m1317_two_stage_native_smoke_v1/m1317_native.md`

Surface:

- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`
- queries: `249`
- validation: leave-one-dataset-out

## Method Note

The current SQL path returns fused candidate rows for one query vector. M1317
therefore uses a native two-query replay:

- run the candidate query and keep its native top1000 docs;
- run the ranking query through the same native path;
- order candidate docs by ranking-query native order when available;
- keep candidate-only docs after ranked docs in candidate-stage order.

This is a replay approximation of a two-stage native implementation. It is
still database-backed and uses the unified posting index, but it is not yet a
single production SQL operator.

## Result

| Variant | RankAtoms | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `rank_prefix_uniform_l1` | 1.309 | 0 | +0.001977 | +0.000862 | +0.000654 | +0.000695 | +0.000015 | 0.015183 |
| `rank_fill_fs0.5` | 3.422 | 0 | +0.001847 | +0.000739 | +0.000395 | +0.001364 | +0.000015 | 0.014988 |
| `rank_fill_fs1.0` | 3.422 | 1 | +0.002030 | +0.000708 | -0.000054 | +0.001153 | +0.000015 | 0.013944 |
| `rank_fill_fs0.25` | 3.422 | 0 | +0.001847 | +0.000569 | +0.000480 | +0.000695 | +0.000015 | 0.013307 |

`rank_prefix_uniform_l1` is the best smoke result:

- all macro metrics are non-negative;
- CUB remains positive because the candidate stage keeps M1314 fill4 pressure;
- NDCG turns positive because the ranking stage no longer uses full fill
  pressure;
- rank query is very small: `1.309` atoms/query.

Fold-level absolute metrics:

| Heldout | Variant | Recall | MAP | NDCG | MRR | CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | `rank_prefix_uniform_l1` | 0.944677 | 0.683897 | 0.749626 | 0.825379 | 0.994966 |
| `scidocs` | `rank_prefix_uniform_l1` | 0.703500 | 0.325381 | 0.417426 | 0.633435 | 0.915000 |
| `webis-touche2020` | `rank_prefix_uniform_l1` | 0.979139 | 0.883839 | 0.877206 | 1.000000 | 0.997996 |

## Interpretation

M1317 is the first clean signal after M1314-M1316 that changes the structure
rather than tuning a source.

The retained conclusion:

> unified posting likely needs separate candidate and rank phases. Candidate
> support atoms should be allowed to recover CUB without forcing their full
> value pressure into final top-rank ordering.

This directly explains the M1315/M1316 frontier:

- fill4 full value is good for candidate coverage;
- prefix-only ranking is safer for top-rank metrics;
- combining them in one vector causes the CUB/NDCG conflict.

## Decision

Promote M1317 to row-level anatomy, not full `shared15` yet.

Required next checks:

1. Store per-query rows for `rank_prefix_uniform_l1`.
2. Compare row-level harm against M1314 and M1316.
3. Verify that gains are not caused by one dataset only.
4. If row anatomy is clean, replay on risk7 or full `shared15`.
5. If row harm reappears, redesign two-stage ranking policy rather than going
   back to single-stage value or gate tuning.

Stop conditions:

- if per-row harm concentrates in `cqadupstack` or `webis-touche2020`;
- if full shared15 gives back CUB or NDCG;
- if gains rely on candidate docs that the production native path cannot keep
  without an explicit two-stage operator.
