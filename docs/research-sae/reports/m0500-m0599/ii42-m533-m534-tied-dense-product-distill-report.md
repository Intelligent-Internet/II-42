# M533-M534 Dense Product Distillation Report

## Route Correction

The current first-stage encoder objective should be query-independent.  It
should learn from corpus documents only:

```text
document text -> dense teacher product -> posting coordinates
```

Queries, qrels, BM25 features, route positives, and ranking losses are not part
of this stage.  Ranking and hybrid fusion belong to a later stage after the
text encoder can reproduce the dense-derived product.

## Why M533 Is Not Enough

M533 tested a cached neural adapter:

```text
raw PPLX denseout -> residual relational adapter -> teacher product
```

Run:

```text
outputs/m533/cached_relational_product_adapter/
m533_broad4_8k_e12_seed5330.json
```

Macro result:

| Source | Active Recall | Cosine | Active Jaccard |
| --- | ---: | ---: | ---: |
| `raw` | 0.93707 | 0.99124 | 0.89147 |
| `adapted` | 0.93707 | 0.99124 | 0.89147 |

Training selected `best_epoch=0`.  The 25M-parameter relational adapter did not
beat the raw denseout floor.  That means raw-to-teacher calibration is not the
right place to spend more iterations unless the teacher surface changes.

## M534 Shape Fix

M526 had a shape error for `dense_signed`: it trained a separate posting head
even though the posting product is directly derived from dense coordinates.
That extra head can preserve cosine while destroying top active dimensions.

M534 removes the extra posting head:

```text
document text -> PPLX root + dense head/LoRA -> dense teacher product
posting = top-k signed coordinates of the dense output
```

It also adds an epoch-0 raw baseline and best-state gate, so a trained model is
only promoted if it beats raw PPLX denseout on heldout corpus documents.

## FiQA Smoke Results

All runs use only FiQA corpus documents.  No query/qrel/BM25/ranking signal is
used.

| Run | Adapter | Active-Rank Loss | Rows | Best Epoch | Active Recall | Cosine |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `m534_fiqa_head_smoke_seed5340` | head | 0 | 2048 | 0 | 0.94603 | 0.99026 |
| `m534_fiqa_lora_smoke_seed5341` | LoRA | 0 | 2048 | 0 | 0.94600 | 0.99026 |
| `m534_fiqa_head_rank_smoke_seed5342` | head | 10 | 2048 | 0 | 0.94603 | 0.99026 |
| `m534_fiqa_lora_rank_smoke_seed5343` | LoRA | 10 | 2048 | 0 | 0.94600 | 0.99026 |

Observed training traces:

| Run | Epoch 0 Active | Epoch 1 Active | Epoch 2 Active |
| --- | ---: | ---: | ---: |
| head, continuous | 0.94603 | 0.93863 | 0.93477 |
| LoRA, continuous | 0.94600 | 0.93652 | 0.93163 |
| head, active-rank | 0.94603 | 0.94307 | 0.94107 |
| LoRA, active-rank | 0.94600 | 0.94164 | 0.93954 |

The active-rank margin loss improves the direction versus the continuous loss,
but it still does not beat the raw floor in the small FiQA setting.

## Current Long Run

The controlled scale-up is running on `spark-1`:

```text
tmux: ii42_m534_broad4_lora_rank
run dir:
/home/huoju/leask/runs/ii42-m534-tied-dense-product-distill-v1/
broad4_lora_rank_e3
```

Configuration:

| Field | Value |
| --- | --- |
| datasets | `FiQA2018, ArguAna, SCIDOCS, TRECCOVID` |
| train rows | up to 8192 documents per dataset |
| eval rows | 1024 documents per dataset |
| adapter | LoRA, last 2 layers, rank 8 |
| epochs | 3 |
| objective | tied dense product + active-rank margin |
| guard | epoch-0 raw floor |

This run is the correct test for the user's scale concern.  It uses more corpus
documents without introducing query-specific or dataset-specific retrieval
optimization.

## Interpretation

The route is now corrected, but the current evidence is conservative:

1. Raw PPLX denseout already matches the materialized dense teacher closely.
2. Simple continuous distillation lowers training loss while hurting the active
   posting set.
3. Active-rank margin moves in the right direction but has not exceeded raw in
   FiQA smoke.
4. The broad4 exact-top128 run started as the scale test, but epoch 1 moved in
   the same bad direction as the FiQA smokes: active recall dropped from
   0.93707 to 0.92941.  That is enough to stop spending GPU on exact-top128 as
   the primary gate.

## M535 Active Boundary Diagnostic

M535 inspects the M531 broad4 cached raw/teacher denseout pairs without
training.  It asks whether exact teacher top-128 coordinates are a stable
first-stage target.

Run:

```text
outputs/m535/cached_active_margin/m535_broad4_8k_margin_seed5350.json
```

Macro results:

| Metric | Value |
| --- | ---: |
| raw top128 recall teacher top128 | 0.937071 |
| raw top160 recall teacher top128 | 0.973296 |
| raw top192 recall teacher top128 | 0.984505 |
| raw top224 recall teacher top128 | 0.990176 |
| raw top256 recall teacher top128 | 0.993518 |
| raw top384 recall teacher top128 | 0.998177 |
| raw top512 recall teacher top128 | 0.999307 |

Teacher boundary gap quantiles:

| Quantile | Gap |
| --- | ---: |
| p10 | 0.00000000 |
| p25 | 0.00000000 |
| p50 | 0.00000000 |
| p75 | 0.00000000 |
| p90 | 0.00000000 |
| p95 | 0.00159997 |
| p99 | 0.00221150 |

This is the strongest diagnostic so far.  Exact top-128 is not a stable target:
the teacher's own boundary has many ties or near-ties.  Raw denseout already
covers almost all teacher top-128 coordinates when allowed an overcomplete
support prefix: top256 covers 99.35%, and top512 covers 99.93%.

The next first-stage target should therefore be overcomplete support retention,
not exact top128 equality.  The encoder can emit a wider posting support, while
stage two handles scoring, pruning, or ranking.

## Route-Layer Cross-Check

Existing M528 route results are consistent with the M535 representation
diagnostic.  On the broad4 route subset, frozen raw denseout support already
has near-complete candidate coverage with a wider prefix:

