# SAE M60 Scaled Supervised Stage-B Results Report

Date: 2026-05-21

Status: active, M60B source-expanded controls closed.

## Summary

M60 started by making the supervised Stage-B surface explicit and runnable on
Spark:

```text
train = M39 broad + nfcorpus-expanded source
eval  = full15 BEIR shared surface
host  = Spark / NVIDIA GB10
```

The first result is useful but not a product breakthrough. M60A confirms that
larger supervised data can slightly improve the M59 full15 supervised baseline,
especially on `trec-covid`, but the current query-side model family still does
not close the fixed-doc teacher gap. M60B added an official MS MARCO train
source and exposed an infrastructure issue: the trainer was parsing dense
embedding JSON that it does not need. The M60B all-slim frozen-control and
guarded-unfrozen runs are now closed. The source is integrated and runnable,
but it does not close the model blocker: frozen/head-only cannot absorb the new
labels, and guarded-unfrozen only gives small local gains while still trailing
M60A frozen and the teacher.

## Data Gate

The initial Spark cache was incomplete: `ii42_sae_beir15_shared` only
contained `nfcorpus` and `scifact`. M60 now treats source-registry counts as a
hard pre-run gate.

Final Spark registry:

| Role | Sources | Datasets | Docs | Queries | Qrel pairs |
| --- | ---: | ---: | ---: | ---: | ---: |
| `train` | 1 | 9 | 146353 | 4981 | 146212 |
| `eval` | 1 | 15 | 49059 | 1342 | 39742 |

Train source:

```text
m39_broad_plus_nfc
```

Eval source:

```text
beir15_current
```

M60B external-source registry adds official MS MARCO train labels:

| Role | Sources | Datasets | Docs | Queries | Qrel pairs |
| --- | ---: | ---: | ---: | ---: | ---: |
| `train` | 2 | 10 | 246353 | 6981 | 148340 |
| `eval` | 1 | 15 | 49059 | 1342 | 39742 |

The actual M60B train run applies `train_query_limit_per_dataset = 700`, so the
effective training scope is 10 datasets, 246353 docs, 3467 queries, and 52672
qrel pairs.

Operationally, M60B now uses slim artifacts for M39 broad and the official MS
MARCO source: `documents.jsonl` / `queries.jsonl` keep only id, text, metadata,
and qrels, while dense embeddings are removed. SAE latents remain in
`shared_sae_8192_64`. This fixes a pre-epoch bottleneck without changing the
training objective.

## Runs

### M60A unfrozen broad training

Path:

```text
results/sae/m60/spark-m60a-broad-full15-e8
```

Configuration:

```text
checkpoint = m54-e2-clean
train datasets = 9 M39 broad aliases
eval datasets = 15 BEIR aliases
epochs = 8
freeze_query_encoder = false
candidate_k = 100
max_candidates = 220
```

Training completed, but unfrozen broad training damaged full15 generalization.
The best source was `m31_fixed_w0p25`.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `m31_fixed_w0p25` | 0.7953 | 0.7981 | 0.6828 | 0.6432 |
| `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 |

Teacher gap:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.0494 |
| MRR@20 | -0.0418 |
| NDCG@10 | -0.0662 |
| MAP@100 | -0.0815 |

Physical cost:

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| `m31_fixed_w0p25` | 2968.9 | 10185.0 | 3287.8 |
| `teacher_fixed_doc` | 2986.2 | 10185.0 | 4522.8 |

Interpretation:

- It is cheaper than teacher on SAE postings.
- It is not better than M59 aggregate quality.
- It should not be extended with more epochs under the same objective.

### M60A frozen query-encoder control

Path:

```text
results/sae/m60/spark-m60a-broad-full15-frozen-e8
```

Configuration is the same as unfrozen M60A, except:

```text
freeze_query_encoder = true
```

The frozen-control result is the better M60A point. It keeps teacher shape more
stable and slightly beats M59 full15 supervised aggregate quality, but still
fails the product-research gate.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| M59 full15 supervised | 0.7980 | 0.8018 | 0.6838 | 0.6457 |
| `m31_fixed_w0p5` | 0.8076 | 0.8010 | 0.6884 | 0.6509 |
| `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 |

Teacher gap:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.0371 |
| MRR@20 | -0.0389 |
| NDCG@10 | -0.0606 |
| MAP@100 | -0.0738 |

Physical cost:

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| `m31_fixed_w0p5` | 3001.6 | 10185.0 | 4629.4 |
| `teacher_fixed_doc` | 2986.2 | 10185.0 | 4522.8 |

Hard-family note:

| Dataset | Metric | M59 | M60A frozen | Teacher |
| --- | --- | ---: | ---: | ---: |
| `trec-covid` | NDCG@10 | 0.7069 | 0.7556 | 0.8351 |
| `trec-covid` | MAP@100 | 0.5543 | 0.6236 | 0.7639 |
| `msmarco` | NDCG@10 | 0.6130 | 0.6134 | 0.7248 |
| `msmarco` | MAP@100 | 0.8464 | 0.8201 | 0.9352 |

