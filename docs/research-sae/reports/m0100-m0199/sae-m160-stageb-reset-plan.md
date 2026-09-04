# M160-Specific Stage-B Reset Plan

Date: 2026-06-02

## Summary

M160A Stage A improved sparse semantic representation, but the direct M160B
Stage-B canary only transferred on the M130 continuity surface. The partial
official gate on `fiqa` and `scidocs` showed that dense/BM25+dense still lead
on official full-corpus distributions.

The next step is not to wait for the long official gate or to compress fanout.
The next step is a M160-specific Stage-B reset:

- start from M160A Stage-A `latest.pt`;
- do not inherit the direct M160B continuity checkpoint;
- train on official non-test qrels and full-corpus hard negatives;
- make SAE act as a BM25 complement rather than generic dense imitation;
- validate with a short official canary before any long BEIR gate.

## Decision Evidence

Continuity surface:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense | 0.3173 | 0.2994 | 0.2262 | 0.1493 |
| M160B BM25+SAE | 0.3421 | 0.3581 | 0.2528 | 0.1557 |

Partial official gate:

| Dataset | M160B BM25+SAE R@100 | Dense R@100 | BM25+dense R@100 |
| --- | ---: | ---: | ---: |
| `fiqa` | 0.8050 | 0.8290 | 0.8100 |
| `scidocs` | 0.4701 | 0.4958 | 0.4867 |

Interpretation:

- M160A representation is useful.
- Direct continuity-surface Stage B is not enough.
- The official failure is a training-surface mismatch, not only a cost issue.

## B0 Canary

Runner:

```bash
scripts/run_m160_stageb_reset_spark.sh
```

Default run:

```text
run_name: bm25sae-m160-stageb-reset-b0-official-v1
checkpoint: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1/bm25sae_stagea_latest.pt
clearml_project: ii42_sae/M160
```

Train specs:

| Dataset | Qrels split | Max queries | Reason |
| --- | --- | ---: | --- |
| `fiqa` | `train` | all | official failed canary family |
| `nfcorpus` | `train` | all | biomedical lexical/semantic mix |
| `scifact` | `train` | all | scientific claim retrieval |
| `quora` | `dev` | 5000 | semantic duplicate-query supervision |

Validation specs:

| Dataset | Qrels split | Max queries |
| --- | --- | ---: |
| `fiqa` | `dev` | all |
| `nfcorpus` | `dev` | all |

Profile:

| Parameter | Value |
| --- | --- |
| `feature_k` | 96 |
| `candidate_k` | 192 |
| `source_top_k` | 500 |
| `max_df_ratio` | 0.12 |
| `steps` | 5000 |

## Acceptance Gate

The B0 canary is useful only if it improves both surfaces:

- continuity full-corpus must remain competitive with M160B direct Stage B;
- `fiqa` official canary must improve over M160B BM25+SAE;
- `scidocs` official canary must at least narrow the dense gap;
- sparse fanout must not become worse than M160B direct Stage B unless quality
  improves enough to justify a later compression pass.

If the B0 canary improves only candidate-surface metrics but not full-corpus
metrics, the next step should be a broader official non-test/streaming hard
negative row surface, not another loss-weight sweep.

## B1 Residual Fusion Probe

Runner:

```bash
RUN_NAME=bm25sae-m160-stageb-reset-b1-residual-v1 \
SESSION=bm25sae_m160_stageb_reset_b1_residual_v1 \
REUSE_EXISTING_ROWS=1 \
TRAIN_ROWS=/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b0-official-v1-candidate-rows \
EVAL_ROWS=/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b0-official-v1-eval-candidate-rows \
FUSION_BM25_MODE=residual \
STEPS=5000 \
scripts/run_m160_stageb_reset_spark.sh
```

Candidate-surface result:

| Row | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| Dense teacher | 0.8200 | 0.5559 |
| B1 SAE-only | 0.8124 | 0.5803 |
| B1 BM25+SAE residual | 0.8230 | 0.5816 |

Official continuity full-corpus result:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| B0 BM25+SAE additive | 0.3122 | 0.2776 | 0.2084 | 0.1417 |
| B1 BM25+SAE residual | 0.3094 | 0.2686 | 0.2021 | 0.1360 |

Interpretation:

- Residual fusion is directionally useful on the local candidate surface:
  BM25 should fill SAE gaps instead of overwriting high-confidence SAE rows.