| Source | Candidate R@100 | NDCG@10 | Dense O@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| `m528_raw_denseout_candidates_p128` | 0.99350 | 0.47695 | 0.93010 | 0.90563 |
| `m528_raw_denseout_candidates_p160` | 0.99676 | 0.47697 | 0.93141 | 0.93624 |
| `m528_raw_denseout_candidates_p192` | 0.99836 | 0.47697 | 0.93220 | 0.95538 |
| `m528_raw_denseout_candidates_p224` | 0.99906 | 0.47697 | 0.93261 | 0.96798 |
| `route_subset_materialized_dense` | 1.00000 | 0.48043 | 0.99809 | 1.00000 |
| `route_subset_teacher_row_int8_dense` | 1.00000 | 0.48032 | 1.00000 | 1.00000 |

This means support generation is almost solved at overcomplete prefix sizes.
The remaining route gap is mostly scoring/ranking, not corpus-support recall.

M529 confirms this with a reranking upper bound.  Using the same raw denseout
candidates, teacher dense reranking closes the gap:

| Source | Candidate R@100 | NDCG@10 | MAP@100 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: |
| `m529_raw_denseout_route_p224` | 0.99906 | 0.47697 | 0.24945 | 0.93304 |
| `m529_raw_denseout_teacher_rerank_p224` | 0.99906 | 0.48032 | 0.25552 | 0.99906 |
| `route_subset_materialized_dense` | 1.00000 | 0.48043 | 0.25562 | 0.99817 |
| `route_subset_teacher_row_int8_dense` | 1.00000 | 0.48032 | 0.25557 | 1.00000 |

So the current useful route is:

```text
encoder -> overcomplete support/candidates -> learned/self scorer
```

not:

```text
encoder -> exact top128 support
```

If broad4 also selects epoch 0, the next change should not be another small
learning-rate tweak.  The next real options are:

1. Change the teacher product surface to an overcomplete or soft active support
   target so the target has stable margins.
2. Train a larger student with a representation-distillation phase before any
   posting top-k objective.
3. Move the quality gain to stage two: keep raw denseout for candidate/support
   and learn a ranking/scoring layer separately.

## M537 Cached Overcomplete Calibration Diagnostic

M537 checks whether output-space calibration alone has useful headroom.  It uses
the cached M531 raw/teacher denseout train/eval pairs and fits two simple
calibrators per task:

- diagonal scale;
- full ridge map with lambda 0.01.

It uses no text-model training, queries, qrels, BM25 features, route positives,
or ranking losses.

Run:

```text
outputs/m537/cached_overcomplete_calibration/
  m537_broad4_8k_calibration_seed5370.json
```

Macro results:

| Method | Top256 retain teacher top128 | Top128 | Top512 | Cosine |
| --- | ---: | ---: | ---: | ---: |
| raw | 0.993518 | 0.937071 | 0.999307 | 0.991237 |
| diagonal | 0.993515 | 0.936192 | 0.999306 | 0.991271 |
| ridge | 0.993944 | 0.919661 | 0.999366 | 0.990623 |

The output-space headroom is real but tiny.  Ridge improves top256 retention by
only 0.000426 absolute, while hurting exact top128 and row cosine.  This makes a
pure output-calibration route weak.  If M536 fails to improve over epoch0, the
next route should be deeper representation distillation or a stage-two scorer,
not another shallow output adapter.

## M539 Cached Nonlinear Calibration Diagnostic

M539 tests a stronger non-linear output-space adapter over cached M531
raw/teacher denseout pairs:

```text
cached raw PPLX denseout -> residual MLP -> teacher dense/product
```

It is still corpus-only and query-independent, and it does not train the text
encoder.  It uses no queries, qrels, BM25 features, route positives, or ranking
losses.

Run:

```text
outputs/m539/cached_nonlinear_product_calibration/
  m539_broad4_8k_nonlinear_seed5390.json
```

Macro results:

| Method | Top256 retain teacher top128 | Top128 | Top512 | Cosine |
| --- | ---: | ---: | ---: | ---: |
| raw | 0.993518 | 0.937071 | 0.999307 | 0.991237 |
| residual MLP | 0.993549 | 0.911980 | 0.999326 | 0.989773 |

The non-linear adapter improves top256 by only 0.000031 absolute and damages
both exact top128 and row cosine.  Together with M537, this says the remaining
gap is not a simple output calibration problem.  A productive next run must
either change the representation-training schedule, as M538 does, or move the
quality target into a stage-two scorer.

## M540 Row-Only Corpus Product Pretraining

The next correction is stricter than M536/M538.  The first-stage model should
not learn exact support membership, overcomplete support rank, qrels, BM25, or
query-document ranking.  It should first learn only this corpus-side map:

```text
document text -> PPLX root + adapter -> dense-derived teacher row
```

M540 implements that as a row-only objective:

- loss: row cosine, row MSE, and optional batch relational similarity;
- diagnostics only: top-k support retention, active recall, sign accuracy;
- checkpoint selector: cosine, then MSE, then active recall;
- excluded: queries, qrels, BM25 features, route positives, support/rank loss,
  and ranking loss.

This directly tests the revised hypothesis: if an integrated encoder can be
made useful, it should first preserve the dense-derived corpus product at large
scale before any posting-support or search-specific objective is introduced.
If M540 cannot improve row/product fidelity over raw PPLX denseout, the right
next move is not another support-loss tweak; it is either a much larger teacher
cache/pretraining setup or treating raw PPLX denseout as already near-optimal
for stage-one corpus representation.

Current spark-1 run:

```text
tmux: ii42_m540_broad4_full_lora16_b8_e1_row
run dir:
/home/huoju/leask/runs/ii42-m540-corpus-row-product-pretrain-v1/
  broad4_full_lora16_b8_e1_rowonly_ckpt
```

Configuration:

| Field | Value |
| --- | --- |
| datasets | `FiQA2018, ArguAna, SCIDOCS, TRECCOVID` |
| train rows | all available corpus docs, currently 263301 rows |
| eval rows | 2048 per dataset |
| adapter | LoRA, last 4 layers, rank 16 |
| batch size | 8 |
| epochs | 1 |
| objective | row cosine + row MSE |
| checkpoint | cosine, then MSE, then active recall |
| saved model | trainable adapter checkpoint enabled |
| excluded | query, qrel, BM25, support/rank loss, ranking loss |

