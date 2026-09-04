# M1310 Query-Delta Oracle Ranker Smoke

## Question

M1309 proved that the `signed_sum` / `mean_teacher` movement target family has
query-level all-safe oracle capacity.  M1310 tests the first deployable
approximation:

> Can a LODO ranker select the M1309 `oracle_all_safe` source using only
> query-time movement-source features?

This is an observability/replay smoke.  It is not a large training run.

## Run

```bash
python3 scripts/train_m1310_query_delta_oracle_ranker.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --modes signed_sum,mean_teacher \
  --scales 0.5,0.75,1.0 \
  --output-root runs/m1310_query_delta_oracle_ranker_risk7_v1
```

Artifacts:

- Script: `scripts/train_m1310_query_delta_oracle_ranker.py`
- JSON: `runs/m1310_query_delta_oracle_ranker_risk7_v1/m1310_ranker.json`
- Markdown: `runs/m1310_query_delta_oracle_ranker_risk7_v1/m1310_ranker.md`

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
| `predicted_ranker` | 0.000 | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 0.000000 |

The ranker predicted baseline for every query.

## Fold Diagnostics

| Heldout | Exact | MovePrecision | MoveRecall | PredMove | OracleMove |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 0.7300 | 0.0000 | 0.0000 | 0 | 27 |
| `fiqa` | 0.8400 | 0.0000 | 0.0000 | 0 | 16 |
| `webis-touche2020` | 0.6122 | 0.0000 | 0.0000 | 0 | 19 |
| `nfcorpus` | 0.5200 | 0.0000 | 0.0000 | 0 | 48 |
| `dbpedia-entity` | 0.3600 | 0.0000 | 0.0000 | 0 | 64 |
| `scidocs` | 0.3900 | 0.0000 | 0.0000 | 0 | 61 |
| `trec-covid` | 0.4600 | 0.0000 | 0.0000 | 0 | 27 |

Aggregate diagnostics:

- exact match: `0.562604`
- predicted movement queries: `0`
- oracle movement queries: `262`
- move precision: `0.0`
- move recall: `0.0`

The exact match number is misleading.  It comes only from abstain queries.

## Interpretation

M1310 fails the first deployable approximation gate.

This does not invalidate M1309.  The oracle capacity remains strong.  It says
the naive single binary candidate ranker cannot approximate the abstain-heavy
all-safe oracle from the current feature view.

The failure mode is useful:

- the model collapses to the conservative baseline choice;
- no harmful movement is introduced;
- but all useful movement is missed;
- therefore deeper training on the same all-in-one candidate ranker is not
  justified.

## Decision

Do not scale M1310 to full shared15.

Do not train a deeper model with the exact same single-stage binary ranker.

The next valid experiment must change the approximation structure, not just
the model size:

1. train a move/abstain detector separately;
2. train a source selector only among oracle-move queries;
3. evaluate detector recall and false-move rate before native replay;
4. replay only if predicted movement recall is non-zero and false movement is
   bounded.

The retained route is still:

> M1309 all-safe oracle capacity -> learn a constrained query-level objective.

But the first approximation must be two-stage or structured; the all-in-one
candidate ranker is too conservative.