- Reusing B0 rows for B1 was not enough. B0 rows were built from an additive
  Stage-A BM25+SAE surface, so the hard negatives did not match the residual
  geometry used by B1 training and full evaluation.
- The full-corpus regression means the current blocker is not a scalar-weight
  sweep. The training surface must expose full-corpus false positives from the
  same final fusion mode used at evaluation time.

## B2 Rebuilt Residual Surface

The next run should rebuild the rows rather than reuse B0:

- surface build: residual BM25+SAE ranking;
- training final score: residual BM25+SAE;
- full-corpus eval: residual BM25+SAE;
- checkpoint selection: keep soft admission stable and delay straight-through
  active clipping until after official full-corpus admission improves.

Recommended first B2 profile:

```bash
RUN_NAME=bm25sae-m160-stageb-reset-b2-residual-surface-v1 \
SESSION=bm25sae_m160_stageb_reset_b2_residual_surface_v1 \
CONTAINER=bm25sae_m160_stageb_reset_b2_residual_surface_v1 \
LOG=/home/huoju/leask/logs/bm25sae_m160_stageb_reset_b2_residual_surface_v1.log \
FUSION_BM25_MODE=residual \
REUSE_EXISTING_ROWS=0 \
TRAIN_SPECS=fiqa:train:0,nfcorpus:train:0,scifact:train:0,quora:dev:1000 \
EVAL_SPECS=fiqa:dev:0,nfcorpus:dev:0 \
STEPS=3500 \
ST_AFTER=100000 \
FEATURE_K=96 \
INITIAL_SAE_SCALE=1.0 \
INITIAL_BM25_SCALE=0.35 \
TARGET_SAE_SCALE=1.0 \
TARGET_BM25_SCALE=0.35 \
scripts/run_m160_stageb_reset_spark.sh
```

If B2 improves local validation but still misses official full-corpus metrics,
the next revision should add a broader official non-test streaming surface with
large-corpus dense/BM25/SAE false positives. At that point the target is no
longer better fusion math; it is better admission/ranking supervision.

`quora:dev:1000` is intentional for the first B2 probe. The 5000-query quora
surface spends too much wall time in full-corpus BM25 hard-negative generation,
while the real B2 isolation variable is consistent residual geometry. If B2 is
directionally good, expand the quora and large-corpus stream later.

## Static Fusion Sweep Stop Rule

Post-hoc sweeps over the existing B0/B1 full-corpus rankings tested additive,
residual, and excess BM25 contribution modes with BM25 candidate limits from 0
to 200. The best static BM25+SAE variants still did not beat the dense or
BM25+dense controls.

| Run | Best BM25+SAE Recall@100 | Dense Recall@100 | BM25+dense Recall@100 | Best BM25+SAE MAP@100 | Dense MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| B0 additive checkpoint | 0.3127 | 0.3132 | 0.3180 | 0.1446 | 0.1504 |
| B1 residual checkpoint | 0.3098 | 0.3132 | 0.3173 | 0.1453 | 0.1504 |

Decision:

- do not spend more cycles on static scalar fusion sweeps for B0/B1;
- keep residual as a useful hypothesis, but only with rows rebuilt from the
  same residual final-fusion geometry;
- if B2 does not transfer, the blocker is full-corpus admission/ranking
  supervision, not the runtime formula alone.

## B2 Result And B3 Pivot

B2 rebuilt the candidate rows with residual BM25+SAE geometry and used the
same residual mode in training and full-corpus evaluation. This fixed the
local candidate-surface mismatch but still did not pass the official
full-corpus gate.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3173 | 0.2993 | 0.2260 | 0.1492 |
| B2 SAE-only | 0.2921 | 0.2698 | 0.2018 | 0.1379 |
| B2 BM25+SAE residual | 0.3096 | 0.2728 | 0.2052 | 0.1388 |

The B2 local validation surface was encouraging: BM25+SAE exceeded dense on
MRR and briefly matched dense on hit@20. The transfer failure means the
problem is still not a scalar fusion formula. The training target is likely
distilling current BM25+SAE ranking mistakes back into the model.

B3 therefore changes the teacher composition:

- BM25+SAE stays in the candidate pool so high-scoring lexical/semantic false
  positives remain visible as hard negatives.