Note: an earlier batch-8 relational run was stopped after step losses rose from
0.000974 to 0.022957 by step 3000.  A batch-4 row-only run showed similar
single-batch variance but no OOM.  The current gate keeps batch 8 for throughput
but removes relational loss; single-step loss variance is no longer used as a
stop signal.  The decision gate is epoch-level eval against the raw row floor.

Parallel control run on spark-2:

```text
tmux: ii42_m540_broad4_head_b16_e1_retry3
run dir:
/home/huoju/leask/runs/ii42-m540-corpus-row-product-pretrain-v1/
  broad4_full_head_b16_e1_rowonly_ckpt_retry3
```

This control freezes the PPLX root (`adapter=head`, `train_last_layers=0`) and
trains only the dense output head with the same row-only objective.  It tests
whether output-head calibration alone can beat the raw dense row floor before
spending more time on LoRA/root updates.

The important scale correction is that stage one should be driven by corpus
materialization, not by retrieval labels.  The broad4 runs are a controlled
first pass over 263301 corpus rows.  If the epoch-level gate improves over the
raw dense row floor, the next scale-up should expand the same document-only
teacher generation to broad8 or a larger unlabeled corpus.  Query-aware ranking,
BM25, hybrid fusion, and per-dataset optimization stay out of this stage.  They
only become valid after the text encoder can reproduce the dense-derived corpus
product without losing row fidelity.

If M540 improves row fidelity, the next stage should not restart from raw PPLX.
M536 now supports `--init-trainable-checkpoint`, so support/posting refinement
can initialize from the M540 trainable adapter checkpoint and then add the
overcomplete support losses.  That keeps the route staged:

```text
stage 1: corpus text -> dense-derived teacher row
stage 2: row-initialized encoder -> overcomplete posting support
stage 3: retrieval/ranking/fusion, only after learned support earns promotion
```

Completed M540 broad4 full-corpus gates:

| Run | Adapter | Best Epoch | Cosine Delta | MSE Delta | Top256 Delta | Soft KL Delta | Active Delta |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `m540_broad4_full_lora16_b8_e1_seed5401` | LoRA last4 r16 | 1 | +0.000307 | -0.000000600 | +0.000443 | -0.017038 | -0.002939 |
| `m540_broad4_full_head_b16_e1_retry3_seed5402` | head only | 1 | +0.000248 | -0.000000484 | +0.000463 | -0.020710 | -0.004729 |

Both runs pass the row-first gate: row cosine improves, MSE improves, top256
retention of teacher top128 improves, and soft support KL drops.  Exact top128
active recall drops, especially for head-only.  That does not invalidate the
row-first result because M535 already showed exact top128 boundaries are
unstable; it does require stage two to prove overcomplete support can improve
without destroying the row geometry.

Active stage-two runs:

| Run | Machine | Initialization | Adapter | Purpose |
| --- | --- | --- | --- | --- |
| `m536_broad4_full_from_m540_lora16_s512_e1_seed5361` | `spark-1` | M540 LoRA checkpoint | LoRA last4 r16 | Test whether conservative support512 refinement can improve overcomplete support after row pretraining. |
| `m536_broad4_full_from_m540_head_s512_e1_seed5362` | `spark-2` | M540 head checkpoint | head only | Control for whether output-head/support refinement is enough without root LoRA. |

The initial stage-two controls both finished and selected epoch 0.  Epoch 1
reduced soft support KL, but it damaged row geometry and support retention:

| Run | Metric | Epoch 0 | Epoch 1 | Delta |
| --- | --- | ---: | ---: | ---: |
| head-only | top256 retain teacher top128 | 0.994358 | 0.994292 | -0.000066 |
| head-only | active recall | 0.933170 | 0.926780 | -0.006391 |
| head-only | row cosine | 0.991389 | 0.991005 | -0.000384 |
| head-only | soft support KL | 0.110033 | 0.098047 | -0.011986 |
| LoRA | top256 retain teacher top128 | 0.994252 | 0.994170 | -0.000082 |
| LoRA | active recall | 0.935037 | 0.929180 | -0.005857 |
| LoRA | row cosine | 0.991728 | 0.991433 | -0.000295 |
| LoRA | soft support KL | 0.115840 | 0.105583 | -0.010258 |

This is a useful negative control: support/KL optimization can still pull the
model away from row fidelity.  The failure is not just head-only capacity,
because the LoRA run shows the same pattern.  Stage two therefore needs a loss
redesign rather than more tuning of the same support-rank/BCE mixture.

A conservative diagnostic run completed on `spark-1`:

```text
tmux: ii42_m536_broad4_full_from_m540_lora16_s512_klonly_e1
run dir:
/home/huoju/leask/runs/ii42-m536-overcomplete-corpus-product-distill-v1/
  broad4_full_from_m540_lora16_s512_klonly_e1
```

This run keeps the M540 LoRA checkpoint, strengthens row cosine/MSE weights,
uses only a weak soft-support KL term, and disables support-rank, BCE, sign,
inactive, and magnitude losses.  It tests whether stage two can reduce soft
support mismatch without breaking row fidelity when the hard support objectives
are removed.

Result:

| Metric | Epoch 0 | Epoch 1 | Delta |
| --- | ---: | ---: | ---: |
| top256 retain teacher top128 | 0.994289 | 0.994279 | -0.000010 |
| active recall | 0.936096 | 0.934967 | -0.001129 |
| row cosine | 0.991837 | 0.991823 | -0.000015 |
| MSE | 0.000015943 | 0.000015970 | +0.000000027 |
| soft support KL | 0.113569 | 0.108661 | -0.004908 |

Training selected epoch 0.  The weak KL-only variant reduces soft support KL,
but it still slightly damages the row/selection surface.  This closes the
current M536 stage-two family: the failure is not only hard support BCE/rank
loss, because even weak KL pulls against the row-preservation gate.

The follow-up on `spark-1` is M542, a pure row-only continuation from the M540
LoRA checkpoint.  It deliberately removes the remaining KL term:

```text
tmux: ii42_m542_broad4_full_from_m540_lora16_rowonly_lr5e8_e1
run dir:
/home/huoju/leask/runs/ii42-m542-row-only-continuation-v1/
  broad4_full_from_m540_lora16_rowonly_lr5e8_e1
```

M542 is a clean diagnostic.  If it cannot improve epoch 0, broad4 M540 is likely
already at the local row-fit limit and the next decision should come from the
larger M541 broad10 row-first run.

