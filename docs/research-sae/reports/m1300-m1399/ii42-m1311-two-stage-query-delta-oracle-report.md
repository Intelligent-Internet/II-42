# M1311 Two-Stage Query-Delta Oracle Report

## Question

M1309 proved that the `signed_sum` / `mean_teacher` movement family has a
strong all-safe query-level oracle. M1310 then failed because a single-stage
ranker collapsed to baseline for every query.

M1311 tests the smallest structural repair:

1. train a query-level move/abstain detector;
2. train a movement-source selector only on oracle-move queries;
3. replay predicted choices on the same risk7 surface before any broader run.

This is a bounded observability test, not a deployment run.

## Run

```bash
python3 scripts/train_m1311_two_stage_query_delta_oracle.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --modes signed_sum,mean_teacher \
  --scales 0.5,0.75,1.0 \
  --output-root runs/m1311_two_stage_query_delta_oracle_risk7_v1
```

Artifacts:

- Script: `scripts/train_m1311_two_stage_query_delta_oracle.py`
- JSON: `runs/m1311_two_stage_query_delta_oracle_risk7_v1/m1311_two_stage.json`
- Markdown: `runs/m1311_two_stage_query_delta_oracle_risk7_v1/m1311_two_stage.md`

Surface:

- datasets: `cqadupstack`, `fiqa`, `webis-touche2020`, `nfcorpus`,
  `dbpedia-entity`, `scidocs`, `trec-covid`
- queries: `599`
- validation: leave-one-dataset-out

## Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_all_safe` | 3.391 | 0 | +0.003217 | +0.004783 | +0.005622 | +0.002608 | +0.002423 | 0.049318 |
| `mean_teacher_s0.75` | 7.673 | 0 | +0.001291 | +0.001114 | +0.001019 | +0.000067 | +0.000037 | 0.012005 |
| `signed_sum_s1` | 7.816 | 1 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | 0.005682 |
| `two_stage_train_f1` | 7.356 | 1 | +0.001867 | +0.001156 | +0.001058 | -0.000961 | +0.000046 | 0.003441 |
| `two_stage_p05` | 3.499 | 3 | +0.000913 | -0.000188 | -0.000364 | -0.000831 | +0.000167 | -0.012051 |

M1311 does not recover the M1309 oracle.

The fixed-threshold policy moves fewer queries than `signed_sum_s1`, but it
becomes macro-negative and adds three negative metrics. The train-F1 threshold
keeps positive Recall/MAP/NDCG but is weaker than fixed `signed_sum_s1` and
still has the same MRR harm.

## Diagnostics

| Policy | Exact | MovePrecision | MoveRecall | SourceExact | PredMove | OracleMove |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `two_stage_p05` | 0.4157 | 0.4542 | 0.4733 | 0.4919 | 273 | 262 |
| `two_stage_train_f1` | 0.2287 | 0.4309 | 0.9275 | 0.4979 | 564 | 262 |

Choice counts:

| Policy | Baseline | `signed_sum_s1` | Other movement sources |
| --- | ---: | ---: | ---: |
| `two_stage_p05` | 326 | 273 | 0 |
| `two_stage_train_f1` | 35 | 564 | 0 |

The source selector degenerates to `signed_sum_s1`. It does not learn the
M1309 all-safe source mix; it only decides how often to apply the same fixed
movement.

The move detector also has the wrong operating shape:

- `two_stage_p05` misses 138 oracle-move queries and falsely moves 149
  abstain queries.
- `two_stage_train_f1` recovers 243 of 262 move queries, but falsely moves
  321 abstain queries.

This explains the native result: higher movement recall is bought by unsafe
movement on abstain queries, while safer movement misses too much useful
movement and still harms rank metrics.

## Interpretation

M1311 is a useful negative result.

It shows that the M1309 all-safe oracle is not recoverable by a shallow
two-stage split over the same query-time feature family. This extends the
M1245 and M1253 conclusions:

- useful movement and action/source structure exist;
- the current observable features do not safely expose when to move;
- ordinary move gates, source routers, and threshold policies are now
  exhausted on this source.

This does not invalidate the positive signal:

- M1224/M1225 still show that CUB-specific target/harm construction improves
  atom quality;
- M1244 still shows that action-source routing is a strong structural upper
  bound;
- M1251/M1308 still show that `signed_sum_s1` is a real macro-positive
  query-time movement target;
- M1309 still shows that a sparse all-safe policy exists as an oracle.

But M1311 says the deployable approximation cannot be another selector over
the current feature view.

## Decision

Stop the current query-feature selector branch:

- do not scale M1311 to full `shared15`;
- do not train a deeper model with the same M1311 feature family;
- do not continue threshold/F1/precision-recall sweeps over the same output;
- do not promote fixed `signed_sum_s1` or the two-stage variants as default.

The next branch must move harm separation into the source or objective itself.

Recommended next step:

1. Build a source/objective construction that uses `signed_sum_s1` as a
   movement target and M1309 all-safe choices as sparse supervision.
2. Add explicit abstain pressure and false-move penalties during training,
   instead of applying a post-hoc move gate.
3. Require target/harm separability before native replay.
4. Run a small risk7 native replay only after the new source/objective shows
   separability.
5. Scale to full `shared15` only if risk7 improves over fixed `signed_sum_s1`
   without MRR/NDCG/CUB harm.

This keeps the retained signal but avoids another selector micro-tuning loop.