Interpretation:

- The broader M39 source helps `trec-covid`.
- It does not fix `msmarco`, `fiqa`, `cqadupstack`, or `nq` ranking gaps.
- Freezing protects teacher shape, but current head-only adaptation cannot
  absorb the new supervision strongly enough.

### M60A shape-guard unfrozen training

Path:

```text
results/sae/m60/spark-m60a-broad-full15-shape-guard-e8
```

Configuration changes versus the baseline unfrozen run:

```text
learning_rate = 1e-4
head_learning_rate = 4e-4
teacher_loss_weight = 2.0
qrel_loss_weight = 0.05
bm25_preserve_weight = 0.50
support_aux_weight = 0.08
value_aux_weight = 0.16
scale_prior_weight = 0.04
```

Training behaved as intended internally: teacher loss dropped from `3.2065` to
`2.7436`, while total loss dropped from `25.0393` to `8.4830`. However, the
held-out full15 result did not beat the frozen-control.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `m31_fixed_w0p5` | 0.8023 | 0.8003 | 0.6858 | 0.6442 |
| `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 |

Physical cost:

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| `m31_fixed_w0p5` | 2982.7 | 10185.0 | 3946.5 |
| `teacher_fixed_doc` | 2986.2 | 10185.0 | 4522.8 |

Interpretation:

- Stronger teacher-shape anchoring prevents the worst unfrozen drift.
- It still underperforms frozen-control on aggregate quality.
- This makes another loss-only sweep on the same M39 surface low value.

### M60B official MS MARCO frozen-control

Path:

```text
results/sae/m60/spark-m60b-combined-msmarco-all-slim-frozen-e8-qcap
```

Configuration:

```text
checkpoint = m54-e2-clean
train datasets = 9 M39 broad slim aliases + 1 official MS MARCO slim alias
eval datasets = 15 BEIR aliases
epochs = 8
freeze_query_encoder = true
train_query_limit_per_dataset = 700
candidate_k = 100
max_candidates = 220
```

This run is a diagnostic, not a promotion candidate. Because the query encoder
is frozen and the selected best source is fixed-weight scoring, the best
aggregate result is unchanged from M60A frozen. The learned calibrated head
does improve versus the previous frozen calibrated head, but it remains below
the fixed `w0.5` path.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| M60A frozen `m31_calibrated` | 0.7980 | 0.7948 | 0.6797 | 0.6395 |
| M60B frozen `m31_calibrated` | 0.8007 | 0.8008 | 0.6861 | 0.6468 |
| M60B frozen `m31_fixed_w0p5` | 0.8076 | 0.8010 | 0.6884 | 0.6509 |
| `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 |

Teacher gap for the selected `m31_fixed_w0p5` path:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.0372 |
| MRR@20 | -0.0389 |
| NDCG@10 | -0.0606 |
| MAP@100 | -0.0738 |

Hard-family note:

| Dataset | Metric | BM25 | M60B frozen | Teacher |
| --- | --- | ---: | ---: | ---: |
| `trec-covid` | NDCG@10 | 0.6645 | 0.7556 | 0.8351 |
| `trec-covid` | MAP@100 | 0.4693 | 0.6236 | 0.7639 |
| `msmarco` | NDCG@10 | 0.5788 | 0.6134 | 0.7248 |
| `msmarco` | MAP@100 | 0.7893 | 0.8201 | 0.9352 |

Physical cost:

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| M60B frozen `m31_fixed_w0p5` | 3001.6 | 10185.0 | 4629.4 |
| `teacher_fixed_doc` | 2986.2 | 10185.0 | 4522.8 |

Interpretation:

- Official MS MARCO supervision is now integrated and runnable.
- Frozen/head-only training is not enough; it improves the calibrated head but
  cannot move fixed-weight retrieval or close the held-out `msmarco` gap.
- The next useful run is guarded-unfrozen with the same all-slim source
  registry, not another frozen/head-only pass.

### M60B official MS MARCO guarded-unfrozen

Path:

```text
results/sae/m60/spark-m60b-combined-msmarco-all-slim-shape-guard-e8-qcap
```

Configuration changes versus M60B frozen-control:

```text
freeze_query_encoder = false
learning_rate = 1e-4
head_learning_rate = 4e-4
teacher_loss_weight = 2.0
qrel_loss_weight = 0.05
bm25_preserve_weight = 0.50
support_aux_weight = 0.08
value_aux_weight = 0.16
scale_prior_weight = 0.04
```