M542 completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m542-row-only-continuation-v1/
  broad4_full_from_m540_lora16_rowonly_lr5e8_e1/
  m542_broad4_full_from_m540_lora16_rowonly_lr5e8_e1_seed5420.json
```

Result:

| Metric | Epoch 0 | Epoch 1 | Delta |
| --- | ---: | ---: | ---: |
| top256 retain teacher top128 | 0.994436 | 0.994441 | +0.000005 |
| active recall | 0.936559 | 0.936529 | -0.000030 |
| row cosine | 0.991746 | 0.991752 | +0.000007 |
| MSE | 0.000015645 | 0.000015632 | -0.000000013 |
| soft support KL | 0.113464 | 0.113206 | -0.000259 |
| top512 retain teacher top512 | 0.958754 | 0.958739 | -0.000015 |
| active jaccard | 0.889155 | 0.889080 | -0.000075 |

Training selected epoch 1 by the narrow top256/cosine/MSE selector, but the
gain is extremely small and the broader active-support metrics move backward.
This is not a clean promotion.  Do not rerun M542 just to save a checkpoint, and
do not continue broad4 row-only micro-tuning.  The next decision should come
from M541, which tests whether the same row-first objective benefits from a
larger broad10 corpus signal.

Implementation note: M536 originally loaded `--init-trainable-checkpoint` but
did not save a best trainable checkpoint.  The runner now accepts optional
`--checkpoint-name` and saves the best trainable state when requested.  The
change was synced to both `spark-1` and `spark-2` and `py_compile` passed on
both remote repos.  The M542 job was launched before this artifact fix, but its
final gate is not positive enough to justify a checkpoint rerun.

In parallel, `spark-2` is running the next scale test:

```text
tmux: ii42_m541_broad10_cap100k_lora16_b8_e1_row
run dir:
/home/huoju/leask/runs/ii42-m541-broad10-balanced-corpus-row-pretrain-v1/
  broad10_cap100k_lora16_b8_e1_rowonly_ckpt
```

M541 keeps the M540 row-only objective but scales the corpus signal to all ten
available shared-root tasks with a per-task cap of 100000 documents.  The cap is
not a retrieval-label optimization; it prevents very large corpora from
dominating the query-independent teacher-fit stage.

M541 completed on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m541-broad10-balanced-corpus-row-pretrain-v1/
  broad10_cap100k_lora16_b8_e1_rowonly_ckpt/
  m541_broad10_cap100k_lora16_b8_e1_seed5410.json
```

Result:

| Metric | Epoch 0 | Epoch 1 | Delta |
| --- | ---: | ---: | ---: |
| top256 retain teacher top128 | 0.987058 | 0.988045 | +0.000987 |
| active recall | 0.933027 | 0.926974 | -0.006052 |
| row cosine | 0.986951 | 0.987410 | +0.000459 |
| MSE | 0.000025487 | 0.000024590 | -0.000000897 |
| soft support KL | 0.183491 | 0.166585 | -0.016906 |
| top512 retain teacher top512 | 0.956599 | 0.952588 | -0.004012 |
| active magnitude cosine | 0.995967 | 0.996199 | +0.000232 |
| active jaccard | 0.889216 | 0.876843 | -0.012373 |

Training selected epoch 1.  This is a real positive result for the stage-one
row objective: broad10 corpus-only training improves row cosine, MSE, top256
teacher-top128 retention, active magnitude cosine, and soft support KL.  It is
not a posting-support breakthrough: exact active recall, top512 retention, and
active jaccard drop.  The conclusion is therefore narrow but useful.  Scaling
document-only row distillation helps the dense-derived product surface, while
the next support/posting stage still needs a loss that does not trade away
active support geometry.

## M543-M545 Support-Safe Stage-Two Gate

M543 is the handoff gate for the M541 checkpoint.  It runs the M536 evaluator
with `--epochs 0` from the M541 trainable checkpoint on the same broad10 task
set.  It must reproduce the M541 row/support surface before any support-stage
training uses that checkpoint.

M543 completed on `spark-1` and exactly reproduced the M541 checkpoint surface:

```text
run dir:
/home/huoju/leask/runs/ii42-m543-m541-checkpoint-handoff-v1/
  broad10_cap100k_lora16_eval_only_seed5410/
json:
  m543_m541_broad10_eval_only_seed5410.json
elapsed: 1340.123s
loaded tensors: 57
max absolute delta vs M541 summary: 0.0
```

| Metric | M541 | M543 | Delta |
| --- | ---: | ---: | ---: |
| top256 retain teacher top128 | 0.988045 | 0.988045 | 0.000000 |
| active recall | 0.926974 | 0.926974 | 0.000000 |
| row cosine | 0.987410 | 0.987410 | 0.000000 |
| MSE | 0.000024590 | 0.000024590 | 0.000000000 |
| soft support KL | 0.166585 | 0.166585 | 0.000000 |
| top512 retain teacher top512 | 0.952588 | 0.952588 | 0.000000 |
| active magnitude cosine | 0.996199 | 0.996199 | 0.000000 |
| active jaccard | 0.876843 | 0.876843 | 0.000000 |

M544 implements the next support-stage canary.  It does not restart the old
M536/M538 support losses.  Instead it keeps a frozen M541 checkpoint model as a
row anchor while training a second model with weak support pressure:

```text
trainable M541 checkpoint -> teacher support refinement
frozen M541 checkpoint -> row-anchor constraint
```

The first M544 run should be a broad10 cap20k canary, not a full broad10 run.
Its promotion gate is support-safe: improve the top256/soft-KL surface without
violating row cosine, active recall, top512 retention, or active-jaccard floors.
If it fails, the conclusion is that stage-two needs a different support target,
not more learning-rate tuning.

M544 was launched on `spark-2` after the M543 handoff gate passed:

```text
tmux:
  ii42_m544_broad10_cap20k_from_m541_anchor_e1
run dir:
  /home/huoju/leask/runs/ii42-m544-support-safe-stage2-v1/
    broad10_cap20k_from_m541_anchor_e1/
json:
  m544_broad10_cap20k_from_m541_anchor_e1_seed5440.json
checkpoint:
  m544_broad10_cap20k_from_m541_anchor_e1_seed5440_trainable.pt
status:
  completed
```

M545 was launched in parallel on `spark-1` as a controlled ablation, not as a
second learning-rate sweep.  It uses the same M541 checkpoint, frozen-anchor
constraint, corpus cap, and promotion floors as M544, but removes both rank
hinge terms:

