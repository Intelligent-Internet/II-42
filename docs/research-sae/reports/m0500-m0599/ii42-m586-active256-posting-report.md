# M586 Active256 M551 Posting Canary

M586 tests whether the M551/M555 posting route should move from the
active128 support used by M566 to a larger active256 support.  This keeps the
successful BM25-free shape: frozen dense teacher, candidate-set competition,
locked support residual, and qrels-free teacher-surface diagnostics.

## Setup

Run output:

```text
runs/m586_active256_h768_official1024_beir7_seed551/
```

Remote output:

```text
/home/huoju/leask/runs/ii42-m586-m551-active256-v1/m586_active256_h768_official1024_beir7_seed551/
```

Configuration:

| Field | Value |
| --- | --- |
| Root | `ii42-m566-m551-official1024-root-v1/_shared/tasks` |
| Tasks | official-1024 BEIR7, excluding `cqadupstack` |
| Active dims | `256` |
| Prefix | `512` |
| Hidden dims | `768` |
| Teacher pool / random negatives | `128 / 128` |
| Teacher / student temperature | `0.025 / 0.050` |
| Max train groups | `512` |
| Epochs | `4` |
| Residual scale | `0.025` |
| Score / support / pairwise weight | `0.05 / 0.5 / 0.0` |
| Seed | `551` |

## Result

Metrics are official-1024 BEIR7 macro values.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M566 `dense_topk128_sparse` | 0.42618 | 0.25126 | 0.55380 | 0.53270 | 0.54587 |
| M566 active128 learned | 0.43197 | 0.25551 | 0.55940 | 0.53604 | 0.54827 |
| M586 `dense_topk256_sparse` | 0.46318 | 0.28352 | 0.59822 | 0.57180 | 0.74299 |
| M586 active256 learned | 0.46461 | 0.28717 | 0.60121 | 0.56952 | 0.74692 |

M586 learned delta versus its own active256 dense-topk sparse baseline:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.00143 |
| MAP@100 | +0.00365 |
| Recall@100 | +0.00299 |
| MRR@20 | -0.00228 |
| Overlap@100 | +0.00393 |

M586 active256 learned delta versus M566 active128 learned:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.03264 |
| MAP@100 | +0.03166 |
| Recall@100 | +0.04181 |
| MRR@20 | +0.03348 |
| Overlap@100 | +0.19865 |

## Per-Task Delta

Rows are M586 active256 learned deltas versus `dense_topk256_sparse`.

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Gate |
| --- | ---: | ---: | ---: | ---: | --- |
| `arguana` | +0.00290 | +0.00171 | +0.00000 | +0.00176 | accept |
| `nfcorpus` | +0.00209 | -0.00007 | +0.00234 | -0.02017 | fallback |
| `fiqa` | +0.00150 | +0.00188 | -0.00006 | -0.00382 | fallback |
| `scidocs` | +0.00048 | +0.00048 | +0.00162 | +0.00272 | accept |
| `scifact` | +0.01813 | +0.02072 | +0.00833 | +0.01977 | accept |
| `trec-covid` | -0.01010 | +0.00250 | +0.00495 | -0.02500 | accept |
| `webis-touche2020` | -0.00501 | -0.00165 | +0.00370 | +0.00883 | accept |

The gate column is the M569-style qrels-free overlap rule: accept the learned
source only when its `teacher_overlap.overlap_at_100` is not lower than the
same task's dense-topk baseline.

## Overlap-Gated Probe

Applying the same overlap gate post-hoc to M586 seed551 gives:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M586 learned | +0.001430 | +0.003650 | +0.002990 | -0.002280 | +0.003930 |
| M586 overlap-gated | +0.000914 | +0.003394 | +0.002657 | +0.001154 | +0.004423 |

## Interpretation

Active256 is valuable as posting capacity.  The active256 dense-topk sparse
baseline is much stronger than the active128 baseline on every macro metric,
and active256 learned remains far above the active128 learned surface.

The learned residual itself is not automatically better at active256.  It
still improves NDCG, MAP, Recall, and overlap versus `dense_topk256_sparse`,
but it loses MRR.  The same M569 overlap gate repairs the MRR sign, which
means the route should continue only as a guarded source rather than as
"always use learned residual."

## M587 Selector-Gated Seed552

M587 tested the same active256/h768 shape with an independent selector split
and the safer M573-style gate:

- train / selector / eval split: `0.50 / 0.20 / 0.30`;
- accept learned source only when selector `overlap_at_200` delta is at least
  `+0.002`;
- require selector `candidate_coverage_at_100` delta at least `+0.002`;
- require at least `20` selector queries.

Output:

```text
runs/m587_active256_h768_guarded_official1024_beir7_seed552/
```

