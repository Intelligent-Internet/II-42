# M326 Dense-Teacher Admission Scorer Report

## Status

M326 is active. The first valid dense-teacher run showed a real admission
signal, and the five-dataset expansion confirms that the signal transfers
outside the original canary. It is still not a full promotion because the
expanded scorer remains clearly below the exact dense teacher on MRR, NDCG,
and MAP.

The active follow-up is no longer another uniform teacher-weight sweep. The
next useful step is feature-family or query-adaptive admission: keep the
dense-teacher signal, but learn when semantic admission should affect recall
and when it should not perturb top-rank qrel ordering.

## Baseline Surface

All numbers below use the same seed-1050 heldout split on `nfcorpus`,
`scifact`, and `fiqa`.

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Exact dense teacher | 0.7433 | 0.6152 | 0.5330 | 0.4464 |
| M323 df<=0.25 best | 0.6642 | 0.5485 | 0.4536 | 0.3663 |
| M324 df<=0.25 best | 0.6337 | 0.5236 | 0.4294 | 0.3531 |
| M325 df<=0.25 best | 0.6488 | 0.5131 | 0.4325 | 0.3436 |

The promotion target is still the exact dense teacher gap. M323 is the
local scorer baseline; M324/M325 are useful negative controls.

## Data Corrections

Two issues were found and fixed before trusting M326 results.

1. The first dense-teacher export used PostgreSQL document ids with the
   `beir15:<dataset>:d:` prefix, while M322/M323 scorer surfaces use raw
   BEIR document ids. This produced zero teacher overlap. The exporter now
   normalizes ids back to raw BEIR ids.

2. The first teacher JSON covered only heldout queries. That is valid for
   diagnostics, but it gives no teacher supervision to training examples.
   The valid M326 training teacher is now exported with `split=all`.

After both fixes, the teacher overlap is healthy:

| Variant | Dataset | Teacher query rate | Avg teacher docs in pool |
| --- | --- | ---: | ---: |
| df<=0.25 | fiqa | 1.0000 | 61.17 |
| df<=0.25 | nfcorpus | 1.0000 | 63.28 |
| df<=0.25 | scifact | 1.0000 | 90.76 |
| df<=0.50 | fiqa | 1.0000 | 67.37 |
| df<=0.50 | nfcorpus | 1.0000 | 66.73 |
| df<=0.50 | scifact | 1.0000 | 98.10 |

This means the current failure mode is not candidate admission coverage. The
candidate pool already contains dense-neighborhood evidence; the remaining
issue is scoring and qrel/top-rank calibration.

## M326A Result

M326A used the valid all-query dense teacher with listwise KL:

- `teacher_weight=0.25`
- `teacher_temperature=0.12`
- `candidate_k=160`
- `datasets=nfcorpus,scifact,fiqa`

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M323 df<=0.25 best | 0.6642 | 0.5485 | 0.4536 | 0.3663 |
| M326A df<=0.25 best | 0.6731 | 0.5485 | 0.4542 | 0.3659 |
| M323 df<=0.50 best | 0.6605 | 0.5517 | 0.4557 | 0.3614 |
| M326A df<=0.50 best | 0.6758 | 0.5471 | 0.4588 | 0.3660 |

Interpretation:

- Dense-teacher supervision improves admission and NDCG.
- MAP/MRR do not move enough, and MRR can regress.
- The next step must preserve qrel positives against dense-near false
  positives instead of simply increasing teacher KL.

## M326B Active Canary

M326B adds a false-positive loss for dense-teacher positives that are not
qrel positives:

- `teacher_weight=0.20`
- `teacher_false_positive_weight=0.30`
- `teacher_false_positive_margin=0.03`
- same datasets, checkpoint, seed, split, and candidate surface as M326A.

Run path:

`/home/huoju/leask/runs/ii42-m326-dense-teacher-all-scorer-fp-v1`