```text
soft support KL + support magnitude only
no support-rank hinge
no teacher-active-rank hinge
```

The purpose is to identify whether M544's rank-pressure terms are responsible
for any row/support-geometry damage.  If M545 is safer than M544, the next
stage should keep soft distributional support targets and drop rank hinge.  If
both fail, the problem is not just the hinge shape; the stage-two target itself
needs to be redesigned.

```text
tmux:
  ii42_m545_broad10_klmag_anchor_e1
run dir:
  /home/huoju/leask/runs/ii42-m545-support-safe-klmag-ablation-v1/
    broad10_cap20k_from_m541_anchor_e1/
json:
  m545_broad10_cap20k_klmag_anchor_e1_seed5450.json
checkpoint:
  m545_broad10_cap20k_klmag_anchor_e1_seed5450_trainable.pt
status:
  completed
```

M544 and M545 both completed.  Both runs passed the support-safety floor at
epoch 1, but neither run improved the selection surface over its epoch0
checkpoint baseline.  The trainer therefore selected `best_epoch=0` for both
runs.

| Run | Objective | Best epoch | Gate | top256 retain teacher top128 | active recall | row cosine | soft support KL |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: |
| M544 | rank + KL + magnitude | 0 | pass | 0.988124 | 0.926708 | 0.987305 | 0.167347 |
| M545 | KL + magnitude only | 0 | pass | 0.987785 | 0.926271 | 0.987121 | 0.168101 |

Epoch1 live logs confirm the same conclusion:

```text
M544 epoch1: valid=1 sel=0.98807 active=0.92617 cos=0.98730
M545 epoch1: valid=1 sel=0.98771 active=0.92545 cos=0.98710
```

This is a negative result for the current support-safe stage-two family.  The
rank hinge was not the only problem: removing it in M545 made the objective
safer, but still did not create a selectable gain.  The next stage should not
scale M544/M545 to full broad10.  It should redesign the target so support
learning is not a weak perturbation of the row-preserving checkpoint.

Recommended next stage:

```text
M546: frozen dense-root output compiler
- keep the dense root and dense head frozen by default
- train only an output compiler from dense output to deterministic posting
- use document-only teacher-fit targets
- select by support cosine/MSE, soft KL, and prefix retention
- do not use queries, qrels, BM25, route positives, or ranking loss
```

The current evidence says the encoder can preserve the dense-product surface,
but post-hoc support pressure at this scale does not reliably improve it.  The
next useful experiment should isolate whether a small output layer can compile
the existing dense model output into the dense-derived posting target before any
downstream search objective is reintroduced.

Update 2026-06-30: the first M546 unconstrained linear canary answered one
part of this question negatively.  The epoch0 identity floor was already
strong, but one epoch of full 1024x1024 linear AdamW training damaged the
teacher-fit surface:

| Run | Epoch | Support Cos | Active Recall | Soft KL | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| M546 linear broad4 cap20k | 0 | 0.991566 | 0.93929 | 0.13148 | identity floor |
| M546 linear broad4 cap20k | 1 | 0.972254 | 0.82369 | 0.30133 | stopped |

This does not invalidate the output-compiler hypothesis.  It shows that the
compiler must be constrained near the dense output instead of letting an
unbounded linear layer move away from an already useful identity mapping.  The
active follow-up is therefore a delta residual compiler with identity-anchor
loss, lower learning rate, reduced support-pressure weights, and automatic
early stop against the epoch0 floor.

The guarded delta follow-up (`broad4_cap20k_delta_anchor_lr2e5_seed5461`)
completed with `best_epoch=1`.  It improved support cosine and soft KL but
still lowered hard active recall:

| Epoch | Support Cos | Active Recall | Soft KL | top256 retain teacher top128 |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 0.991255 | 0.93779 | 0.13331 | 0.993409 |
| 1 | 0.991352 | 0.93416 | 0.11429 | 0.993520 |
| 2 | 0.991350 | 0.93310 | 0.11055 | 0.993514 |

This is not yet a passing first-stage encoder.  It shows that the residual
compiler can reduce soft distribution error without collapsing, but the
objective still under-protects teacher top128 membership.  The next M546
variant adds an active-rank teacher-fit term: teacher top128 active dimensions
must outrank hard negatives sampled from teacher top512.  This remains a pure
document-only dense-output compiler experiment, with no queries, qrels, BM25,
route positives, or ranking labels.

The first active-rank follow-up using top512-internal negatives also regressed
active recall at epoch1 (`0.93802 -> 0.93476`) and was stopped.  The active
competition for Recall@top128 is global, not limited to teacher top512, so the
next active M546 run changed the negative pool to global non-active dimensions
and enabled active-floor checkpoint selection.

That global-negative active-rank run also failed the first-stage gate.  It
improved support cosine and soft KL, but exact active recall still fell below
the epoch0 identity floor:

| Run | Epoch | Support Cos | Active Recall | Soft KL | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| M546 global-rank broad4 cap20k | 0 | 0.991453 | 0.93826 | 0.12850 | identity floor |
| M546 global-rank broad4 cap20k | 1 | 0.991574 | 0.93503 | 0.10984 | stopped |

The useful diagnosis is that pairwise active-rank averaging is still too
diffuse for the hard top128 boundary.  M546 then added a direct
`active_boundary` loss: top global non-active predicted magnitudes are
penalized if they exceed the weakest teacher-active predicted magnitude.  That
also failed as a full-epoch update:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Decision |
| --- | --- | ---: | ---: | ---: | --- |
| M546 boundary broad4 cap20k | epoch0 | 0.991379 | 0.938867 | 0.13271 | identity floor |
| M546 boundary broad4 cap20k | epoch1 | 0.991518 | 0.936757 | 0.11452 | stopped |

The next correction was step-level gating.  This exposed a very narrow safe
window where a small residual compiler improves soft teacher fit without
breaking exact active membership:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Decision |
| --- | --- | ---: | ---: | ---: | --- |
| M546 stepgate broad4 cap20k lr5e-6 | epoch0 | 0.991125 | 0.937670 | 0.13304 | identity floor |
| M546 stepgate broad4 cap20k lr5e-6 | step1000 | 0.991150 | 0.937721 | 0.13143 | passes floor |
| M546 stepgate broad4 cap20k lr5e-6 | step2000 | 0.991165 | 0.937708 | 0.13008 | passes floor |
| M546 stepgate broad4 cap20k lr5e-6 | step3000 | 0.991177 | 0.937675 | 0.12893 | selected |
| M546 stepgate broad4 cap20k lr5e-6 | step4000 | 0.991186 | 0.937622 | 0.12796 | active below floor |
| M546 stepgate broad4 cap20k lr5e-6 | step6000 | 0.991200 | 0.937463 | 0.12643 | active below floor |