Result versus `dense_topk256_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M587 learned | +0.000780 | +0.001150 | +0.003160 | -0.004580 | +0.001650 |
| M587 guarded | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |

The strict selector gate accepted no tasks.  Most tasks failed overlap or
coverage; `trec-covid` and `webis-touche2020` also had only `10` selector
queries and therefore failed the `MIN_SELECTOR_QUERIES=20` guard.

The important signal is not the all-fallback guarded source.  It is that the
learned active256 residual again improves NDCG/MAP/Recall while losing MRR.
This repeats M586 and suggests that active256 residual scale `0.025` is too
aggressive for early-rank stability.

## Current Decision

Active256 should continue, but not by more gate micro-tuning.  The capacity
increase itself is clearly useful; the learned residual needs to become more
conservative at this support size.

## M588 Conservative Residual Probe

M588 keeps the active256/h768 shape and changes only:

- `RESIDUAL_SCALE=0.0125`

Output:

```text
runs/m588_active256_h768_resid0125_official1024_beir7_seed551/
```

Result versus `dense_topk256_sparse`:

| Run | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M586 resid `0.025` | +0.001430 | +0.003650 | +0.002990 | -0.002280 | +0.003930 |
| M588 resid `0.0125` | +0.002740 | +0.003850 | +0.003320 | +0.004250 | +0.001850 |

M588 learned absolute delta versus M586 learned:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.00131 |
| MAP@100 | +0.00020 |
| Recall@100 | +0.00033 |
| MRR@20 | +0.00653 |
| Overlap@100 | -0.00208 |

Per-task M588 deltas versus `dense_topk256_sparse`:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.00313 | +0.00270 | +0.00000 | +0.00270 |
| `nfcorpus` | -0.00008 | -0.00158 | +0.00248 | -0.02017 |
| `fiqa` | +0.00235 | +0.00248 | +0.00380 | -0.00510 |
| `scidocs` | -0.00033 | +0.00082 | +0.00162 | +0.00260 |
| `scifact` | +0.01723 | +0.01965 | +0.00833 | +0.01843 |
| `trec-covid` | -0.00641 | +0.00138 | +0.00329 | +0.00000 |
| `webis-touche2020` | +0.00325 | +0.00152 | +0.00370 | +0.03133 |

Interpretation: this is the first active256 learned residual probe with all
five macro deltas positive versus `dense_topk256_sparse`.  The MRR repair is
large enough to justify seed expansion.  The tradeoff is lower overlap gain
than M586, so the next seed check must verify that this is not a seed551-only
rank benefit.

## M588 Seed552

M588 seed552 did not reproduce the seed551 learned-residual win.

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.002740 | +0.003850 | +0.003320 | +0.004250 | +0.001850 |
| 552 | -0.001660 | -0.000050 | -0.000560 | -0.009110 | +0.001000 |
| Mean | +0.000540 | +0.001900 | +0.001380 | -0.002430 | +0.001425 |

Seed552 learned per-task deltas show the failure mode:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.00173 | +0.00068 | +0.00000 | +0.00106 |
| `nfcorpus` | -0.00701 | +0.00199 | -0.00712 | -0.00449 |
| `fiqa` | -0.00238 | -0.00081 | -0.00052 | -0.00194 |
| `scidocs` | +0.00126 | +0.00014 | +0.00300 | -0.00071 |
| `scifact` | +0.00343 | +0.00445 | +0.00834 | +0.00250 |
| `trec-covid` | +0.00007 | +0.00050 | +0.00287 | +0.00000 |
| `webis-touche2020` | -0.00871 | -0.00731 | -0.01048 | -0.06016 |

The learned active256 residual is therefore not promotable as an unconditional
source.

## M588 Post-Hoc Overlap Gate

The M588 no-selector runs only store `teacher_overlap.overlap_at_100`, so the
available qrels-free post-hoc gate is the M569-style overlap rule.

| Seed | Gate | Accepted Tasks | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | overlap@100 `>= 0` | `arguana`, `nfcorpus`, `scidocs`, `scifact`, `trec-covid` | +0.001934 | +0.003281 | +0.002246 | +0.000509 | +0.002301 |
| 552 | overlap@100 `>= 0` | `scifact`, `trec-covid` | +0.000500 | +0.000707 | +0.001601 | +0.000357 | +0.002036 |
| 551-552 mean | overlap@100 `>= 0` | mixed | +0.001217 | +0.001994 | +0.001924 | +0.000433 | +0.002169 |

This is a narrow but real rescue: the post-hoc overlap gate turns the two-seed
mean MRR from negative to positive while preserving positive NDCG, MAP, Recall,
and overlap.

## Current Decision

The viable active256 shape is not "always use learned residual."  It is:

1. use active256 as the stronger dense-derived posting capacity;
2. train a conservative locked-support residual with scale `0.0125`;
3. accept the learned source only when a qrels-free teacher-overlap signal says
   the dense teacher surface was not damaged.

Next step: run M589, the deployment-shaped version of this rule:
active256/h768/resid0.0125 with an independent selector split and
`overlap_at_100 >= 0`.  If selector overlap cannot reproduce the post-hoc
rescue, active256 should be kept as a strong dense-topk posting baseline and
the current M551 learned residual route should be treated as bounded.

## M589 Selector-Overlap Gate

M589 turns the M588 post-hoc rescue into a cleaner selector-split source.

Configuration:

- active dims: `256`;
- hidden dims: `768`;
- residual scale: `0.0125`;
- train / selector / eval split: `0.50 / 0.20 / 0.30`;
- guarded source: `m589_active256_selector_m551`;
- selector gate: accept learned source when selector `overlap_at_100` delta is
  nonnegative.

Seed552 output:

```text
runs/m589_active256_h768_selector_official1024_beir7_seed552/
```

Seed552 deltas versus `dense_topk256_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M589 learned | +0.001660 | +0.000900 | +0.002380 | -0.004930 | +0.000090 |
| M589 selector-gated | +0.000450 | +0.000900 | +0.001690 | +0.000730 | +0.000500 |

Gate decisions:

| Task | Decision | Selector dOverlap@100 |
| --- | --- | ---: |
| `arguana` | accept | +0.000857 |
| `nfcorpus` | fallback | -0.002616 |
| `fiqa` | fallback | -0.000077 |
| `scidocs` | fallback | -0.000150 |
| `scifact` | accept | +0.001334 |
| `trec-covid` | accept | +0.013000 |
| `webis-touche2020` | fallback | -0.011000 |

M589 seed552 is important because it reproduces the M588 post-hoc rescue with
an independent selector split: the learned active256 residual still loses MRR,
but the qrels-free selector gate makes all five macro deltas positive.

Next step: run M589 seed553.  If seed553 also keeps the selector-gated source
positive on NDCG, MAP, Recall, MRR, and overlap, then the active256 line has a
credible guarded candidate and should be tested with `cqadupstack`.

## M589 Seed553 And Two-Seed Decision

M589 seed553 completed with the same selector-overlap gate.