- BM25+SAE no longer contributes to teacher scores.
- Dense, BM25+dense, standalone SAE, qrels, and a small BM25 rank signal drive
  the training target.
- The goal is to preserve SAE semantic ordering while letting BM25 fill
  lexical gaps only when it helps qrel/dense/BM25+dense agreement.

Recommended B3 probe:

```bash
RUN_NAME=bm25sae-m160-stageb-reset-b3-no-selfteacher-v1 \
SESSION=bm25sae_m160_stageb_reset_b3_no_selfteacher_v1 \
CONTAINER=bm25sae_m160_stageb_reset_b3_no_selfteacher_v1 \
LOG=/home/huoju/leask/logs/bm25sae_m160_stageb_reset_b3_no_selfteacher_v1.log \
FUSION_BM25_MODE=residual \
REUSE_EXISTING_ROWS=0 \
TRAIN_SPECS=fiqa:train:0,nfcorpus:train:0,scifact:train:0,quora:dev:1000 \
EVAL_SPECS=fiqa:dev:0,nfcorpus:dev:0 \
STEPS=3500 \
ST_AFTER=100000 \
FEATURE_K=96 \
INITIAL_SAE_SCALE=1.0 \
INITIAL_BM25_SCALE=0.30 \
TARGET_SAE_SCALE=1.0 \
TARGET_BM25_SCALE=0.30 \
TEACHER_DENSE_WEIGHT=0.45 \
TEACHER_FUSION_RANK_WEIGHT=0.35 \
TEACHER_BM25_RANK_WEIGHT=0.05 \
TEACHER_BM25_SAE_RANK_WEIGHT=0.00 \
TEACHER_SAE_RANK_WEIGHT=0.20 \
ROW_WEIGHT_BM25_SAE_HIT=0.45 \
ROW_WEIGHT_DENSE_MISS=5.00 \
ROW_WEIGHT_SCORE_LOW=4.00 \
ROW_WEIGHT_NOT_RETRIEVED=2.00 \
scripts/run_m160_stageb_reset_spark.sh
```

Stop rule:

- if B3 local validation improves but full-corpus still fails, expand the
  surface with larger non-test hard-negative streams rather than changing
  scalar weights again;
- if B3 local validation gets worse than B2, restore a small BM25+SAE teacher
  weight only for qrel-positive rows, not for the whole rank list.

## B3 Result And B4 Ranking Calibration

B3 removed BM25+SAE from teacher scores but kept it in the candidate pool. This
improved official full-corpus admission but did not fix ranking.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3178 | 0.2972 | 0.2256 | 0.1493 |
| B3 SAE-only | 0.2982 | 0.2696 | 0.2026 | 0.1394 |
| B3 BM25+SAE residual | 0.3132 | 0.2718 | 0.2047 | 0.1390 |

B3 meaning:

- removing self-teacher helped admission: BM25+SAE Recall@100 recovered from
  B2 `0.3096` to `0.3132`, effectively matching dense recall;
- ranking remains weak: BM25+SAE MRR/NDCG/MAP are still far below dense and
  BM25+dense;
- post-hoc fusion sweeps on B3 rankings still did not beat dense on MRR/MAP,
  so this is not fixed by scalar runtime weights alone.

B4 should reuse B3 rows and become ranking-first:

- do not rebuild surfaces;
- raise teacher KL and final hard-pairwise pressure;
- reduce complement/admission pressure;
- select checkpoints by MRR more than hit@20;
- keep residual fusion and no-self-teacher rows.

Recommended B4 probe:

```bash
RUN_NAME=bm25sae-m160-stageb-reset-b4-rankingcal-v1 \
SESSION=bm25sae_m160_stageb_reset_b4_rankingcal_v1 \
CONTAINER=bm25sae_m160_stageb_reset_b4_rankingcal_v1 \
LOG=/home/huoju/leask/logs/bm25sae_m160_stageb_reset_b4_rankingcal_v1.log \
REUSE_EXISTING_ROWS=1 \
TRAIN_ROWS=/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b3-no-selfteacher-v1-candidate-rows \
EVAL_ROWS=/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b3-no-selfteacher-v1-eval-candidate-rows \
FUSION_BM25_MODE=residual \
STEPS=3500 \
ST_AFTER=100000 \
FEATURE_K=96 \
INITIAL_SAE_SCALE=1.0 \
INITIAL_BM25_SCALE=0.28 \
TARGET_SAE_SCALE=1.0 \
TARGET_BM25_SCALE=0.28 \
LOSS_RECALL=1.0 \
LOSS_SINGLE_CE=0.10 \
LOSS_MULTI_CE=1.20 \
LOSS_TEACHER_KL=0.75 \
LOSS_COMPLEMENT=0.50 \
LOSS_FINAL_HARD_PAIRWISE=1.40 \
LOSS_SAE_HARD_PAIRWISE=0.40 \
LOSS_RECONSTRUCTION=0.02 \
LOSS_SCALE_REGULARIZATION=0.05 \
HARD_PAIRWISE_TOP_NEGATIVES=96 \
SELECTION_HIT_WEIGHT=0.70 \
SELECTION_MRR_WEIGHT=2.00 \
SELECTION_SCALE_DRIFT_WEIGHT=0.05 \
scripts/run_m160_stageb_reset_spark.sh
```

B4 pass signal is not local hit@20 alone. It needs either:

- official BM25+SAE MRR/NDCG/MAP move materially toward dense while preserving
  B3 recall; or
- a clear failure showing that B3 rows are too narrow, in which case the next
  step is broader full-corpus hard-negative rows, not another ranking loss
  tweak.

## B4 Result And B5 Matched-Surface Pivot

B4 confirmed that stronger ranking loss alone is not enough.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3180 | 0.2973 | 0.2261 | 0.1503 |
| B4 SAE-only | 0.2991 | 0.2727 | 0.2025 | 0.1388 |
| B4 BM25+SAE residual | 0.3110 | 0.2765 | 0.2039 | 0.1366 |

B4 improved BM25+SAE MRR over B3 (`0.2718` to `0.2765`) but lost
Recall@100 and MAP. This means the current bottleneck is not just pairwise
ranking pressure. The rows used by B3/B4 are still too narrow:

- B3/B4 train rows are mostly `fiqa`, `nfcorpus`, `scifact`, and a small
  `quora` slice.
- The continuity full-corpus gate is dominated by internal M-series sources
  (`m39`, `m60`, `m52`) plus a small BEIR slice.
- Therefore the model sees different hard negatives during training than it
  sees during the final gate.

B5 changes the axis again: train on a matched non-eval M-series surface before
trying more loss tuning.

Available matched source:

```text
/home/huoju/leask/data/ii42_sae_m130/all-data-source-v0
```

This root already contains PPLX embeddings and supervised candidate rows from:

- `broad_generated_query_surface`
- `large_supervised_split_surface`
- `beir15_current_eval_surface`

B5 must exclude `beir15_current_eval_surface`, and must select only train rows
for training. Validation should use only non-eval validation/holdout rows from
the same root. Do not use the 886-query continuity gate for checkpoint
selection; use it only for final full-corpus evaluation.

Candidate length differs from B3/B4:

- M130 all-data rows are mostly `80-128` candidates.
- B3/B4 rows were `192` candidates.

The first B5 probe should use `candidate_k=80` to preserve both broad-generated
and supervised rows. If this direction transfers, build a deeper B6 surface
with larger candidate rows rather than silently discarding the supervised rows.

Recommended B5 setup:

```bash
python3 scripts/research_sae_filter_candidate_rows.py \
  --source-root /home/huoju/leask/data/ii42_sae_m130/all-data-source-v0 \
  --output-root /home/huoju/leask/runs/bm25sae-m160-stageb-reset-b5-matched-v1-train-source \
  --include-split train \
  --exclude-source-family beir15_current_eval_surface \
  --min-candidates 80

python3 scripts/research_sae_filter_candidate_rows.py \
  --source-root /home/huoju/leask/data/ii42_sae_m130/all-data-source-v0 \
  --output-root /home/huoju/leask/runs/bm25sae-m160-stageb-reset-b5-matched-v1-validation-source \
  --include-split validation \
  --include-split holdout \
  --exclude-source-family beir15_current_eval_surface \
  --min-candidates 80

RUN_NAME=bm25sae-m160-stageb-reset-b5-matched-v1 \
SESSION=bm25sae_m160_stageb_reset_b5_matched_v1 \
CONTAINER=bm25sae_m160_stageb_reset_b5_matched_v1 \
LOG=/home/huoju/leask/logs/bm25sae_m160_stageb_reset_b5_matched_v1.log \
TRAIN_SPECS= \
EVAL_SPECS= \
EXTERNAL_TRAIN_ROOTS=/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b5-matched-v1-train-source \
EXTERNAL_EVAL_ROOTS=/home/huoju/leask/runs/bm25sae-m160-stageb-reset-b5-matched-v1-validation-source \
CANDIDATE_K=80 \
SOURCE_TOP_K=100 \
FUSION_BM25_MODE=residual \
STEPS=4500 \
ST_AFTER=100000 \
FEATURE_K=96 \
INITIAL_SAE_SCALE=1.0 \
INITIAL_BM25_SCALE=0.30 \
TARGET_SAE_SCALE=1.0 \
TARGET_BM25_SCALE=0.30 \
TEACHER_DENSE_WEIGHT=0.45 \
TEACHER_FUSION_RANK_WEIGHT=0.35 \
TEACHER_BM25_RANK_WEIGHT=0.05 \
TEACHER_BM25_SAE_RANK_WEIGHT=0.00 \
TEACHER_SAE_RANK_WEIGHT=0.20 \
ROW_WEIGHT_BM25_SAE_HIT=0.45 \
ROW_WEIGHT_DENSE_MISS=5.00 \
ROW_WEIGHT_SCORE_LOW=4.00 \
ROW_WEIGHT_NOT_RETRIEVED=2.00 \
LOSS_RECALL=1.3 \
LOSS_MULTI_CE=1.0 \
LOSS_TEACHER_KL=0.25 \
LOSS_COMPLEMENT=0.8 \
LOSS_FINAL_HARD_PAIRWISE=0.8 \
LOSS_SAE_HARD_PAIRWISE=0.8 \
LOSS_SCALE_REGULARIZATION=0.03 \
SELECTION_HIT_K=20 \
SELECTION_HIT_WEIGHT=1.1 \
SELECTION_MRR_WEIGHT=1.2 \
scripts/run_m160_stageb_reset_spark.sh
```

B5 pass/fail rule:

- If full-corpus Recall@100 improves while MRR/MAP do not collapse, the root
  cause was training-surface mismatch. Continue with a B6 larger-candidate
  matched surface.
- If B5 does not improve admission or ranking, stop B-series loss tuning and
  revisit candidate construction: the current M130 source rows are too shallow
  (`candidate_k=80`) for final BM25+SAE ranking.

## B5 Result And B6 Current-Surface Deep Rows

The first B5 matched-source attempt exposed a dimensionality mismatch:

- `/home/huoju/leask/data/ii42_sae_m130/all-data-source-v0` contains
  768-dimensional embeddings from an older Snowflake route;
- M160/PPLX checkpoints expect 1024-dimensional inputs;
- those rows cannot be mixed into the M160/PPLX Stage-B reset without changing
  the model input contract.

The fallback B5 probe used PPLX-compatible C0 rows rebuilt with no
BM25+SAE self-teacher weights:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3180 | 0.2978 | 0.2262 | 0.1503 |
| B5 SAE-only | 0.2915 | 0.2560 | 0.1963 | 0.1353 |
| B5 BM25+SAE residual | 0.3073 | 0.2575 | 0.1982 | 0.1356 |

B5 was excellent on its local candidate surface but regressed on the official
full-corpus gate. The important conclusion is that old C0 rows are stale: the
model learned to rank inside that row surface, but it did not learn to admit
or rank the current full-corpus false positives.

During this review we also found a runner bug that made the intended deeper
surface shallower than configured: `SOURCE_TOP_K` was passed to candidate-row
construction, but not to `research_sae_m110_full_corpus_index_eval.py`. The
rankings were therefore capped by the evaluator default `top_k=100`, even when
the run was configured with `SOURCE_TOP_K=500`.

B6 fixes the training surface before changing losses again:

- rebuild rows from the current M160A checkpoint;
- pass `--top-k "$SOURCE_TOP_K"` during full-corpus ranking generation;
- keep the no-self-teacher target from B3/B5;
- use candidate depth `192` over source top `500`;
- validate on the same continuity full-corpus gate before broader official
  expansion.

Recommended B6 probe:

```bash
RUN_NAME=bm25sae-m160-stageb-reset-b6-currentdeep-v1 \
SESSION=bm25sae_m160_stageb_reset_b6_currentdeep_v1 \
CONTAINER=bm25sae_m160_stageb_reset_b6_currentdeep_v1 \
LOG=/home/huoju/leask/logs/bm25sae_m160_stageb_reset_b6_currentdeep_v1.log \
FUSION_BM25_MODE=residual \
REUSE_EXISTING_ROWS=0 \
TRAIN_SPECS=fiqa:train:0,nfcorpus:train:0,scifact:train:0,quora:dev:1000 \
EVAL_SPECS=fiqa:dev:0,nfcorpus:dev:0 \
CANDIDATE_K=192 \
SOURCE_TOP_K=500 \
STEPS=4500 \
ST_AFTER=100000 \
FEATURE_K=96 \
INITIAL_SAE_SCALE=1.0 \
INITIAL_BM25_SCALE=0.30 \
TARGET_SAE_SCALE=1.0 \
TARGET_BM25_SCALE=0.30 \
TEACHER_DENSE_WEIGHT=0.45 \
TEACHER_FUSION_RANK_WEIGHT=0.35 \
TEACHER_BM25_RANK_WEIGHT=0.05 \
TEACHER_BM25_SAE_RANK_WEIGHT=0.00 \
TEACHER_SAE_RANK_WEIGHT=0.20 \
ROW_WEIGHT_BM25_SAE_HIT=0.45 \
ROW_WEIGHT_DENSE_MISS=5.00 \
ROW_WEIGHT_SCORE_LOW=4.00 \
ROW_WEIGHT_NOT_RETRIEVED=2.00 \
LOSS_RECALL=1.3 \
LOSS_MULTI_CE=1.0 \
LOSS_TEACHER_KL=0.25 \
LOSS_COMPLEMENT=0.8 \
LOSS_FINAL_HARD_PAIRWISE=0.8 \
LOSS_SAE_HARD_PAIRWISE=0.8 \
LOSS_SCALE_REGULARIZATION=0.03 \
SELECTION_HIT_K=20 \
SELECTION_HIT_WEIGHT=1.1 \
SELECTION_MRR_WEIGHT=1.2 \
scripts/run_m160_stageb_reset_spark.sh
```

B6 pass/fail rule:

- if local metrics and full-corpus metrics both improve, the main blocker was
  stale/shallow candidate rows;
- if local metrics improve but full-corpus still regresses, rows must be built
  from broader non-test official surfaces and large-corpus BM25 caches before
  changing the model loss again;
- if local metrics do not improve, the B-series reset is not learning useful
  current-surface structure and should stop.

## B6 Result And Decision

B6 fixed the row-depth bug and rebuilt current M160A full-corpus rows with
`top_k=500`. This produced real deeper rankings, for example:

- `fiqa_train_max0` rankings: `894,913,559` bytes;
- `nfcorpus_train_max0` rankings: `452,126,395` bytes;
- `scifact_train_max0` rankings: `145,493,006` bytes;
- candidate rows retained `source_top_k=500` and `candidate_k=192`.

The local validation surface improved but did not catch dense:

| Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | Dense Hit@20 | Dense MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 0.7937 | 0.5531 | 0.8204 | 0.5954 |
| 3000 | 0.8083 | 0.5614 | 0.8204 | 0.5954 |
| 3750 | 0.8070 | 0.5582 | 0.8204 | 0.5954 |

The full-corpus continuity gate still regressed:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3179 | 0.2978 | 0.2260 | 0.1499 |
| B6 SAE-only | 0.2956 | 0.2627 | 0.1992 | 0.1372 |
| B6 BM25+SAE residual | 0.3088 | 0.2658 | 0.2012 | 0.1369 |

Decision:

- the top100 truncation bug was real and is now fixed;
- deeper current rows alone are not enough;
- B6 further confirms that the current Stage-B reset is overfitting the
  small validation surface and failing the continuity distribution;
- do not run another B-series loss tweak on the same
  `fiqa/nfcorpus/scifact/quora` row family.

The next useful branch must change the data surface, not only the loss:

- add broader official non-test surfaces from large datasets, which requires
  building or caching BM25 rankings for large corpora;
- or change Stage-B into a full-corpus sampled objective over the M130/PPLX
  all-data root, where training rows are sampled directly from the same
  993k-doc continuity distribution used by the final gate.