Result:

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M323 df<=0.25 best | 0.6642 | 0.5485 | 0.4536 | 0.3663 |
| M326A df<=0.25 best | 0.6731 | 0.5485 | 0.4542 | 0.3659 |
| M326B df<=0.25 epoch20 | 0.6803 | 0.5533 | 0.4593 | 0.3690 |
| M323 df<=0.50 best | 0.6605 | 0.5517 | 0.4557 | 0.3614 |
| M326A df<=0.50 best | 0.6758 | 0.5471 | 0.4588 | 0.3660 |
| M326B df<=0.50 epoch40 | 0.6773 | 0.5566 | 0.4588 | 0.3650 |
| M326B tw0.15/fp0.30 df<=0.25 epoch20 | 0.6777 | 0.5571 | 0.4620 | 0.3724 |
| M326B tw0.15/fp0.50 df<=0.25 epoch20 | 0.6764 | 0.5560 | 0.4629 | 0.3729 |

M326B improves all four metrics over M323 on `df<=0.25`. The `df<=0.50`
variant has the highest MRR, but the best balanced checkpoint is currently
`tw0.15/fp0.30` with `df<=0.25` epoch 20 because it improves MRR, NDCG, and
MAP while keeping almost all of the recall gain.

The `tw0.15/fp0.50` row slightly improves NDCG/MAP, but it gives up more
Recall and MRR. That is not a clear promotion over `tw0.15/fp0.30` for a
retrieval admission scorer.

Later epochs regress for both variants. This confirms that the false-positive
constraint is useful, but the scorer overfits quickly. Any follow-up should
keep early-stopping and avoid increasing epochs.

Expected decision:

- Stop the local sweep here. `tw0.15/fp0.30 df<=0.25 epoch20` is the current
  balanced canary winner.
- Before expanding, add candidate/example caching. The current runner caches
  atom surfaces, but repeated scorer sweeps still spend too much time
  rebuilding lexical/atom candidate examples.
- Next model step: expand the winning configuration to one or two additional
  canary datasets only after the example-cache path is in place.
- Do not expand to larger datasets until this canary surface has a stable
  winner.

## Current Decision

M326B is the current best candidate-pool scorer on the seed-1050 canary. It is
not promoted to broad evaluation yet, but it is strong enough to justify a
small local sweep and then an expansion canary if the sweep confirms the gain.

## Candidate Cache Follow-Up

The scorer runner now supports `--candidate-cache-dir`. This cache stores the
frozen lexical and atom candidate surface after doc/query atoms have been
pooled:

- BM25 rankings and scores.
- Pruned atom rankings and scores for each DF ratio.
- Candidate costs and DF diagnostics.
- Candidate cache config covering data-file stats, checkpoint stats,
  active-k, candidate-k, BM25 parameters, and DF ratios.

Smoke validation on `nfcorpus`:

1. First run wrote the cache:
   `wrote candidate cache dataset=nfcorpus`.
2. Second run loaded the cache:
   `load candidate cache dataset=nfcorpus`.
3. The smoke metrics matched between both runs.

This does not change model behavior. It only removes repeated candidate
surface construction from scorer-loss sweeps. The next expansion should use
this cache path by default.

## Dense Export Infrastructure

The dense exporter now has two explicit modes:

- `exact-teacher`: disables approximate index paths and exports exact dense
  labels. This remains the only valid source for dense teacher supervision and
  dense-gap reporting.
- `ann-candidate`: exports a fast product candidate/baseline cache. It must use
  VectorChord for every dataset. Missing `dataset_manifest.vector_probes` is a
  configuration error, not a reason to fall back to exact scan.

The exporter uses raw BEIR ids externally but can look up both raw query ids
and local `beir15:<dataset>:q:<id>` database query ids. In `ann-candidate`
mode it fetches each query embedding through `queries_pkey`, passes the
embedding as a KNN constant, and checks that the first indexed query plan uses
the expected `docs_<dataset>_embedding_vchord_idx` index.

`exact-teacher` also records and sets the resolved `vector_probes` value. This
does not enable ANN retrieval: index scans are still disabled. It only avoids
local VectorChord halfvec operator failures where a list index expects a
nonzero probes setting.

The BEIR15 local database now assigns VectorChord `lists/probes` to every
dataset, including small tables. The product-evaluation rule is intentionally
stricter than the first build: dense product baselines must exercise the same
indexed path that production would use.

| Dataset | Docs | Lists | Probes |
| --- | ---: | ---: | ---: |
| `nfcorpus` | 3,633 | 7 | 3 |
| `scifact` | 5,183 | 10 | 3 |
| `arguana` | 8,674 | 17 | 4 |
| `scidocs` | 25,657 | 51 | 7 |
| `fiqa` | 57,638 | 115 | 11 |