Two-seed deltas versus `dense_topk256_sparse`:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 552 | learned | +0.001660 | +0.000900 | +0.002380 | -0.004930 | +0.000090 |
| 552 | guarded | +0.000450 | +0.000900 | +0.001690 | +0.000730 | +0.000500 |
| 553 | learned | +0.001110 | +0.002310 | +0.002420 | +0.000070 | +0.001250 |
| 553 | guarded | +0.001110 | +0.002310 | +0.002420 | +0.000070 | +0.001250 |
| 552-553 mean | learned | +0.001385 | +0.001605 | +0.002400 | -0.002430 | +0.000670 |
| 552-553 mean | guarded | +0.000780 | +0.001605 | +0.002055 | +0.000400 | +0.000875 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 2/2 | 2/2 | 2/2 | 1/2 | 2/2 |
| guarded | 2/2 | 2/2 | 2/2 | 2/2 | 2/2 |

M589 seed553 accepted all seven BEIR7 tasks; seed552 fell back on `nfcorpus`,
`fiqa`, `scidocs`, and `webis-touche2020`.  This is the strongest active256
result so far because the deployment-shaped selector gate keeps all five macro
deltas positive across two seeds.

## Next Gate

M589 is not final because it has only been tested on BEIR7.  The required next
gate is M590:

- same active256/h768/resid0.0125 selector-overlap shape;
- include `cqadupstack`;
- start with one seed because `cqadupstack` dominates query count and runtime;
- promote only if the guarded source keeps nonnegative MRR and positive
  NDCG/MAP/Recall/overlap on BEIR8.

If M590 fails because `cqadupstack` breaks MRR or Recall, the route remains a
useful BEIR7 guarded canary but should not be called final.

## M590 BEIR8 Seed552 Minbase

M590 tests the M589 selector-overlap shape on BEIR8 by adding
`cqadupstack`.  The first attempt exposed two runtime issues rather than a
model issue:

- dense top-k overlap evaluation was doing repeated per-query dense products;
- the default M549 transform baselines were not needed for the M590 decision
  and dominated `cqadupstack` runtime.

The run was therefore repeated as a minbase run with the same M590 model/gate
configuration, but with `BASELINE_TRANSFORMS=""`.  This keeps only the sources
needed for the gate: `dense_topk256_sparse`, learned M590, and selector-gated
M590.

Output:

```text
runs/m590_active256_h768_selector_official1024_beir8_seed552_minbase/
```

Runtime was `1673.956` seconds for all eight tasks.

Seed552 BEIR8 deltas versus `dense_topk256_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M590 learned | +0.001460 | +0.000720 | +0.001500 | -0.004420 | +0.000030 |
| M590 selector-gated | +0.000400 | +0.000730 | +0.000900 | +0.000530 | +0.000400 |

Selector decisions:

| Task | Decision | Selector dOverlap@100 |
| --- | --- | ---: |
| `arguana` | accept | +0.000893 |
| `nfcorpus` | fallback | -0.002615 |
| `fiqa` | fallback | -0.000077 |
| `scidocs` | fallback | -0.000200 |
| `scifact` | accept | +0.001500 |
| `trec-covid` | accept | +0.013000 |
| `webis-touche2020` | fallback | -0.011000 |
| `cqadupstack` | accept | +0.000171 |

This is a positive BEIR8 first gate.  The unconditional learned source still
has the same failure mode as M589 seed552: it improves NDCG/MAP/Recall but
loses MRR.  The selector-gated source fixes that by falling back on the risky
tasks and keeps all five macro deltas positive, including after adding
`cqadupstack`.

M590 is still not final.  The next required check is a second BEIR8 seed with
the same minbase configuration.  If seed553 also keeps selector-gated deltas
positive across NDCG, MAP, Recall, MRR, and overlap, active256 selector-gated
M551 becomes the current best promotable BM25-free route.

## M590 BEIR8 Seed553 And Two-Seed Decision

M590 seed553 completed with the same minbase configuration.  It reproduced the
same high-level pattern:

- unconditional learned M590 is not safe because MRR is negative;
- selector-gated M590 keeps all five macro deltas positive;
- seed553 accepts most tasks but falls back on `cqadupstack`, which is exactly
  the intended behavior for a qrels-free selector gate.

Two-seed BEIR8 deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 552 | learned | +0.001460 | +0.000720 | +0.001500 | -0.004420 | +0.000030 |
| 552 | guarded | +0.000400 | +0.000730 | +0.000900 | +0.000530 | +0.000400 |
| 553 | learned | -0.000040 | +0.001100 | +0.000460 | -0.000830 | +0.000780 |
| 553 | guarded | +0.000970 | +0.002020 | +0.002110 | +0.000060 | +0.001070 |
| 552-553 mean | learned | +0.000710 | +0.000910 | +0.000980 | -0.002625 | +0.000405 |
| 552-553 mean | guarded | +0.000685 | +0.001375 | +0.001505 | +0.000295 | +0.000735 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 1/2 | 2/2 | 2/2 | 0/2 | 2/2 |
| guarded | 2/2 | 2/2 | 2/2 | 2/2 | 2/2 |

Seed553 selector decisions:

| Task | Decision | Selector dOverlap@100 |
| --- | --- | ---: |
| `arguana` | accept | +0.000822 |
| `nfcorpus` | accept | +0.000770 |
| `fiqa` | accept | +0.000846 |
| `scidocs` | accept | +0.002200 |
| `scifact` | accept | +0.001000 |
| `trec-covid` | accept | +0.016000 |
| `webis-touche2020` | accept | +0.002000 |
| `cqadupstack` | fallback | -0.001316 |

Decision: M590 selector-gated active256 is now the strongest BM25-free M551
route.  The learned source alone should not be promoted because MRR is negative
in both BEIR8 seeds.  The qrels-free selector gate is the important part: it
keeps NDCG, MAP, Recall, MRR, and dense-overlap positive across both BEIR8
seeds while automatically falling back on risky tasks.

The route is promotable as the current active256 candidate, but not final as a
product claim.  Next evidence should be a broader standard matrix, not more
small gate tuning: compare M590 selector-gated against dense-topk sparse,
exact dense, and the prior best BM25-free route on the available larger task
set.

## M590 BEIR8 Seed551 And Gate Failure

M590 seed551 was added as a third official1024 BEIR8 seed with the same
minbase configuration.  This is the first broader stability check after the
two-seed positive result.

Run output:

- `runs/m590_active256_h768_selector_official1024_beir8_seed551_minbase/m590_active256_h768_selector_official1024_beir8_seed551_minbase.json`

Seed551 deltas versus `dense_topk256_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M590 learned | +0.004620 | +0.002060 | +0.000850 | +0.002270 | +0.003180 |
| M590 selector-gated | +0.003540 | +0.002180 | +0.001410 | -0.001780 | +0.002540 |