The second route is the cleaner next probe because it directly targets the
distribution that keeps failing. It should first run a small query sample over
the 993k-doc root, not the whole 205k-query corpus.

## B7 Strict-Preflight Large Streaming Surface

B7 returns to the broader official non-test surface, but with strict data
guards before training starts. The goal is to test whether the Stage-B failure
is caused by the narrow `fiqa/nfcorpus/scifact/quora` row distribution rather
than by the M160A representation itself.

New guardrails:

- `scripts/research_sae_stageb_preflight.py` checks checkpoint shape,
  materialized embedding dimensions, qrels split availability, no train/test
  leakage, candidate row length, sampled row positives, score vectors, and
  sampled query/document embedding dimensions.
- M160/PPLX Stage-B rows must be 1024-dimensional and use 16,384 SAE features.
  The old 768-dimensional Snowflake rows remain invalid for this path.
- Regular one-process Stage-B surfaces still require BM25 caches. Large
  corpora without BM25 caches must use the streaming path until those caches
  are intentionally built.
- The runner performs preflight before row generation and again after merged
  candidate rows are written. Training only starts if both checks pass.

Runner:

```bash
scripts/run_m160_stageb_streaming_large_spark.sh
```

Default run:

```text
run_name: bm25sae-m160-stageb-b7-streaming-large-v1
checkpoint: /home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1/bm25sae_stagea_latest.pt
clearml_project: ii42_sae/M160
```

Base current rows:

| Root | Role | Reason |
| --- | --- | --- |
| `bm25sae-m160-stageb-reset-b6-currentdeep-v1-candidate-rows` | train | validated current M160A small/medium rows with source top-k 500 |
| `bm25sae-m160-stageb-reset-b6-currentdeep-v1-eval-candidate-rows` | validation | same B6 validation surface |

Streaming train specs:

| Dataset | Split | Max queries | Reason |
| --- | --- | ---: | --- |
| `msmarco` | `train` | 1000 | large short-query retrieval pressure |
| `hotpotqa` | `train` | 1000 | large QA semantic retrieval |
| `fever` | `train` | 1000 | large fact-verification retrieval |
| `dbpedia-entity` | `dev` | 1000 | entity-heavy semantic/lexical mix; no train split |

Streaming validation specs:

| Dataset | Split | Max queries |
| --- | --- | ---: |
| `msmarco` | `dev` | 300 |
| `hotpotqa` | `dev` | 300 |
| `fever` | `dev` | 300 |

This first B7 branch intentionally uses streaming dense hard negatives for the
large corpora. That does not fully solve BM25 false-positive supervision, but
it is the minimal safe step that brings large official non-test corpora into
Stage-B without silently depending on missing large-corpus BM25 caches.

If B7 improves full-corpus continuity transfer, the next branch should add
streaming BM25 cache construction or sharded BM25 rankings for the same large
datasets. If B7 does not improve transfer, the blocker is likely not just large
corpus exposure and Stage-B loss/target construction must be revisited.

## B7 Result And B8 Direction

B7 completed the strict preflight path successfully:

- pre-build and post-merge preflight both passed;
- checkpoint shape was `d_model=1024`, `n_features=16384`;
- post-merge train rows: `12,964`;
- post-merge validation rows: `1,724`;
- source families included B6 rows plus `msmarco`, `hotpotqa`, `fever`, and
  `dbpedia-entity` streaming rows;
- no test-split train leakage or 768/1024 dimension mismatch was detected.

The candidate validation surface improved through training, but did not catch
dense:

| Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | Dense Hit@20 | Dense MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 0.8266 | 0.6111 | 0.8532 | 0.6581 |
| 1500 | 0.8324 | 0.6153 | 0.8532 | 0.6581 |
| 3500 | 0.8318 | 0.6196 | 0.8532 | 0.6581 |
| 5500 | 0.8329 | 0.6190 | 0.8532 | 0.6581 |

Official full-corpus continuity gate:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3178 | 0.2978 | 0.2260 | 0.1499 |
| B7 SAE-only | 0.2915 | 0.2616 | 0.1976 | 0.1359 |
| B7 BM25+SAE RRF | 0.3018 | 0.2599 | 0.1863 | 0.1231 |
| B7 BM25+SAE score fusion | 0.3059 | 0.2658 | 0.2005 | 0.1358 |