Evaluation rules:

1. Use `exact-teacher` for any training labels, dense teacher matrices, or
   dense gap claims.
2. Use `ann-candidate` for product-style dense baselines and speed-oriented
   candidate cache generation.
3. Before accepting an ANN cache for a new dataset family, run a sample
   `exact-teacher` export and compare ANN-vs-exact overlap at the relevant
   `top_k`, plus qrel recall delta.
4. Treat `actual_path` in the JSON as authoritative:
   accepted product matrices must report `ann_vector_index`.
5. Do not mix exact and ANN rows in the same metric table without labeling
   the source path.

Smoke evidence:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m326_export_dense_teacher.py \
  --dsn 'dbname=postgres' \
  --schema ii42_beir15 \
  --datasets-root /Volumes/Betty/Tmp/ii42-m326-expansion-root-v1/all-test \
  --dataset nfcorpus \
  --output-json /Volumes/Betty/Tmp/ii42-m326-baselines/dense_ann_index_smoke_nfcorpus_top5.json \
  --seed 1050 \
  --split heldout \
  --top-k 5 \
  --mode ann-candidate
```

Result: `nfcorpus` wrote `actual_path=ann_vector_index`, `vector_probes=3`,
`plan_guard_checked=true`, `db_query_lookup_misses=0`, and
`exported_query_count=97`.

Additional local DB-level sample check:

| Dataset | Queries | Probes | Path | Uses Vector Index | Overlap@100 | Exact qrel R@100 | ANN qrel R@100 | Exact seconds | ANN seconds |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| `trec-covid` | 10 | 19 | `ann_vector_index` | yes | 0.9210 | 0.1679 | 0.1653 | 5.108 | 1.377 |

Exporter-level smoke:

- `trec-covid exact-teacher`: `actual_path=exact_scan`,
  `vector_probes=19`, `db_query_lookup_misses=0`, `exported_query_count=15`.
- `trec-covid ann-candidate`: `actual_path=ann_vector_index`,
  `vector_probes=19`, `plan_guard_checked=true`,
  `db_query_lookup_misses=0`, `exported_query_count=15`.
- `nfcorpus ann-candidate`: `actual_path=ann_vector_index`,
  `vector_probes=3`, `plan_guard_checked=true`,
  `db_query_lookup_misses=0`, `exported_query_count=97`.
- `small5 heldout top1 smoke`: `nfcorpus`, `scifact`, `arguana`,
  `scidocs`, and `fiqa` all reported `actual_path=ann_vector_index`,
  `plan_guard_checked=true`, and zero lookup misses.

Five-dataset product dense export:

`/Volumes/Betty/Tmp/ii42-m326-expansion-results/dense_vectorchord_product_seed1050_expansion5_top300.json`

Metrics:

`/Volumes/Betty/Tmp/ii42-m326-expansion-results/dense_vectorchord_product_seed1050_expansion5_top300_metrics.json`

Aggregate `all` split comparison:

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Exact dense teacher | 0.7703 | 0.4371 | 0.4126 | 0.3144 |
| Product VectorChord dense | 0.6858 | 0.5055 | 0.4402 | 0.3472 |
| M326B `tw0.15/fp0.30` | 0.8518 | 0.3641 | 0.3495 | 0.2735 |

Interpretation:

- Exact dense remains the correct teacher/oracle label surface.
- Product VectorChord dense is the fair product baseline. It has lower
  Recall@100 than exact dense on this five-dataset surface, but higher
  top-rank metrics.
- M326B still has much higher admission recall than product dense, but its
  top-rank scoring remains materially weaker. The next optimization should
  target scoring/calibration, not more raw candidate coverage.

## Expansion5 Result

The expansion reran the same scorer family on five heldout datasets:

- `nfcorpus`
- `scifact`
- `fiqa`
- `arguana`
- `scidocs`

The raw BEIR dataset root used unprefixed query ids while the local
PostgreSQL dense table stores query ids as `beir15:<dataset>:q:<id>`. The
dense-teacher exporter now checks both id forms and records
`db_query_lookup_misses`. The full export reported zero lookup misses.

Expansion paths:

- Dataset root:
  `/Volumes/Betty/Tmp/ii42-m326-expansion-root-v1/all-test`
- Dense teacher:
  `/Volumes/Betty/Tmp/ii42-m326-expansion-results/dense_teacher_expansion5_same_split_metrics.json`
- Product VectorChord dense:
  `/Volumes/Betty/Tmp/ii42-m326-expansion-results/dense_vectorchord_product_seed1050_expansion5_top300_metrics.json`
- Scorer outputs:
  `/Volumes/Betty/Tmp/ii42-m326-expansion-results/`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m326-expansion5-candidate-cache-v1`