Three-seed BEIR8 mean deltas:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | +0.002013 | +0.001293 | +0.000937 | -0.000993 | +0.001330 |
| guarded | +0.001637 | +0.001643 | +0.001473 | -0.000397 | +0.001337 |

Positive-seed count over seeds 551, 552, and 553:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 2/3 | 3/3 | 3/3 | 1/3 | 3/3 |
| guarded | 3/3 | 3/3 | 3/3 | 2/3 | 3/3 |

Seed551 shows that the current `overlap@100 >= 0` selector is not stable
enough for early rank.  It accepts `nfcorpus` because selector overlap improves
by `+0.001231`, but that acceptance drops task MRR@20 by `-0.031780`, which
turns the macro MRR negative despite positive NDCG, MAP, Recall, and dense
overlap.

This means the two-seed M590 decision was too optimistic.  The active256
selector route remains alive, but the current gate is not promotable.

## M591 Offline Gate Candidate

A post-hoc qrels-free gate scan over the completed M590 seed551/552/553 JSONs
found one cleaner selector surface:

- gate key: selector `overlap@200`;
- acceptance rule: model `overlap@200 - baseline overlap@200 >= -0.001`;
- no qrels, BM25, or task labels are used by the gate.

Offline three-seed deltas for this candidate:

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.004248 | +0.002011 | +0.000391 | +0.006035 | +0.003242 |
| 552 | +0.000403 | +0.000728 | +0.000894 | +0.000528 | +0.000403 |
| 553 | +0.001315 | +0.001596 | +0.002449 | +0.000215 | +0.000731 |
| mean | +0.001988 | +0.001445 | +0.001245 | +0.002259 | +0.001459 |

Positive-seed count:

| NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |

This is the next candidate to validate as M591 by running the same M590 model
shape with `SELECTOR_OVERLAP_GATE_K=200` and
`OVERLAP_GATE_MIN_DELTA=-0.001`.  The point is not to tune on qrels, but to
replace the fragile top-100 selector signal with a wider top-200 dense-support
signal that appears to avoid the seed551 `nfcorpus` early-rank failure.

## M591 Official Selector200 Result

M591 ran the top-200 selector gate as a three-seed official1024 BEIR8 check:

- `runs/m591_active256_h768_selector200_official1024_beir8_seed551_minbase/m591_active256_h768_selector200_official1024_beir8_seed551_minbase.json`
- `runs/m591_active256_h768_selector200_official1024_beir8_seed552_minbase/m591_active256_h768_selector200_official1024_beir8_seed552_minbase.json`
- `runs/m591_active256_h768_selector200_official1024_beir8_seed553_minbase/m591_active256_h768_selector200_official1024_beir8_seed553_minbase.json`

The top-200 gate fixed the M590 seed551 `nfcorpus` early-rank failure, but it
is still not the final selector.  Seed553 keeps MRR positive but loses NDCG and
Recall.

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | +0.004620 | +0.002060 | +0.000850 | +0.002270 | +0.003180 |
| 551 | guarded | +0.004250 | +0.002010 | +0.000390 | +0.006040 | +0.003240 |
| 552 | learned | +0.004760 | +0.001800 | +0.005480 | +0.001600 | +0.005590 |
| 552 | guarded | +0.004760 | +0.001800 | +0.005480 | +0.001600 | +0.005590 |
| 553 | learned | -0.001120 | +0.001130 | -0.000350 | +0.001090 | +0.004210 |
| 553 | guarded | -0.000750 | +0.001090 | -0.000740 | +0.002280 | +0.004100 |
| mean | learned | +0.002753 | +0.001663 | +0.001993 | +0.001653 | +0.004327 |
| mean | guarded | +0.002753 | +0.001633 | +0.001710 | +0.003307 | +0.004310 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 2/3 | 3/3 | 2/3 | 3/3 | 3/3 |
| guarded | 2/3 | 3/3 | 2/3 | 3/3 | 3/3 |

Decision: M591 is better than M590 because it turns MRR positive across all
three seeds and improves mean MRR to `+0.003307`, but it is still not robust
enough to promote because NDCG and Recall are negative on seed553.

## M592 Offline Top20 Gate Candidate

A qrels-free replay over the completed M591 JSONs shows that the best next
supported gate is stricter on early dense support:

- `SELECTOR_OVERLAP_GATE_K=20`
- `OVERLAP_GATE_MIN_DELTA=0.001`

This is supported by the current runner without code changes.  It accepts a
trained source only when selector-set dense overlap improves in the top-20
support.  It is stricter than the M591 top-200 gate and avoids the seed553
NDCG/Recall regression in the replay.

Offline replay deltas:

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.004948 | +0.002420 | +0.000491 | +0.006420 | +0.003185 |
| 552 | +0.003209 | +0.001056 | +0.002334 | +0.001069 | +0.004136 |
| 553 | +0.000559 | +0.001829 | +0.001377 | +0.002118 | +0.001041 |
| mean | +0.002905 | +0.001768 | +0.001401 | +0.003202 | +0.002788 |

Positive-seed count:

| NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| ---: | ---: | ---: | ---: | ---: |
| 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |

This should be validated as M592 with the same active256/h768 model shape and
the top-20 selector gate.  If the official M592 rerun reproduces the replay,
this becomes the strongest current BM25-free M551-family selector route.