This is the first strict-gate positive M546 result, but it is not broad
enough to claim the first-stage encoder is stable.  The accepted improvement
over epoch0 active recall is only `0.00000477` absolute; the real gain is soft
KL, from `0.13304` to `0.12893`.

The wider available-task check did not pass at the same learning rate:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Decision |
| --- | --- | ---: | ---: | ---: | --- |
| M546 broad10 cap10k lr5e-6 | epoch0 | 0.986564 | 0.932283 | 0.18390 | identity floor |
| M546 broad10 cap10k lr5e-6 | step1000 | 0.986577 | 0.932279 | 0.18316 | stopped |

The current active follow-ups reduce update amplitude on broad10:

```text
spark-1: broad10_available_cap10k_delta_stepgate_lr2e6_seed5468
spark-2: broad10_available_cap10k_delta_stepgate_lr1e6_seed5469
```

Both stopped at step1000 and selected epoch0:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Decision |
| --- | --- | ---: | ---: | ---: | --- |
| M546 broad10 cap10k lr2e-6 | epoch0 | 0.987106 | 0.93305054 | 0.18212 | identity floor |
| M546 broad10 cap10k lr2e-6 | step1000 | 0.987111 | 0.93304977 | 0.18181 | stopped |
| M546 broad10 cap10k lr1e-6 | epoch0 | 0.986591 | 0.93219147 | 0.18356 | identity floor |
| M546 broad10 cap10k lr1e-6 | step1000 | 0.986594 | 0.93218918 | 0.18339 | stopped |

The failures are tiny but exact.  The sub-1000-step residual gate also failed:
even step250 moved broad10 active recall below the exact epoch0 floor.  This
ends the unconstrained residual output-compiler branch for the first stage.

The successful M546 branch is stricter and more structural: a monotonic output
compiler learns only a positive global magnitude exponent over the frozen dense
coordinates.  It cannot change coordinate ordering, so exact active membership
and top-k support retention are preserved by construction.

Broad10 monotonic KL-first result:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | top256/T128 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| M546 monotonic broad10 seed5475 | epoch0 | 1.0000 | 0.986332 | 0.931186 | 0.18613 | 0.986314 | identity floor |
| M546 monotonic broad10 seed5475 | step250 | 1.0833 | 0.984819 | 0.931186 | 0.13957 | 0.986314 | selected |
| M546 monotonic broad10 seed5475 | step500 | 1.0835 | 0.984816 | 0.931186 | 0.13958 | 0.986314 | held |
| M546 monotonic broad10 seed5475 | step750 | 1.0785 | 0.984950 | 0.931186 | 0.13966 | 0.986314 | held |
| M546 monotonic broad10 seed5475 | step1000 | 1.0837 | 0.984808 | 0.931186 | 0.13958 | 0.986314 | held |

This gives a stable first-stage stopping point.  It is not evidence that an
arbitrary learned output layer can beat the dense-derived route.  It is
evidence that the smallest useful compiler is support-equivalent value
calibration: preserve the dense-derived posting support exactly, and calibrate
the posting magnitudes before any BM25, qrels, or ranking objective is allowed
back into the system.

Reviewer correction: the earlier `seed5474` monotonic run had the same broad10
signal but failed to write JSON after max steps.  The rerun `seed5475` fixed
the finalization path and produced JSON/Markdown/checkpoint, so the remaining
issue is scientific rather than infrastructure: KL-first accepts support
cosine degradation.  The next formal M546 run must select only checkpoints that
meet all three constraints:

```text
active recall >= epoch0
support cosine >= epoch0
soft support KL <= epoch0
```

That gate tests whether the output compiler can become more teacher-faithful
without changing support geometry.  If it selects epoch0, the correct
conclusion is that monotonic power is a useful diagnostic and calibration
signal, but not yet a final dense-equivalent first-stage encoder.

The multi-constraint run selected epoch0:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | --- |
| M546 multigate broad10 seed5476 | epoch0 | 0.986500 | 0.932273 | 0.18720 | selected |
| M546 multigate broad10 seed5476 | step250 | 0.984921 | 0.932273 | 0.13939 | support fail |
| M546 multigate broad10 seed5476 | step500 | 0.985029 | 0.932273 | 0.13937 | support fail |
| M546 multigate broad10 seed5476 | step750 | 0.985049 | 0.932273 | 0.13940 | support fail |
| M546 multigate broad10 seed5476 | step1000 | 0.984929 | 0.932273 | 0.13938 | support fail |

This confirms the boundary: active/top-k preservation is solved by monotonic
shape, and KL can be compressed by roughly a quarter, but this compression
does not preserve support geometry under exact support-cosine gating.  The
next version should not be another KL-first promotion.  It needs either a
different value-calibration objective that does not reduce support cosine, or
a clearly justified tolerance/bi-objective frontier if exact support cosine is
too strict for the intended posting representation.

The first bounded-geometry continuation tested the tolerance route directly:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | --- |
| M546 boundcos05 w0.5 seed5477 | epoch0 | 0.986048 | 0.930757 | 0.19071 | selected |
| M546 boundcos05 w0.5 seed5477 | step250 | 0.984655 | 0.930757 | 0.14380 | support fail |
| M546 boundcos05 w0.5 seed5477 | step500 | 0.984509 | 0.930757 | 0.14374 | support fail |
| M546 boundcos05 w0.5 seed5477 | step750 | 0.984625 | 0.930757 | 0.14375 | support fail |
| M546 boundcos05 w0.5 seed5477 | step1000 | 0.984665 | 0.930757 | 0.14382 | support fail |

That run used `SUPPORT_FLOOR_TOLERANCE=0.0005` and
`SUPPORT_COSINE_WEIGHT=0.5`.  It still selected epoch0, so the route should be
tightened as a fast frontier search before any BM25 or ranking objective is
added: lower learning rate, denser eval checkpoints, and much stronger
support-cosine pressure.  The active follow-up is
`broad10_available_cap10k_monotonic_power_frontier_w20_eval50_seed5478`
(`EVAL_DOC_ROWS=256`, `EVAL_EVERY_STEPS=50`, `MAX_TRAIN_STEPS=300`,
`LEARNING_RATE=1e-4`, `SUPPORT_COSINE_WEIGHT=20.0`).  This is a search loop,
not a promoted result; any passing checkpoint must be rerun under full broad10
verification.