B7 result:

- strict corpus and dimension hygiene worked;
- adding large streaming dense-near-miss rows helped the local validation
  surface, but did not transfer to the final full-corpus gate;
- B7 BM25+SAE score fusion remained below dense by `Recall@100 -0.0072`,
  `MRR@20 -0.0334`, `NDCG@10 -0.0247`, and `MAP@100 -0.0146`;
- BM25+dense still remained the stronger hybrid baseline on this gate.

Decision:

- do not promote B7;
- do not run another dense-near-miss-only B-series variant;
- the next useful probe is B8, which must include full-corpus BM25
  false-positive and admission/ranking supervision.

B8 should keep B7's strict preflight path, but change row construction:

- build sharded BM25 top-k or reusable BM25 caches for the same large
  official non-test datasets;
- include documents that BM25 ranks highly but qrels/dense/SAE do not support
  as explicit hard negatives;
- optimize final BM25+SAE top-100 admission and ranking directly, instead of
  only distilling dense-neighborhood rows;
- keep M160A as the Stage-A checkpoint and keep 1024-dimensional PPLX rows as
  the only valid row contract.

B8 promotion gate is unchanged: full-corpus BM25+SAE must beat dense and move
toward or past BM25+dense on Recall@100, MRR@20, NDCG@10, and MAP@100 without
materially worsening fanout.

## B8 Execution

B8 is now defined as `bm25sae-m160-stageb-b8-bm25falsepositive-v1`.

It does not restart Stage A. The checkpoint remains:

```text
/home/huoju/leask/runs/bm25sae-m160a-pplx16384k96-stagea-cont-e3-v1/bm25sae_stagea_latest.pt
```

The execution wrapper is:

```bash
scripts/run_m160_stageb_b8_bm25_false_positive_spark.sh
```

The underlying reusable Spark runner remains:

```bash
scripts/run_m160_stageb_streaming_large_spark.sh
```

B8 changes the streaming row builder to:

```bash
scripts/research_sae_build_streaming_bm25_dense_qrels_rows.py
```

This builder makes two streaming passes over each selected full corpus:

1. collect BM25 document-frequency statistics for the selected query terms;
2. produce dense top-k and BM25 top-k hard-negative rankings without loading
   the full large corpus into a single in-memory evaluator.

The generated candidate rows include:

- qrels positives;
- dense top-k candidates;
- BM25 top-k candidates;
- score-fused BM25+dense candidates;
- RRF-fused BM25+dense candidates;
- nonzero `bm25_scores` for the final BM25+SAE training surface.

The post-merge preflight now runs with `--require-bm25-scores`. This is a
hard guardrail: B8 must fail before training if the merged train/eval rows are
still dense-only or if BM25 scores are accidentally all zero.

Default B8 supervised surface:

| Role | Dataset | Split | Max queries |
| --- | --- | --- | ---: |
| Train | `msmarco` | `train` | 1000 |
| Train | `hotpotqa` | `train` | 1000 |
| Train | `fever` | `train` | 1000 |
| Train | `dbpedia-entity` | `dev` | 1000 |
| Validation | `msmarco` | `dev` | 300 |
| Validation | `hotpotqa` | `dev` | 300 |
| Validation | `fever` | `dev` | 300 |

Important B8 defaults:

| Parameter | Value |
| --- | ---: |
| candidate rows per query | 192 |
| dense source top-k | 500 |
| BM25 source top-k | 500 |
| fusion source top-k | 500 |
| query active features | 96 |
| BM25 scale | 0.25 |
| SAE scale | 1.00 |
| final hard-pairwise loss | 1.10 |
| SAE hard-pairwise loss | 0.70 |
| complement loss | 1.00 |
| streaming row-builder device | CPU |

This is intentionally stricter than B7: if the problem is BM25 false-positive
admission/ranking, B8 should expose it directly. If B8 still fails the official
gate, the next blocker is likely loss design or query-side calibration rather
than another dense-neighborhood data expansion.

Early execution note: the first `msmarco` train shard completed with nonzero
BM25 scores, but the second shard hit a CUDA OOM during streaming dense
ranking. B8 now keeps the training/evaluation phases on GPU while forcing the
row-builder dense scoring path to CPU. This makes row construction slower but
removes GB10 CUDA memory instability from the data-preparation path.