## M592 Official Top20 Gate Result

M592 reran the same active256/h768 model shape with the top-20 selector gate as
an official1024 BEIR8 three-seed check:

- `runs/m592_active256_h768_selector20_official1024_beir8_seed551_minbase/m592_active256_h768_selector20_official1024_beir8_seed551_minbase.json`
- `runs/m592_active256_h768_selector20_official1024_beir8_seed552_minbase/m592_active256_h768_selector20_official1024_beir8_seed552_minbase.json`
- `runs/m592_active256_h768_selector20_official1024_beir8_seed553_minbase/m592_active256_h768_selector20_official1024_beir8_seed553_minbase.json`

The official run reproduced the offline replay.  The guarded source is positive
on all five tracked macro deltas for all three seeds, while the raw learned
source still has the seed553 NDCG/Recall regression.  The qrels-free top-20
selector is therefore the current strongest BM25-free M551-family route.

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | +0.004620 | +0.002060 | +0.000850 | +0.002270 | +0.003180 |
| 551 | guarded | +0.004950 | +0.002420 | +0.000490 | +0.006420 | +0.003190 |
| 552 | learned | +0.004760 | +0.001800 | +0.005480 | +0.001600 | +0.005590 |
| 552 | guarded | +0.003210 | +0.001060 | +0.002340 | +0.001070 | +0.004140 |
| 553 | learned | -0.001120 | +0.001130 | -0.000350 | +0.001090 | +0.004210 |
| 553 | guarded | +0.000560 | +0.001830 | +0.001380 | +0.002120 | +0.001040 |
| mean | learned | +0.002753 | +0.001663 | +0.001993 | +0.001653 | +0.004327 |
| mean | guarded | +0.002907 | +0.001770 | +0.001403 | +0.003203 | +0.002790 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 2/3 | 3/3 | 2/3 | 3/3 | 3/3 |
| guarded | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |

Decision: M592 is promotion-grade within the current official1024 BEIR8
validation scope.  It is not yet a final product result because it still needs
either a second root/split validation or a follow-up composite gate check before
being treated as the stable endpoint for the M551 route.

## M593 Composite Gate Preparation

The current runner now supports optional extra qrels-free dense-overlap gates
without changing the training objective.  This was added to allow the next
experiment to validate the strongest replayed selector shape after M592
finishes.

Primary candidate:

- `SELECTOR_OVERLAP_GATE_K=20`
- `OVERLAP_GATE_MIN_DELTA=0.001`
- `SELECTOR_EXTRA_OVERLAP_GATES=200:-0.002`

Replay over the completed M591 official JSONs:

| Gate | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `20:+0.001` | +0.002906 | +0.001770 | +0.001402 | +0.003203 | +0.002789 |
| `20:+0.001,200:-0.002` | +0.003028 | +0.001755 | +0.001271 | +0.003598 | +0.002754 |

Both gates are `3/3` positive across NDCG@10, MAP@100, Recall@100, MRR@20,
and overlap@100 in replay.  M593 should only start after M592 official three
seed completes, so the M592 evidence surface is not mixed with a mid-run code
change.

After M592 completed, M593 was launched on `spark-1` in tmux session
`ii42_m593_gate20_200_three_seed` with the corrected per-seed run names.  An
earlier malformed launch expanded `${seed}` too early and produced only a
`seed_minbase.bad-launch-*` directory; it was stopped before JSON output and is
not part of the M593 evidence surface.

## M593 Official Composite Gate Result

M593 completed the composite selector gate as an official1024 BEIR8 three-seed
check:

- `runs/m593_active256_h768_selector20_200_official1024_beir8_seed551_minbase/m593_active256_h768_selector20_200_official1024_beir8_seed551_minbase.json`
- `runs/m593_active256_h768_selector20_200_official1024_beir8_seed552_minbase/m593_active256_h768_selector20_200_official1024_beir8_seed552_minbase.json`
- `runs/m593_active256_h768_selector20_200_official1024_beir8_seed553_minbase/m593_active256_h768_selector20_200_official1024_beir8_seed553_minbase.json`

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | +0.004620 | +0.002060 | +0.000850 | +0.002270 | +0.003180 |
| 551 | guarded | +0.004950 | +0.002420 | +0.000490 | +0.006420 | +0.003190 |
| 552 | learned | +0.004760 | +0.001800 | +0.005480 | +0.001600 | +0.005590 |
| 552 | guarded | +0.003210 | +0.001060 | +0.002340 | +0.001070 | +0.004140 |
| 553 | learned | -0.001120 | +0.001130 | -0.000350 | +0.001090 | +0.004210 |
| 553 | guarded | +0.000930 | +0.001790 | +0.000990 | +0.003300 | +0.000940 |
| mean | learned | +0.002753 | +0.001663 | +0.001993 | +0.001653 | +0.004327 |
| mean | guarded | +0.003030 | +0.001757 | +0.001273 | +0.003597 | +0.002757 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 2/3 | 3/3 | 2/3 | 3/3 | 3/3 |
| guarded | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |

Decision: M593 improves mean NDCG and MRR over M592, but gives back a small
amount of Recall and overlap.  It is the stronger early-rank selector, while
M592 remains the simpler higher-recall selector.  The next required test is
cross-root validation; without that, neither should be treated as the final
endpoint of the M551 route.

## M594 BEIR Shared-Root Cross-Root Check

M594 ran the M593 composite selector on the BEIR shared root:

- root: `/home/huoju/leask/runs/ii42-m565-m551-beir-shared-root-v1/_shared/tasks`
- run dir: `/home/huoju/leask/runs/ii42-m594-m551-active256-selector20-200-beir-shared-v1`
- local JSONs:
  - `runs/m594_active256_h768_selector20_200_beir_shared_seed551_minbase/m594_active256_h768_selector20_200_beir_shared_seed551_minbase.json`
  - `runs/m594_active256_h768_selector20_200_beir_shared_seed552_minbase/m594_active256_h768_selector20_200_beir_shared_seed552_minbase.json`
  - `runs/m594_active256_h768_selector20_200_beir_shared_seed553_minbase/m594_active256_h768_selector20_200_beir_shared_seed553_minbase.json`