That fast frontier produced the first bounded positive signal:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | --- |
| M546 frontier w20 seed5478 | epoch0 | 0.987833 | 0.935544 | 0.17811 | floor |
| M546 frontier w20 seed5478 | step50 | 0.987788 | 0.935544 | 0.17288 | pass |
| M546 frontier w20 seed5478 | step100 | 0.987742 | 0.935544 | 0.16829 | pass |
| M546 frontier w20 seed5478 | step150 | 0.987690 | 0.935544 | 0.16390 | pass |
| M546 frontier w20 seed5478 | step200 | 0.987636 | 0.935544 | 0.15998 | pass |
| M546 frontier w20 seed5478 | step250 | 0.987588 | 0.935544 | 0.15696 | pass |
| M546 frontier w20 seed5478 | step300 | 0.987535 | 0.935544 | 0.15395 | selected |

The selected checkpoint keeps active recall flat, stays inside the 0.0005
support-cosine budget, and reduces soft KL by about 13.6% on the fast 256-doc
broad10 surface.  This revives the line, but only as a candidate.  The active
verification run is `M546 frontier w20 full1024 seed5478`, using the same
objective with `EVAL_DOC_ROWS=1024` and `EVAL_EVERY_STEPS=100`.

The full verification also passed:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | --- |
| M546 frontier w20 full1024 seed5478 | epoch0 | 0.987086 | 0.933940 | 0.18349 | floor |
| M546 frontier w20 full1024 seed5478 | step100 | 0.986996 | 0.933940 | 0.17360 | pass |
| M546 frontier w20 full1024 seed5478 | step200 | 0.986890 | 0.933940 | 0.16521 | pass |
| M546 frontier w20 full1024 seed5478 | step300 | 0.986789 | 0.933940 | 0.15913 | selected |

This is now a valid first-stage milestone: no BM25, no qrels, no ranking
objective, frozen dense root, monotonic output compiler only.  Active recall is
unchanged, support cosine drops by only 0.000297 within the declared 0.0005
budget, and soft support KL improves by about 13.3%.  The effect is smaller
than the unconstrained KL-first runs, but it is the first M546 broad10 result
that improves teacher distribution fit without violating the support-geometry
gate.

The early-stop gate is now strict: with active-floor selection enabled, any
active-floor breach stops the run immediately instead of only preventing
checkpoint selection.

Implementation surface:

```text
scripts/research_sae_m546_frozen_output_compiler.py
scripts/run_m546_frozen_output_compiler_spark.sh
docs/research-sae/reports/m0500-m0599/ii42-m546-frozen-output-compiler-plan.md
```

M546 differs from the nearby prior runs:

- M526 allowed both dense and posting heads to train; M546 freezes the dense
  root and dense head by default.
- M540/M541 trained row/product faithfulness; M546 trains the output compiler
  that maps the frozen dense output to the deterministic posting target.
- M544/M545 applied weak support pressure around M541; M546 does not use that
  checkpoint or support-stage objective.

Promotion should be based only on teacher-fit metrics: support cosine, support
MSE, soft support KL, top256 retention of teacher top128, active recall,
active jaccard, sign accuracy, and active magnitude cosine.  Retrieval, BM25,
and ranking calibration are deliberately deferred until this compiler surface
beats its epoch0 raw-dense baseline.

## M536 Route Correction

M536 was the earlier route correction.  It kept the model query-independent and
corpus-only, but changed the target from exact top128 equality to a wider soft
product surface:

```text
document text -> PPLX root + LoRA/head -> dense-derived product
```

The training objective uses:

- full-row dense cosine/MSE;
- soft KL over teacher top512 support;
- support BCE and magnitude preservation over top512;
- rank margins for top512 containment and teacher top128 retention;
- sign preservation over the support.

It still uses no queries, qrels, BM25 features, route positives, or ranking
loss.  The current spark-1 run is:

```text
/home/huoju/leask/runs/ii42-m536-overcomplete-corpus-product-distill-v1/
  broad4_lora_s512_16k_e2/m536_broad4_lora_s512_16k_e2_seed5360.json
```

Config summary:

| Field | Value |
| --- | --- |
| tasks | FiQA2018, ArguAna, SCIDOCS, TRECCOVID |
| max docs per task | 16384 |
| support prefix | 512 |
| teacher active dims | 128 |
| selection metric | top256 recall of teacher top128 |
| epochs | 2 |
| adapter | LoRA, last 2 layers, rank 8 |

Live epoch0 baseline:

| Metric | Value |
| --- | ---: |
| top256 retain teacher top128 | 0.99360 |
| exact active recall | 0.93791 |
| row cosine | 0.99127 |

Epoch1 result:

| Metric | Value |
| --- | ---: |
| top256 retain teacher top128 | 0.99369 |
| exact active recall | 0.92297 |
| row cosine | 0.99063 |
| loss | 0.658329 |

Epoch1 slightly improves overcomplete top256 retention, but it damages exact
top128 support and row cosine.  This is not a clean breakthrough.  The run is
continuing to epoch2 for the final gate.  spark-2 is not used for a parallel
M538 run because its Scale-RAE container is active on the GPU.

The promotion rule is no longer exact top128 active recall.  M536 must improve
heldout top256-retains-teacher-top128 or soft support KL without materially
damaging row cosine.  If it still selects epoch 0, the current post-training
route is unlikely to improve the already strong raw PPLX denseout support; the
next useful work should move to a larger first-stage representation distillation
or to a stage-two learned scorer.

## M538 Prepared Two-Phase Follow-Up

M538 is prepared but not launched while M536 owns the spark-1 GPU.  It tests a
more conservative training hypothesis:

```text
phase 1: dense row preservation only
phase 2: overcomplete product-support distillation
```

The purpose is to isolate whether M536's mixed objective damages dense geometry
too early.  M538 defaults to a stronger LoRA setup (`rank=16`, last 4 layers),
but uses the same corpus-only, query-independent data contract.  It should be
started only if M536 fails to improve over the epoch0 selection baseline or if
M536 improves support while materially degrading cosine.

Prepared spark-1 launch command:

```bash
RUN=/home/huoju/leask/runs/ii42-m538-two-phase-corpus-product-distill-v1/broad4_lora16_s512_16k_p1p2
mkdir -p "$RUN"
tmux new-session -d -s ii42_m538_broad4_lora16_p1p2 \
  "cd /home/huoju/leask/dev/ii42-m327-work && \
  docker run --rm --gpus all --ipc=host \
    --ulimit memlock=-1 --ulimit stack=67108864 \
    -v /home/huoju:/home/huoju \
    -v /home/huoju/leask:/workspace/leask \
    -w /home/huoju/leask/dev/ii42-m327-work \
    -e HF_HOME=/home/huoju/leask/hf_cache \
    -e TRANSFORMERS_CACHE=/home/huoju/leask/hf_cache/hub \
    -e PYTHONPATH=/home/huoju/leask/runs/ii42-m520-support-target-ablation-v1/python-deps:scripts \
    nvcr.io/nvidia/pytorch:26.03-py3 bash -lc '
      python3 scripts/research_sae_m538_two_phase_corpus_product_distill.py \
        --tasks FiQA2018,ArguAna,SCIDOCS,TRECCOVID \
        --output-root $RUN \
        --json-name m538_broad4_lora16_s512_16k_p1p2_seed5380.json \
        --report-name m538_broad4_lora16_s512_16k_p1p2_seed5380.md \
        --fix-mistral-regex --local-files-only \
        --model-revision 2c4d510dd4a732063c31a0f70193e35067b51fd8 \
        --adapter lora --train-last-layers 4 \
        --lora-rank 16 --lora-alpha 32 \
        --max-doc-rows-per-task 16384 --eval-doc-rows 2048 \
        --phase1-epochs 1 --phase2-epochs 2 \
        --train-batch-size 8 --project-batch-size 16 \
        --learning-rate 1e-6 --head-learning-rate 3e-6 \
        --teacher-active-dims 128 --support-prefix 512 \
        --selection-prefix 256 \
        --eval-prefixes 128,160,192,224,256,384,512 \
        --log-every-steps 1000' \
    2>&1 | tee $RUN/m538_broad4_lora16_s512_16k_p1p2_seed5380.log"
```

If this hits OOM, restart with `--train-batch-size 4`; otherwise keep the batch
8 setting to avoid another long silent epoch.

Update after M536 KL-only and M542: do not launch this prepared M538 run as the
next action.  Its core question has been tested more directly by initializing
from the stronger M540 row checkpoint.  M536-from-M540 showed that even weak
support/KL pressure can pull against row preservation, and M542 showed that a
pure row-only continuation on broad4 is already at the local limit.  M538 may
remain useful only if M541 shows that broad10 row pretraining improves the
stage-one surface enough to justify revisiting staged support refinement.

## M546 Current Stage-One Milestone

M546 is now the cleanest version of the user's intended first-stage route:

```text
frozen dense root -> monotonic output compiler -> dense-derived posting surface
```

It excludes BM25, qrels, route positives, and ranking losses.  The useful
shape is not a free residual or LoRA update.  Those repeatedly damaged exact
active support on broad10.  The useful shape is a bounded monotonic power
compiler that preserves coordinate ordering and only calibrates magnitudes.

The current validated setting is:

```text
COMPILER_MODE=monotonic_power
SELECTION_MODE=kl_first
ACTIVE_FLOOR_SELECTION=1
SUPPORT_FLOOR_SELECTION=1
SUPPORT_FLOOR_TOLERANCE=0.0005
KL_IMPROVEMENT_SELECTION=1
SUPPORT_COSINE_WEIGHT=20.0
SOFT_SUPPORT_KL_WEIGHT=1.0
MAX_TRAIN_STEPS=300
EVAL_EVERY_STEPS=100
```

Three full1024 broad10 seeds all selected step300:

| Metric | Mean | Std | Min | Max |
| --- | ---: | ---: | ---: | ---: |
| gamma | 1.025625 | 0.000351 | 1.025148 | 1.025983 |
| support drop | 0.000295 | 0.000006 | 0.000287 | 0.000300 |
| active drop | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| KL relative improvement | 13.33% | 0.28% | 12.93% | 13.56% |

Interpretation: M546 gives a stable, small, dense-root-preserving compiler
improvement.  It is not yet a search result.  The next required step before
BM25/fusion is M547 dense-only retrieval verification against exact dense.

## M547 Dense-Only Verification

M547 completed the required retrieval-side check before BM25/fusion.  On full
broad10, the M546 gamma mean surface is effectively dense-equivalent:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00000 | 0.00000 |
| `row_int8` | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | 0.00006 | -0.00004 |
| `m546_gamma_mean` | 0.57282 | 0.43531 | 0.74779 | 0.65188 | 0.99414 | 0.00019 | -0.00024 |

This resolves the stage-one question: the monotonic compiler is not merely a
teacher-fit artifact; it also keeps downstream dense-only retrieval behavior
inside a very small tolerance.  It still should not be treated as a meaningful
search-quality improvement over dense.  The next phase should move to
BM25/fusion/ranking experiments while keeping exact dense, row-int8 dense, and
M546 gamma as fixed baselines.

## Artifacts

- `scripts/research_sae_m533_cached_relational_product_adapter.py`
- `scripts/research_sae_m534_tied_dense_product_distill.py`
- `scripts/research_sae_m535_cached_active_margin_diagnostic.py`
- `scripts/research_sae_m536_overcomplete_corpus_product_distill.py`
- `scripts/research_sae_m537_cached_overcomplete_calibration.py`
- `scripts/research_sae_m538_two_phase_corpus_product_distill.py`
- `scripts/research_sae_m539_cached_nonlinear_product_calibration.py`
- `scripts/research_sae_m546_frozen_output_compiler.py`
- `scripts/research_sae_m547_stage1_dense_only_retrieval.py`
- `docs/research-sae/reports/m0500-m0599/ii42-m547-stage1-dense-only-retrieval-report.md`
- `outputs/m533/cached_relational_product_adapter/`
- `outputs/m534/tied_dense_product_distill/`
- `outputs/m535/cached_active_margin/`
- `outputs/m537/cached_overcomplete_calibration/`
- `outputs/m539/cached_nonlinear_product_calibration/`
- `outputs/m546/frozen_output_compiler/`
- `outputs/m547/stage1_dense_only_retrieval/`