`arguana` remains a partial upstream surface because official qrels reference
five positive document ids that are absent from the official corpus. This is
tracked as an upstream-corpus caveat, not a scorer/data join failure.

### Aggregate Heldout Matrix

All rows use the same seed-1050 heldout split and `df<=0.25`.

| Surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Exact dense teacher | 0.7739 | 0.4280 | 0.4001 | 0.3032 |
| M323 baseline best | 0.7251 | 0.3444 | 0.3326 | 0.2360 |
| M323 baseline row | 0.7054 | 0.3515 | 0.3279 | 0.2337 |
| M326B `tw0.15/fp0.30` best | 0.7449 | 0.3568 | 0.3369 | 0.2431 |
| M326B `tw0.20/fp0.30` best | 0.7450 | 0.3549 | 0.3335 | 0.2409 |
| M326B `tw0.15/fp0.50` best | 0.7462 | 0.3567 | 0.3363 | 0.2431 |
| M326B `tw0.10/fp0.30` best | 0.7423 | 0.3570 | 0.3359 | 0.2435 |

The balanced expansion winner remains `tw0.15/fp0.30`. `tw0.15/fp0.50`
has the best Recall@100, and `tw0.10/fp0.30` has the best MAP@100, but both
trade off another metric. The balanced row improves over M323 best by:

| Delta | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `tw0.15/fp0.30` - M323 best | +0.0198 | +0.0124 | +0.0043 | +0.0071 |
| `tw0.15/fp0.30` - dense | -0.0290 | -0.0712 | -0.0632 | -0.0601 |

The expansion confirms a real but incomplete transfer: dense-teacher
admission recovers recall and some top-rank quality over M323, but a large
MRR/NDCG/MAP gap to dense remains.

### Per-Dataset Behavior

`tw0.15/fp0.30` versus M323 and exact dense:

| Dataset | M323 R | M326 R | Dense R | M326-M323 R | M326-Dense R | M326-M323 NDCG | M326-Dense NDCG |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.9562 | 0.9903 | 1.0000 | +0.0341 | -0.0097 | -0.0119 | -0.0794 |
| `fiqa` | 0.6880 | 0.7144 | 0.8465 | +0.0264 | -0.1321 | +0.0534 | -0.1107 |
| `nfcorpus` | 0.3028 | 0.2993 | 0.3186 | -0.0034 | -0.0192 | +0.0073 | -0.0175 |
| `scidocs` | 0.4253 | 0.4251 | 0.5059 | -0.0002 | -0.0808 | +0.0006 | -0.0594 |
| `scifact` | 0.9386 | 0.9569 | 0.9444 | +0.0183 | +0.0125 | +0.0047 | +0.0177 |

Interpretation:

- `fiqa` is the strongest positive transfer: both recall and top-rank quality
  improve materially over M323.
- `scifact` already has a high-quality surface, and M326B exceeds dense on the
  heldout split.
- `nfcorpus` and `scidocs` mostly show top-rank calibration improvements with
  little or no recall movement.
- `arguana` exposes the remaining failure mode: admission improves recall, but
  the teacher/false-positive balance can still hurt top-rank ordering.

## Updated Decision

M326B is validated as a transferable scorer improvement, not just a local
three-dataset canary. However, the dense gap is still too large for promotion
as the final scorer route.

Next work should not keep sweeping a global teacher weight. The useful
directions are:

1. Add query/candidate-family adaptive admission so `arguana`-style top-rank
   regressions can be suppressed without losing `fiqa` recall gains.
2. Split admission and ranking losses: use dense teacher for admission, but
   enforce qrel/top-rank constraints more strongly after admission succeeds.
3. Report per-source candidate impacts for the expansion datasets before any
   larger full15 run, especially dense-hit/SAE-miss and high-BM25 false
   positives.