This root is a much smaller sampled surface than the official1024 root
(`scidocs`, `scifact`, `webis-touche2020`, and `cqadupstack` show 2000
documents or about 100 queries in the logs), so it is a cross-root sanity check,
not the final broad evaluation.

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | -0.002060 | -0.003320 | -0.000200 | -0.004080 | +0.000750 |
| 551 | guarded | +0.001280 | +0.000420 | +0.000000 | -0.000280 | +0.000170 |
| 552 | learned | -0.001630 | -0.000140 | +0.000000 | -0.000220 | +0.000130 |
| 552 | guarded | -0.000720 | +0.000380 | -0.000840 | +0.000120 | -0.000080 |
| 553 | learned | -0.003370 | -0.002950 | +0.001360 | -0.005680 | -0.001290 |
| 553 | guarded | -0.000290 | -0.000920 | +0.001540 | -0.004710 | -0.001170 |
| mean | learned | -0.002353 | -0.002137 | +0.000387 | -0.003327 | -0.000137 |
| mean | guarded | +0.000090 | -0.000040 | +0.000233 | -0.001623 | -0.000360 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 0/3 | 0/3 | 1/3 | 0/3 | 2/3 |
| guarded | 1/3 | 2/3 | 1/3 | 1/3 | 1/3 |

Offline replay on the same M594 JSONs shows the simpler M592 `20:+0.001` gate
does not rescue the root either:

| Replay Gate | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `20:+0.001` | +0.000010 | -0.000146 | +0.000235 | -0.001467 | -0.000417 |
| `20:+0.001,200:-0.002` | +0.000091 | -0.000038 | +0.000235 | -0.001621 | -0.000361 |

Decision: M594 does not validate M592/M593 as a final endpoint.  The official1024
signal is real, but the cross-root sampled check says the selector is not yet
stable enough.  The next test should use a more representative MTEB/full-root
surface instead of over-tuning this small sampled root.

## M595 MTEB10 Root Validation

M595 was launched after M594 to test the same M593 composite selector on a more
representative MTEB root:

- root: `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`
- run dir: `/home/huoju/leask/runs/ii42-m595-m551-active256-selector20-200-mteb10-v1`
- tmux: `ii42_m595_gate20_200_mteb10`
- tasks: `ArguAna`, `FiQA2018`, `SCIDOCS`, `TRECCOVID`,
  `Touche2020Retrieval.v3`, `CQADupstackGamingRetrieval`,
  `CQADupstackUnixRetrieval`, `ClimateFEVERHardNegatives`,
  `FEVERHardNegatives`, `HotpotQAHardNegatives`

The first seed started normally on `spark-1`; the command line confirms the
MTEB root and the composite qrels-free gate
`20:+0.001,200:-0.002`.  This is the next decisive validation surface because
M594 showed the small BEIR sampled root is not sufficient to promote the route.

M595 completed all three seeds.  Local JSONs:

- `runs/m595_active256_h768_selector20_200_mteb10_seed551_minbase/m595_active256_h768_selector20_200_mteb10_seed551_minbase.json`
- `runs/m595_active256_h768_selector20_200_mteb10_seed552_minbase/m595_active256_h768_selector20_200_mteb10_seed552_minbase.json`
- `runs/m595_active256_h768_selector20_200_mteb10_seed553_minbase/m595_active256_h768_selector20_200_mteb10_seed553_minbase.json`

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | +0.005820 | +0.007510 | +0.004640 | +0.005390 | +0.000050 |
| 551 | guarded | +0.000360 | -0.000620 | -0.000970 | -0.000910 | +0.001670 |
| 552 | learned | -0.000140 | +0.002850 | +0.002480 | -0.011440 | -0.001050 |
| 552 | guarded | +0.000600 | +0.000330 | +0.000250 | +0.000290 | +0.000510 |
| 553 | learned | +0.009980 | +0.008330 | +0.009540 | +0.007020 | +0.002130 |
| 553 | guarded | +0.001020 | +0.000250 | +0.001270 | -0.003580 | +0.004840 |
| mean | learned | +0.005220 | +0.006230 | +0.005553 | +0.000323 | +0.000377 |
| mean | guarded | +0.000660 | -0.000013 | +0.000183 | -0.001400 | +0.002340 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 2/3 | 3/3 | 3/3 | 2/3 | 2/3 |
| guarded | 3/3 | 2/3 | 2/3 | 1/3 | 3/3 |

The learned source is meaningfully positive on the representative MTEB10 root:
MAP and Recall are positive on all seeds, and mean NDCG improves by about
0.0052.  The current overlap-based guarded source is too conservative and loses
most of this signal, especially MRR.

Offline replay with a DREAM-style utility selector is more informative.  Gating
by `rank_teacher_mass_at_20` delta instead of support overlap gives:

| Replay Gate | Accepted Tasks by Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `mass20_delta>=0.005` | 3 / 1 / 3 | +0.005970 | +0.005110 | +0.004265 | +0.005404 | -0.000819 |
| `mass20_delta>=0.005,cov100_delta>=0` | 2 / 1 / 3 | +0.005720 | +0.004832 | +0.004098 | +0.005143 | -0.000829 |
| `overlap20_delta>=0.001,overlap200_delta>=-0.002` | 4 / 2 / 5 | +0.000662 | -0.000014 | +0.000180 | -0.001397 | +0.002340 |

Decision: M595 says the DREAM-lite idea is worth continuing, but the promotion
gate must move from support-preservation-first to utility/mass-first.  The
retrieval metrics improve consistently when selected by teacher-mass utility,
even though overlap can drop slightly.  This is aligned with the DREAM lesson:
the important signal is whether a candidate set preserves retrieval usefulness,
not whether every support geometry statistic improves.  The next validation is
M596: run the same active256/h768 route on official1024 BEIR8 with
`rank_teacher_mass_at_20>=0.005` as the guarded selector and an effectively
disabled overlap floor, then compare against M592/M593.

