# M1309 Query-Delta Oracle Upper Bound

## Question

M1308 showed that `signed_sum_s1` is the strongest fixed query-delta movement
target on full `shared15`, but it still has row-level harm.  M1309 asks the
capacity question before training:

> Does the existing `signed_sum` / `mean_teacher` movement target family contain
> a row-safe query-level policy if an oracle can choose when to move?

This is an upper-bound audit, not a deployable selector.  It determines whether
the next stage should train a constrained objective over this target family or
abandon it for a new source.

## Runs

Risk7 smoke:

```bash
python3 scripts/audit_m1309_query_delta_oracle_upper_bound.py \
  --datasets cqadupstack,fiqa,webis-touche2020,nfcorpus,dbpedia-entity,scidocs,trec-covid \
  --modes signed_sum,mean_teacher \
  --scales 0.5,0.75,1.0 \
  --output-root runs/m1309_query_delta_oracle_risk7_v1
```

Full `shared15`:

```bash
python3 scripts/audit_m1309_query_delta_oracle_upper_bound.py \
  --datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,cqadupstack,webis-touche2020,climate-fever,dbpedia-entity,fever,hotpotqa,msmarco,nq,quora \
  --modes signed_sum,mean_teacher \
  --scales 0.5,0.75,1.0 \
  --output-root runs/m1309_query_delta_oracle_full_shared15_v1
```

Artifacts:

- Script: `scripts/audit_m1309_query_delta_oracle_upper_bound.py`
- Risk7 JSON: `runs/m1309_query_delta_oracle_risk7_v1/m1309_oracle.json`
- Risk7 Markdown: `runs/m1309_query_delta_oracle_risk7_v1/m1309_oracle.md`
- Full JSON:
  `runs/m1309_query_delta_oracle_full_shared15_v1/m1309_oracle.json`
- Full Markdown:
  `runs/m1309_query_delta_oracle_full_shared15_v1/m1309_oracle.md`

## Oracle Definitions

All oracles choose among:

- baseline / abstain
- `signed_sum_s0.5`, `signed_sum_s0.75`, `signed_sum_s1`
- `mean_teacher_s0.5`, `mean_teacher_s0.75`, `mean_teacher_s1`

Policies:

- `oracle_max_utility`: choose the largest query-level utility, even if a
  metric is negative.
- `oracle_rank_safe`: choose only variants with non-negative Recall, MAP,
  NDCG, and MRR deltas.
- `oracle_all_safe`: choose only variants with all five metric deltas
  non-negative, including candidate upper bound.

## Risk7 Result

Risk7 query count: `599`.

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_max_utility` | 6.781 | 0 | +0.003437 | +0.004906 | +0.006095 | +0.002613 | +0.002841 | 0.052159 |
| `oracle_rank_safe` | 3.469 | 0 | +0.003392 | +0.004851 | +0.005952 | +0.002608 | +0.002156 | 0.050787 |
| `oracle_all_safe` | 3.391 | 0 | +0.003217 | +0.004783 | +0.005622 | +0.002608 | +0.002423 | 0.049318 |
| `mean_teacher_s0.75` | 7.673 | 0 | +0.001291 | +0.001114 | +0.001019 | +0.000067 | +0.000037 | 0.012005 |
| `signed_sum_s1` | 7.816 | 1 | +0.001848 | +0.001444 | +0.001789 | -0.000961 | +0.000059 | 0.005682 |

`oracle_all_safe` is much stronger than the best fixed risk7 policy while
remaining macro-clean.

Risk7 `oracle_all_safe` choice distribution:

| Source | Queries |
| --- | ---: |
| `baseline` | 337 |
| `signed_sum_s1` | 129 |
| `signed_sum_s0.5` | 44 |
| `signed_sum_s0.75` | 39 |
| `mean_teacher_s0.5` | 19 |
| `mean_teacher_s1` | 17 |
| `mean_teacher_s0.75` | 14 |

The shape is abstain-heavy, which matches the desired product form: move only
when a constrained objective expects safe utility.

## Full Shared15 Result

Full query count: `1342`.

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_max_utility` | 7.361 | 0 | +0.003471 | +0.006280 | +0.006795 | +0.006016 | +0.001452 | 0.063274 |
| `oracle_rank_safe` | 2.239 | 0 | +0.003466 | +0.006255 | +0.006752 | +0.006020 | +0.001256 | 0.062898 |
| `oracle_all_safe` | 2.212 | 0 | +0.003424 | +0.006222 | +0.006556 | +0.006020 | +0.001341 | 0.062275 |
| `signed_sum_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | 0.030620 |
| `mean_teacher_s1` | 7.697 | 0 | +0.001653 | +0.002902 | +0.002442 | +0.002807 | +0.000017 | 0.027485 |
| `signed_sum_s0.75` | 7.975 | 0 | +0.000943 | +0.003131 | +0.002927 | +0.003098 | +0.000145 | 0.026302 |

The full result confirms the risk7 signal.  The movement family has a strong
safe oracle upper bound:

- `oracle_all_safe` roughly doubles the `signed_sum_s1` score;
- all five macro metrics stay positive;
- mean selected count drops from `7.975` to `2.212`, so the oracle is not just
  applying the same movement everywhere;
- the gap between `oracle_rank_safe` and `oracle_all_safe` is small, meaning
  CUB safety does not destroy the objective.

Full `oracle_all_safe` choice distribution:

| Source | Queries |
| --- | ---: |
| `baseline` | 967 |
| `signed_sum_s1` | 186 |
| `signed_sum_s0.75` | 72 |
| `signed_sum_s0.5` | 70 |
| `mean_teacher_s0.5` | 19 |
| `mean_teacher_s1` | 15 |
| `mean_teacher_s0.75` | 13 |

The useful action is sparse.  About `72%` of queries should abstain under the
all-safe oracle.  This explains why fixed replay policies are row-fragile:
they move many queries that should not move.

## Interpretation

M1309 is a real positive upper-bound signal.

Recent negative results did not mean the movement target family was dead.  They
meant fixed replay and post-hoc source selection were the wrong interface.
M1309 shows the target family has enough safe capacity if a query-level
objective learns when to abstain and which movement target to use.

The key change in direction is:

- stop `atom selector -> replay -> guard`;
- train `query-level constrained objective -> sparse movement target choice`.

This aligns with the review: useful atoms and action structures exist, but
safe selection must become part of the objective/source construction, not a
post-processing gate.

## Decision

Proceed to M1310.

M1310 should train a deployable approximation of `oracle_all_safe`, starting
small:

1. Labels: `baseline` versus the six fixed movement targets, using the M1309
   all-safe oracle as teacher.
2. Features: only query-time/native-observable features.  No qrels, no oracle
   metric deltas at inference.
3. First gate: LODO classification/ranking accuracy on risk7.
4. Second gate: native replay using predicted choices on risk7.
5. Scale only if predicted replay keeps macro gain and reduces row harm versus
   fixed `signed_sum_s1`.

Stop conditions:

- if LODO cannot distinguish abstain from movement better than a conservative
  baseline;
- if predicted replay collapses to abstain-only;
- if predicted replay reintroduces row-level harm close to fixed `signed_sum_s1`;
- if gains require dataset-specific thresholds.

Do not train a large model yet.  M1310 should first prove the all-safe oracle is
observable enough to approximate.