Training behaved well internally: teacher loss improved from `3.1654` to
`2.3826`, BM25-preserve loss improved from `39.1196` to `4.1496`, and qrel
loss collapsed to near zero. That confirms the model can optimize this
objective, but the held-out full15 surface still does not beat the M60A/M60B
frozen-control baseline.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| M60A shape-guard `m31_fixed_w0p25` | 0.8007 | 0.7992 | 0.6868 | 0.6456 |
| M60B shape-guard `m31_fixed_w0p25` | 0.7997 | 0.7995 | 0.6868 | 0.6473 |
| M60B frozen `m31_fixed_w0p5` | 0.8076 | 0.8010 | 0.6884 | 0.6509 |
| `teacher_fixed_doc` | 0.8447 | 0.8399 | 0.7490 | 0.7247 |

Teacher gap for the selected `m31_fixed_w0p25` path:

| Metric | Gap |
| --- | ---: |
| Recall@100 | -0.0450 |
| MRR@20 | -0.0404 |
| NDCG@10 | -0.0622 |
| MAP@100 | -0.0774 |

Hard-family note:

| Dataset | Metric | M60A shape | M60B shape | Teacher |
| --- | --- | ---: | ---: | ---: |
| `msmarco` | NDCG@10 | 0.6120 | 0.6170 | 0.7248 |
| `msmarco` | MAP@100 | 0.8039 | 0.8107 | 0.9352 |
| `trec-covid` | NDCG@10 | 0.7357 | 0.7282 | 0.8351 |
| `trec-covid` | MAP@100 | 0.5748 | 0.5735 | 0.7639 |
| `cqadupstack` | NDCG@10 | 0.6134 | 0.6204 | 0.7589 |
| `cqadupstack` | MAP@100 | 0.5355 | 0.5461 | 0.7000 |

Physical cost:

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| M60B shape `m31_fixed_w0p25` | 2975.2 | 10185.0 | 4112.4 |
| `teacher_fixed_doc` | 2986.2 | 10185.0 | 4522.8 |

Interpretation:

- Official MS MARCO labels produce small local gains on `msmarco` and
  `cqadupstack` when the encoder can move.
- The same run weakens `trec-covid` and remains below frozen-control on the
  full15 aggregate.
- The limiting factor is no longer only data wiring. The current single-stage
  objective still converts qrels into near-zero training loss without producing
  enough held-out ranking gain.
- The next model step should be a true B0/B1 staged objective or a larger
  high-quality query supervision source such as TREC DL; do not keep repeating
  this single-stage guarded objective.

## Operational Notes

- Spark is usable for M60, but the first run exposed a missing full15 cache.
- Source-registry counts are now mandatory before training starts.
- `PYTHONUNBUFFERED=1` should be used for all future remote runs; otherwise
  epoch logs are delayed behind Docker/tmux pipes.
- GPU utilization during training was healthy, roughly 60-76%. The heavy CPU
  phase is candidate/example construction before epoch output.
- M60B exposed a second infrastructure issue: `documents.jsonl` contained dense
  embeddings that M31 never uses. M39 broad and official MS MARCO now have slim
  train artifacts; future M60 runs should use the all-slim registry.
- The official MS MARCO 100k-doc / 700-query train context still costs about
  475 seconds to build. If M60C scales beyond this, add ranking/postings cache
  or shard the source before increasing query count again.
- M31 now logs context loading and applies `train_query_limit_per_dataset`
  before expensive train-context ranking construction. This makes scaled runs
  observable and prevents scoring train queries that would later be discarded.

## Decision

M60A and M60B do not close the blocker.

Do not continue:

- more epochs on the unfrozen M60A objective;
- broad supervised unfrozen training without a stronger teacher-shape stage;
- treating M39 broad alone as sufficient new supervision.
- loss-only sweeps over the same M39 broad source without adding new labels or
  separating teacher-shape warmup from ranking residual training.
- frozen/head-only runs as a promotion path. They remain useful diagnostics but
  cannot absorb new human supervision into the query encoder.
- single-stage guarded-unfrozen training on the current source mix as a
  promotion path. It optimizes training losses but does not improve the full15
  aggregate enough.

Continue:

- source-registry-gated training;
- frozen/head-only controls as diagnostics;
- adding external human-qrel sources;
- explicit Stage B0 teacher-shape warmup before Stage B1 residual ranking.
- true two-stage B0/B1 training or a new external high-quality judged source.
  The next run should change the training structure or supervision quality, not
  just repeat the same objective.

## Next Steps

1. Split the objective into two visible stages, not just a weighted single
   training loop:
   B0 teacher-shape query warmup, then B1 residual qrel ranking.
2. Add the next external-source adapter only if it brings higher-quality labels
   than the current 2k-query MS MARCO source; prefer TREC DL next because it
   targets the same web/MS MARCO family with deeper judgments.
3. Add ranking/postings cache before increasing the official MS MARCO query
   cap beyond 700; otherwise context construction will dominate every run.
4. Keep full15 evaluation on every run. The M60A unfrozen run would have looked
   acceptable on small subsets but clearly fails on full15.