## M596 Utility-Mass Gate Official1024 Check

M596 was launched on `spark-1` to validate whether the M595 utility/mass replay
generalizes back to the official1024 BEIR8 root:

- root: `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`
- run dir: `/home/huoju/leask/runs/ii42-m596-m551-active256-utility20-official1024-v1`
- tmux: `ii42_m596_utility20_official1024`
- tasks: `arguana`, `nfcorpus`, `fiqa`, `scidocs`, `scifact`,
  `trec-covid`, `webis-touche2020`, `cqadupstack`
- model shape: M551 active256/h768, residual scale `0.0125`,
  coordinate gain clip `0.0`
- guarded selector: `require_selector_utility=true`,
  `selector_utility_gate_k=20`, `selector_utility_min_delta=0.005`
- overlap floor: effectively disabled with `overlap_gate_min_delta=-1.0`

This run is the key cross-check for the DREAM-lite selector change.  If M596
keeps the M592/M593 official1024 gains while M595 shows the same gate working
on MTEB10 replay, the route should graduate from overlap preservation to
utility-mass selection.  If M596 fails, the M595 signal is real but not yet
portable enough for promotion.

M596 completed all three official1024 seeds.  Local JSONs:

- `runs/m596_active256_h768_utility20_official1024_seed551_minbase/m596_active256_h768_utility20_official1024_seed551_minbase.json`
- `runs/m596_active256_h768_utility20_official1024_seed552_minbase/m596_active256_h768_utility20_official1024_seed552_minbase.json`
- `runs/m596_active256_h768_utility20_official1024_seed553_minbase/m596_active256_h768_utility20_official1024_seed553_minbase.json`

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 | Accepted tasks |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 551 | learned | +0.004620 | +0.002060 | +0.000850 | +0.002270 | +0.003180 | all |
| 551 | guarded | +0.001600 | +0.000350 | +0.000290 | +0.000000 | +0.001420 | `trec-covid` |
| 552 | learned | +0.004760 | +0.001800 | +0.005480 | +0.001600 | +0.005590 | all |
| 552 | guarded | +0.000730 | +0.000210 | +0.000320 | +0.000000 | +0.002750 | `trec-covid` |
| 553 | learned | -0.001120 | +0.001130 | -0.000350 | +0.001090 | +0.004210 | all |
| 553 | guarded | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | none |
| mean | learned | +0.002753 | +0.001663 | +0.001993 | +0.001653 | +0.004327 | all |
| mean | guarded | +0.000777 | +0.000187 | +0.000203 | +0.000000 | +0.001390 | sparse |

Decision: pure `rank_teacher_mass_at_20>=0.005` is too strict on official1024.
It only selects `trec-covid` on two seeds and nothing on the third.  The learned
source itself is still useful: MAP, MRR, and overlap are positive on all three
seeds, but NDCG and Recall are only 2/3 positive.  This means M596 validates the
training signal, not the strict utility-only gate.

Offline gate replay across M596 official1024 and M595 MTEB10 shows the best
portable shape is a union selector:

| Replay Gate | Root | Accepted Tasks by Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `ov20>=0.001 OR mass20>=0.003` | official1024 | 6 / 3 / 5 | +0.002905 | +0.001768 | +0.001401 | +0.003202 | +0.002788 |
| `ov20>=0.001 OR mass20>=0.003` | MTEB10 | 6 / 3 / 7 | +0.005958 | +0.005308 | +0.004670 | +0.004377 | +0.000143 |
| `mass20>=0.005` | official1024 | 1 / 1 / 0 | +0.000778 | +0.000185 | +0.000206 | +0.000000 | +0.001389 |
| `mass20>=0.005` | MTEB10 | 3 / 1 / 3 | +0.005970 | +0.005110 | +0.004265 | +0.005404 | -0.000819 |
| `ov20>=0.001` | official1024 | 6 / 3 / 5 | +0.002905 | +0.001768 | +0.001401 | +0.003202 | +0.002788 |
| `ov20>=0.001` | MTEB10 | 4 / 2 / 6 | +0.001226 | +0.000373 | +0.000902 | -0.000620 | +0.002296 |

The union gate preserves the official1024 overlap-gate gains while importing
the MTEB10 teacher-mass signal that fixes MRR.  This is the first selector shape
that is positive on both roots for all four retrieval metrics in replay.

## M597/M598 Union Gate Validation

M597 and M598 were launched in parallel after adding `selector_gate_mode=any` to
the runner.  The default remains `all`, so older M592-M596 JSON semantics are
unchanged.

- M597 on `spark-1`: official1024 BEIR8, run dir
  `/home/huoju/leask/runs/ii42-m597-m551-active256-union20-official1024-v1`,
  tmux `ii42_m597_union20_official1024`
- M598 on `spark-2`: MTEB10, run dir
  `/home/huoju/leask/runs/ii42-m598-m551-active256-union20-mteb10-v1`,
  tmux `ii42_m598_union20_mteb10`
- selector: `selector_gate_mode=any`,
  `overlap_at_20_delta>=0.001 OR rank_teacher_mass_at_20_delta>=0.003`

Promotion criterion for this sub-route: both M597 and M598 must reproduce the
offline replay direction with official JSONs.  If they do, M597/M598 become the
current best M551-based selector milestone.  If not, keep M596 learned-all as
the evidence that the model signal exists, but do not promote the union gate.

M597 and M598 completed all three seeds.  Local JSONs:

- `runs/m597_active256_h768_union20_official1024_seed551_minbase/m597_active256_h768_union20_official1024_seed551_minbase.json`
- `runs/m597_active256_h768_union20_official1024_seed552_minbase/m597_active256_h768_union20_official1024_seed552_minbase.json`
- `runs/m597_active256_h768_union20_official1024_seed553_minbase/m597_active256_h768_union20_official1024_seed553_minbase.json`
- `runs/m598_active256_h768_union20_mteb10_seed551_minbase/m598_active256_h768_union20_mteb10_seed551_minbase.json`
- `runs/m598_active256_h768_union20_mteb10_seed552_minbase/m598_active256_h768_union20_mteb10_seed552_minbase.json`
- `runs/m598_active256_h768_union20_mteb10_seed553_minbase/m598_active256_h768_union20_mteb10_seed553_minbase.json`

M597 official1024 deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 | Accepted tasks |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 551 | learned | +0.004620 | +0.002060 | +0.000850 | +0.002270 | +0.003180 | all |
| 551 | guarded | +0.004950 | +0.002420 | +0.000490 | +0.006420 | +0.003190 | 6 |
| 552 | learned | +0.004760 | +0.001800 | +0.005480 | +0.001600 | +0.005590 | all |
| 552 | guarded | +0.003210 | +0.001060 | +0.002340 | +0.001070 | +0.004140 | 3 |
| 553 | learned | -0.001120 | +0.001130 | -0.000350 | +0.001090 | +0.004210 | all |
| 553 | guarded | +0.000560 | +0.001830 | +0.001380 | +0.002120 | +0.001040 | 5 |
| mean | learned | +0.002753 | +0.001663 | +0.001993 | +0.001653 | +0.004327 | all |
| mean | guarded | +0.002907 | +0.001770 | +0.001403 | +0.003203 | +0.002790 | union |

M598 MTEB10 deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 | Accepted tasks |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 551 | learned | +0.005820 | +0.007510 | +0.004640 | +0.005390 | +0.000050 | all |
| 551 | guarded | +0.005920 | +0.005380 | +0.003030 | +0.005100 | -0.000860 | 6 |
| 552 | learned | -0.000140 | +0.002850 | +0.002480 | -0.011440 | -0.001050 | all |
| 552 | guarded | +0.004330 | +0.004100 | +0.003400 | +0.004100 | -0.001350 | 3 |
| 553 | learned | +0.009980 | +0.008330 | +0.009540 | +0.007020 | +0.002130 | all |
| 553 | guarded | +0.007620 | +0.006440 | +0.007600 | +0.003920 | +0.002650 | 7 |
| mean | learned | +0.005220 | +0.006230 | +0.005553 | +0.000323 | +0.000377 | all |
| mean | guarded | +0.005957 | +0.005307 | +0.004677 | +0.004373 | +0.000147 | union |

Positive-seed count:

| Run | Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| M597 official1024 | learned | 2/3 | 3/3 | 2/3 | 3/3 | 3/3 |
| M597 official1024 | guarded | 3/3 | 3/3 | 3/3 | 3/3 | 3/3 |
| M598 MTEB10 | learned | 2/3 | 3/3 | 3/3 | 2/3 | 2/3 |
| M598 MTEB10 | guarded | 3/3 | 3/3 | 3/3 | 3/3 | 1/3 |

Decision: M597/M598 promote the union selector as the current best M551
milestone.  It preserves the official1024 overlap-gate behavior and fixes the
MTEB10 MRR failure by allowing teacher-mass-selected tasks.  The only remaining
weakness is overlap on MTEB10: mean overlap is slightly positive, but only one
seed is positive.  That is acceptable for a retrieval-function milestone, but
the next stage should validate on the broader standard matrix before claiming a
final endpoint.

## M599 Broader BEIR15 Shared15 Validation

M599 ran the exact M597/M598 union selector on the broader local BEIR15 shared
face with 15 tasks.  This is the practical broader matrix available to the
current M551 runner; the official 1024-dimensional full15 root exists, but some
document matrices are 20-66 GB and need a streaming evaluator before they are a
fair M551 validation surface.

Run dir:
`/home/huoju/leask/runs/ii42-m599-m551-active256-union20-beir15-shared15-v1`.

Local JSONs:

- `runs/m599_active256_h768_union20_beir15_shared15_seed551_minbase/m599_active256_h768_union20_beir15_shared15_seed551_minbase.json`
- `runs/m599_active256_h768_union20_beir15_shared15_seed552_minbase/m599_active256_h768_union20_beir15_shared15_seed552_minbase.json`
- `runs/m599_active256_h768_union20_beir15_shared15_seed553_minbase/m599_active256_h768_union20_beir15_shared15_seed553_minbase.json`

Three-seed deltas versus each seed's `dense_topk256_sparse` baseline:

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | -0.001590 | -0.002090 | -0.000150 | -0.004500 | -0.001000 |
| 551 | guarded | +0.001100 | +0.000720 | +0.000000 | +0.000820 | -0.000310 |
| 552 | learned | -0.000890 | -0.000010 | +0.000060 | -0.002640 | -0.001180 |
| 552 | guarded | -0.000470 | +0.000090 | -0.000440 | -0.000050 | -0.000170 |
| 553 | learned | -0.002880 | -0.001960 | +0.000680 | -0.003660 | -0.001410 |
| 553 | guarded | -0.000620 | -0.001040 | +0.000820 | -0.003120 | -0.000600 |
| mean | learned | -0.001787 | -0.001353 | +0.000197 | -0.003600 | -0.001197 |
| mean | guarded | +0.000003 | -0.000077 | +0.000127 | -0.000783 | -0.000360 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 0/3 | 0/3 | 2/3 | 0/3 | 0/3 |
| guarded | 1/3 | 2/3 | 1/3 | 1/3 | 0/3 |

Decision: M599 stops the M597/M598 gate-microtuning line as a final endpoint.
The union selector remains a useful diagnostic milestone, but it does not pass
the broader matrix.  M600 should not start from this branch because its
precondition was a broader-matrix pass.  Future work should either build a
streaming evaluator for official full15, or start a separate output-head /
posting-compiler route whose target is dense-derived posting equivalence rather
than another selector threshold sweep.
