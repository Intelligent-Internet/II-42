# ii42 M190 Execution Status Report

## Snapshot

Date: 2026-06-13

M190 is currently in the strict M150 replay line. The clean Stage-A checkpoint is:

```text
/home/huoju/leask/runs/ii42-m190-m150a1-replay-stagea-v1/bm25sae_stagea_best.pt
```

The Stage-A run finished 280,491 steps, but the handoff checkpoint is the best checkpoint at step 110,000, not latest:

| Field | Value |
| --- | --- |
| total examples | 107,708,436 |
| best step | 110,000 |
| best examples | 42,239,964 |
| best neighbor overlap | 0.946875 |
| latest checkpoint | not promoted |

## Current Gate State

The M190 Stage-B fusionnorm official full-corpus gate is running here:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1-official-full-corpus-gate
```

Completed datasets:

```text
arguana
cqadupstack
fiqa
nfcorpus
quora
scidocs
scifact
trec-covid
webis-touche2020
```

Completed through current gate:

```text
arguana
cqadupstack
fiqa
nfcorpus
nq
quora
scidocs
scifact
trec-covid
webis-touche2020
```

The original serial NQ job was CPU/IO-bound in streaming SAE search. It ran for more than 6 hours and remained at `sae search docs=1000000` with no final eval output. It was stopped before producing any final eval JSON.

NQ is now running through the query-sharded evaluator with the same checkpoint, corpus, BM25 cache, active-k settings, and checkpoint fusion weights:

| Field | Value |
| --- | --- |
| session | `ii42_m190_stageb_nq_sharded_gate` |
| shards | 4 |
| max parallel | 4 |
| SAE weight | 1.1647638082504272 |
| BM25 weight | 0.8687121868133545 |

This changed only the execution path, not the evaluation contract. All four shards completed and were collected into the final NQ eval payload.

NQ result:

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.6986 | 0.2263 | 0.2552 | 0.2140 |
| Dense | 0.9613 | 0.5550 | 0.6042 | 0.5341 |
| BM25+Dense score | 0.9550 | 0.4719 | 0.5234 | 0.4506 |
| SAE-only | 0.9100 | 0.4739 | 0.5180 | 0.4540 |
| BM25+SAE score | 0.9260 | 0.4559 | 0.5035 | 0.4353 |
| BM25+SAE RRF | 0.9252 | 0.3793 | 0.4202 | 0.3602 |

## Official Gate Aggregates

The Stage-A reference rows below are simple unweighted means over the 9 completed representative official datasets. The Stage-B rows include NQ and are simple unweighted means over the 10 completed representative official datasets.

### M190A Stage-A Best 9-Dataset Reference

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5622 | 0.4893 | 0.4193 | 0.2848 |
| Dense | 0.6722 | 0.5994 | 0.5355 | 0.3747 |
| BM25+Dense score | 0.6712 | 0.5935 | 0.5228 | 0.3629 |
| BM25+SAE score | 0.6490 | 0.5778 | 0.5052 | 0.3510 |
| BM25+SAE RRF | 0.6487 | 0.5555 | 0.4887 | 0.3336 |

### M190 Stage-B Fusionnorm 10-Dataset Gate

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5761 | 0.4631 | 0.4030 | 0.2778 |
| Dense | 0.7015 | 0.5954 | 0.5426 | 0.3907 |
| BM25+Dense score | 0.7010 | 0.5855 | 0.5298 | 0.3763 |
| SAE-only | 0.6699 | 0.5505 | 0.4969 | 0.3584 |
| BM25+SAE score | 0.6826 | 0.5763 | 0.5168 | 0.3715 |
| BM25+SAE RRF | 0.6810 | 0.5438 | 0.4863 | 0.3430 |

## Benefit Check

Stage-B was positive versus Stage-A on the first 9 completed datasets:

| Metric | Stage-B minus Stage-A |
| --- | ---: |
| Recall@100 | +0.0065 |
| MRR@20 | +0.0119 |
| NDCG@10 | +0.0131 |
| MAP@100 | +0.0134 |

After NQ, the conclusion is unchanged: Stage-B is directionally useful and should not be discarded. However, it still trails dense on all four ranking metrics, and NQ makes the ranking gap more visible.

## Current Interpretation

M190A Stage-A is the clean representation baseline. It should be treated as the current frozen base unless a retrieval-selected checkpoint beats it on a comparable official gate.

M190 Stage-B fusionnorm is a useful but insufficient transfer from representation to retrieval. The gap that remains is not solved by adding Stage-A pairwise rank loss; M201 already showed that direct pairwise ranking inside Stage A quickly regresses the candidate-eval surface.

The NQ result is consistent with the 9-dataset trend: BM25+SAE has strong recall, but the top-rank metrics remain below dense. The next step is not another Stage-A restart. It is a new Stage-B/C objective that directly targets final BM25+SAE admission and ranking.

## Next Ordered Actions

1. Start a new Stage-B/C line from M190A best.
2. Use full-corpus hard-negative/admission rows, not narrow candidate-only rows.
3. The row surface must include BM25 topK, SAE topK, dense/BM25+dense control hits, qrel positives, BM25 false positives, and SAE candidates that are lost by fusion scoring.
4. Optimize final BM25+SAE top-100 admission and top-rank ordering directly.
5. Do not restart Stage A unless this new Stage-B/C evidence shows the M190A representation itself is the bottleneck.

## M210 Canary Launch

M210 is the first ordered follow-up after the 10-dataset M190 gate.

Run:

```text
ii42-m210-m190a-fullsurface-stageb-canary-v1
```

Session:

```text
ii42_m210_m190a_fullsurface_stageb_canary_v1
```

Log:

```text
/home/huoju/leask/logs/ii42_m210_m190a_fullsurface_stageb_canary_v1.log
```

Key constraints:

| Field | Value |
| --- | --- |
| init checkpoint | M190A best |
| materialized root | `/home/huoju/leask/runs/ii42-m190-replay-actual-beir15-pplx1024` |
| base train roots | none |
| base eval roots | none |
| candidate_k | 192 |
| dense_top_k / bm25_top_k / sae_top_k / source_top_k | 500 / 500 / 500 / 500 |
| train steps | 1500 |
| ClearML project | `ii42_sae/M210` |

Train row specs:

```text
fiqa:train:300:900
nfcorpus:train:300:900
scifact:train:0:0
dbpedia-entity:dev:300:900
fever:train:300:900
hotpotqa:train:300:900
msmarco:train:300:900
```

Validation row specs:

```text
fiqa:dev:120:360
nfcorpus:dev:120:360
quora:dev:120:360
msmarco:dev:120:360
hotpotqa:dev:120:360
fever:dev:120:360
```

Initial status:

- preflight passed;
- checkpoint dimension is 1024 and feature count is 16384;
- no legacy base rows are included;
- fresh train rows completed for `fiqa`, `nfcorpus`, and `scifact`;
- current build step is `dbpedia-entity:dev`.

### M210 Training And Official Gate Status

M210 completed row construction and Stage-B training. The best checkpoint is:

```text
/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1/bm25sae_stageb_best.pt
```

Candidate-surface best event:

| Split | Model | Hit@20 | MRR@20 |
| --- | --- | ---: | ---: |
| validation | Dense | 0.8083 | 0.5973 |
| validation | SAE-only | 0.7431 | 0.5395 |
| validation | BM25+SAE | 0.7597 | 0.5437 |
| train | Dense | 0.8354 | 0.6135 |
| train | SAE-only | 0.8497 | 0.6634 |
| train | BM25+SAE | 0.8586 | 0.6664 |

Interpretation: the training surface has a real signal, but validation is still
below dense. M210 therefore requires official full-corpus evidence before any
promotion decision.

The initial post-training eval failed because the runner pointed at the old
continuity root:

```text
/home/huoju/leask/runs/m130-pplx-all-data-eval-corpus
```

That root did not contain `quality_qrels.json`. This was an evaluation path
configuration issue, not a checkpoint failure.

The follow-up official gate is now:

```text
ii42-m210-official-beir15-streaming-gate-v1
```

It uses per-dataset official BEIR qrels and the M210 best checkpoint. The gate
script now preflights reused BM25 ranking caches; if a cache does not cover all
queries for the current official dataset, it is discarded and BM25 is recomputed
for that dataset. This fixed the `arguana` cache mismatch, where the reused
cache was missing 5 query IDs.

Current official gate progress:

| Dataset | Status |
| --- | --- |
| `nfcorpus` | complete |
| `scifact` | complete |
| `arguana` | complete |
| `scidocs` | complete |
| `fiqa` | complete |
| `trec-covid` | complete |
| `webis-touche2020` | complete |
| `cqadupstack` | running |

Seven-dataset interim aggregate:

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5116 | 0.4798 | 0.3902 | 0.2270 |
| Dense | 0.6102 | 0.5819 | 0.4976 | 0.3012 |
| BM25+Dense score | 0.6179 | 0.5875 | 0.5051 | 0.3056 |
| SAE-only | 0.5715 | 0.5130 | 0.4300 | 0.2527 |
| BM25+SAE score | 0.5950 | 0.5196 | 0.4417 | 0.2620 |
| BM25+SAE RRF | 0.5911 | 0.5420 | 0.4531 | 0.2670 |

Interim interpretation: M210 improves over BM25 and has useful recall signal,
but it does not yet close the dense/BM25+dense ranking gap on the completed
official datasets.

The gate was stopped after the `diffsae-codex` review. The useful new evidence
is not the reranker route, but the non-reranker training shape:

- protect the head score/order surface instead of only optimizing broad
  candidate admission;
- suppress high-rank BM25/SAE non-qrel background candidates with a margin;
- keep dense-head candidates protected so the false-positive loss does not fight
  the semantic teacher;
- continue to select by official full-corpus evidence, not candidate-bank
  metrics alone.

## M210 Score-Head / Background-Margin Canary

New runner:

```text
scripts/run_ii42_m210_score_head_bgmargin_spark.sh
```

Run:

```text
ii42-m210-scorehead-bgmargin-v1
```

This run reuses the validated M210 row surface so the only changed variable is
the Stage-B objective. It does not use a reranker teacher.

Added training losses:

| Loss | Purpose |
| --- | --- |
| `loss_head_score_distill` | Preserve the dense/BM25+dense head score surface. |
| `loss_background_false_positive_margin` | Push qrel positives above BM25/SAE high-rank non-qrel background candidates. |

Default canary settings:

| Field | Value |
| --- | --- |
| train rows | `/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1-candidate-rows` |
| eval rows | `/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1-eval-candidate-rows` |
| steps | `2000` |
| head source / topK | `dense` / `20` |
| background source / topK | `bm25_or_sae` / `100` |
| reranker teacher | disabled |

Promotion rule: first compare candidate validation against the previous M210
canary. Only if validation improves without obvious SAE collapse should this
checkpoint receive a new official BEIR full-corpus gate.

Result:

| Run | Best step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | Delta vs previous M210 |
| --- | ---: | ---: | ---: | --- |
| previous M210 canary | best | 0.7597 | 0.5437 | baseline |
| `ii42-m210-scorehead-bgmargin-v1` | 1000 | 0.7556 | 0.5452 | Hit@20 -0.0041, MRR@20 +0.0015 |
| `ii42-m210-scorehead-bgmargin-v1` | 2000 | 0.7444 | 0.5421 | degraded late checkpoint |

Interpretation: the non-reranker `diffsae-codex`-inspired losses are useful
for head ranking, but this first weighting is too admission-negative. Training
continued to improve the train surface while validation Hit@20 declined after
step 1000. This checkpoint should not receive an official full-corpus gate.

Operational note: the shared Stage-B runner now supports
`RUN_CONTINUITY_EVAL=0` and skips continuity eval if the configured qrels are
missing. The M210 score-head/background-margin wrapper disables continuity eval
by default because its promotion decision is candidate validation first; official
full-corpus gates should be launched explicitly after a canary passes.

## M210 Coverage / Ranking-First Canary

New runner:

```text
scripts/run_ii42_m210_coverage_ranking_spark.sh
```

Run:

```text
ii42-m210-coverage-ranking-v1
```

This canary tested the simplified objective discussed after the score-head run:
optimize final coverage and ranking directly instead of adding a reranker or
dataset-specific profile. It reused the same validated M210 row surface and the
same M190A checkpoint.

Added training losses:

| Loss | Purpose |
| --- | --- |
| `loss_coverage_boundary` | Push the best qrel positive above the topK non-qrel boundary. |
| `loss_source_ranking_pairwise` | Rank qrel positives above hard negatives from BM25, dense, SAE, and BM25+SAE source heads. |

Default canary settings:

| Field | Value |
| --- | --- |
| train rows | `/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1-candidate-rows` |
| eval rows | `/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1-eval-candidate-rows` |
| steps | `1500` |
| coverage boundary K / margin | `20` / `0.04` |
| source ranking negatives | `source_union`, top `100`, margin `0.04` |
| reranker teacher | disabled |

Result:

| Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| previous M210 canary best | 0.7597 | 0.5437 | n/a | n/a |
| 1 | 0.7500 | 0.5413 | 0.7375 | 0.5370 |
| 250 | 0.7528 | 0.5441 | 0.7375 | 0.5393 |
| 500 | 0.7569 | 0.5437 | 0.7403 | 0.5407 |
| 750 | 0.7556 | 0.5422 | 0.7417 | 0.5390 |
| 1000 | 0.7569 | 0.5432 | 0.7458 | 0.5395 |
| 1250 | 0.7542 | 0.5434 | 0.7431 | 0.5406 |

Decision: stopped before the final checkpoint. The loss improved SAE-only
coverage slightly, but it did not beat the previous M210 BM25+SAE Hit@20 and it
did not produce a stable ranking gain. This means the simplified coverage /
ranking-first abstraction is directionally reasonable but not sufficient in
this form. It should be parked unless the next attempt changes the row surface
or target definition, not just the same weights.

## M210 Soft TopK Utility Canary

New runner:

```text
scripts/run_ii42_m210_soft_topk_utility_spark.sh
```

Run:

```text
ii42-m210-soft-topk-utility-v1
```

This canary replaced the hard admission boundary with a soft rank objective that
directly approximates final topK Recall/NDCG utility over BM25+SAE scores.

Added training loss:

| Loss | Purpose |
| --- | --- |
| `loss_soft_topk_utility` | Optimize qrel-positive admission and top-rank discount with differentiable soft ranks. |

Default canary settings:

| Field | Value |
| --- | --- |
| train rows | `/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1-candidate-rows` |
| eval rows | `/home/huoju/leask/runs/ii42-m210-m190a-fullsurface-stageb-canary-v1-eval-candidate-rows` |
| steps | `1500` |
| soft topK K | `20` |
| soft NDCG / Recall weights | `0.75` / `0.25` |
| reranker teacher | disabled |

Result:

| Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| previous M210 canary best | 0.7597 | 0.5437 | n/a | n/a |
| 1 | 0.7500 | 0.5413 | 0.7375 | 0.5370 |
| 250 | 0.7514 | 0.5426 | 0.7333 | 0.5386 |
| 500 | 0.7583 | 0.5429 | 0.7375 | 0.5399 |
| 750 | 0.7528 | 0.5402 | 0.7417 | 0.5376 |

Decision: stopped after step 750. The differentiable topK utility briefly
approached the previous M210 Hit@20 but did not beat it, did not improve MRR,
and then regressed. This is stronger evidence that the next improvement is not
another local topK surrogate over the same candidate bank. The row surface must
better expose true full-corpus admission failures, especially BM25-only
relevant hits, dense-only relevant hits, SAE missed positives, and high-BM25
false positives under the same official-eval scoring contract.

## M211 Official-Error-Surface Canary

New runner:

```text
scripts/run_ii42_m211_official_error_surface_spark.sh
```

M211 changed the training surface instead of only changing the loss. It rebuilds
Stage-B rows from completed official full-corpus ranking outputs:

```text
/home/huoju/leask/runs/ii42-m210-official-beir15-streaming-gate-v1/all-test
```

This is a diagnostic canary, not a promotion gate. The rows are derived from
completed official test-ranking outputs and are split by dataset into a train
group and heldout group only to test whether an official-error-shaped training
surface transfers better than the old local candidate bank.

Initial setup:

| Field | Value |
| --- | --- |
| stage-A checkpoint | `/home/huoju/leask/runs/ii42-m190-m150a1-replay-stagea-v1/bm25sae_stagea_best.pt` |
| checkpoint dimension / features | `1024` / `16384` |
| first candidate K | `192` |
| corrected candidate K | `80` |
| train datasets | `fiqa,nfcorpus,scifact,trec-covid,webis-touche2020` |
| heldout dataset | `scidocs` |
| ClearML project | `ii42_sae/M211` |

The first `candidate_k=192` attempt failed preflight because the official
rankings only contain top-100 candidates. The canary was corrected to
`candidate_k=80`, then both train and heldout rows passed preflight with
dimension `1024` and feature count `16384`.

Training rows:

| Source dataset | Rows |
| --- | ---: |
| `fiqa` | 648 |
| `nfcorpus` | 323 |
| `scifact` | 300 |
| `trec-covid` | 50 |
| `webis-touche2020` | 49 |
| total | 1370 |

Training row categories:

| Category | Rows |
| --- | ---: |
| `bm25_sae_hit` | 1140 |
| `bm25_sae_rank_regression` | 94 |
| `high_bm25_false_positive` | 64 |
| `dense_only_representation_gap` | 40 |
| `not_retrieved_by_controls` | 12 |
| `sae_positive_lost_by_fusion` | 11 |
| `bm25_positive_lost_by_fusion` | 9 |

Heldout rows:

| Source dataset | Rows | Notes |
| --- | ---: | --- |
| `scidocs` | 1000 | Passed preflight. |
| `arguana` | n/a | Parked because ranking candidates reference document IDs missing from the available `documents.jsonl` embedding file. |

Heldout `scidocs` baseline from the row surface:

| Model | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| BM25 | 0.5700 | 0.2757 |
| dense | 0.7350 | 0.3808 |
| initial SAE-only | 0.7030 | 0.3613 |
| initial BM25+SAE | 0.7060 | 0.3641 |

### M211 v1: Official Error + Admission-Heavy Loss

Run:

```text
ii42-m211-official-error-surface-v1
```

Result:

| Checkpoint | Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| first | 1 | 0.7060 | 0.3641 | 0.7030 | 0.3613 |
| best Hit@20 | 1000 | 0.7150 | 0.3594 | 0.7010 | 0.3565 |
| best MRR@20 | 250 | 0.7060 | 0.3643 | 0.7000 | 0.3609 |
| final | 1500 | 0.7070 | 0.3568 | 0.7020 | 0.3549 |

Interpretation: the official-error surface did find an admission signal:
BM25+SAE Hit@20 improved from `0.7060` to `0.7150` on heldout `scidocs`.
However, MRR degraded immediately after the first few checkpoints. This means
the current objective can move relevant documents into top-20 but does not
preserve the top-rank order.

### M211 v2: Rank-Preserving Loss Weights

Run:

```text
ii42-m211-official-error-surface-rankpreserve-v1
```

This run reused the same rows and changed only the optimization weights:
lower admission / soft-topK pressure, stronger dense head-score distillation,
source ranking pairwise loss enabled, lower LR, and MRR-weighted model
selection.

Result:

| Checkpoint | Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| first | 1 | 0.7070 | 0.3639 | 0.7030 | 0.3612 |
| best Hit@20 | 1 | 0.7070 | 0.3639 | 0.7030 | 0.3612 |
| best MRR@20 | 1000 | 0.7040 | 0.3649 | 0.6960 | 0.3624 |

Interpretation: rank-preserving weights recovered a very small MRR gain
(`+0.0010` over the v2 first checkpoint) but lost admission quality. This does
not justify an official full-corpus gate.

Decision: M211 validates the problem diagnosis more than it validates a
checkpoint. The useful signal is that official-error rows can improve
admission, while the failed signal is that our current training objective still
trades off admission against top-rank ordering. The next attempt should not run
another broad loss sweep. It should either:

1. add an explicit scorer/fusion calibration model trained on the same
   official-error rows, or
2. rebuild rows with a deeper official-error target that includes
   source-specific top-rank labels, not only candidate admission.

Do not promote M211 v1 or v2 to an official BEIR gate.

## M212 Loss Separation Canaries

M212 tested whether the M211 conflict was caused by loss mixing. The runner and
trainer now support:

| Control | Purpose |
| --- | --- |
| `--freeze-retriever` | Freeze the SAE encoder and train only BM25/SAE fusion scales. |
| `--*-row-categories` | Apply selected losses only to specified `row_category` values. |

All M212 runs reused the same M211 official-error rows:

```text
/home/huoju/leask/runs/ii42-m211-official-error-surface-v1-train-rows-k80
/home/huoju/leask/runs/ii42-m211-official-error-surface-v1-eval-rows-k80
```

The heldout surface was still `scidocs`, so the results are comparable with
M211. These runs test mechanism only; they are not promotion gates.

### M212 v1: Calibration Only

Run:

```text
ii42-m212-calibration-only-v1
```

This froze all SAE parameters and trained only fusion scales.

| Checkpoint | Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| first | 1 | 0.7070 | 0.3638 | 0.7030 | 0.3611 |
| best Hit@20 | 250 | 0.7080 | 0.3635 | 0.7030 | 0.3611 |
| best MRR@20 | 1 | 0.7070 | 0.3638 | 0.7030 | 0.3611 |
| final | 750 | 0.7080 | 0.3634 | 0.7030 | 0.3611 |

Interpretation: pure scale calibration can move Hit@20 by about `+0.001`, but
it does not improve MRR. Fusion scale alone is not the main blocker.

### M212 v2: Category-Masked Mixed Loss

Run:

```text
ii42-m212-category-masked-v1
```

Masking policy:

| Loss group | Row categories |
| --- | --- |
| admission / coverage / soft topK | `dense_only_representation_gap`, `sae_positive_lost_by_fusion`, `bm25_positive_lost_by_fusion`, `not_retrieved_by_controls` |
| source ranking / head distillation | `bm25_sae_rank_regression`, `bm25_sae_hit` |
| background demotion | `high_bm25_false_positive` |

Result:

| Checkpoint | Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| first | 1 | 0.7070 | 0.3639 | 0.7030 | 0.3612 |
| best MRR@20 | 250 | 0.7050 | 0.3641 | 0.7020 | 0.3611 |
| final | 1000 | 0.7060 | 0.3608 | 0.6980 | 0.3578 |

Interpretation: category masking slightly improved MRR at step 250 but reduced
Hit@20, then degraded. The conflict is not fixed by simply assigning existing
losses to row categories.

### M212 v3: Rank-Only

Run:

```text
ii42-m212-rankonly-v1
```

This disabled admission and focused on rank regression / false-positive
demotion.

| Checkpoint | Step | BM25+SAE Hit@20 | BM25+SAE MRR@20 | SAE-only Hit@20 | SAE-only MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| first | 1 | 0.7070 | 0.3639 | 0.7030 | 0.3612 |
| step 250 | 250 | 0.7050 | 0.3609 | 0.7020 | 0.3581 |
| step 500 | 500 | 0.7030 | 0.3593 | 0.6990 | 0.3571 |
| stopped | 750 | 0.7040 | 0.3605 | 0.7000 | 0.3573 |

Interpretation: rank-only training makes the heldout ranking worse. This
rejects the hypothesis that M211 only needs more direct rank-regression loss.

Decision: M212 rules out three cheap fixes: scale-only calibration,
category-masked reuse of existing losses, and rank-only loss. The remaining
plausible path is a different scoring target, not another weight sweep:

1. train a small learned fusion/scorer head over source features
   (`bm25`, `sae`, `dense-teacher rank/score`, source ranks, and category-like
   diagnostics) while keeping SAE fixed, then
2. if the scorer can jointly improve Hit@20 and MRR@20 on heldout official-error
   rows, distill that scorer back into the native BM25+SAE scoring model.

Do not run additional M211/M212 scalar-loss sweeps unless the row target or
scoring model changes.

### M212 v4: Source-Feature Scorer Head

M212 v4 tested the remaining cheap hypothesis: keep the SAE representation
frozen and train a tiny candidate-level scorer over source features only. The
first naive scorer was invalid because the row surface intentionally injects
qrel-positive documents before source candidates. Those injected positives often
have all source ranks and scores equal to zero, so a scorer can learn the qrel
injection artifact. The valid scorer therefore masks labels to candidates
admitted by the selected runtime source.

Native-admitted label coverage on heldout `scidocs`:

| Surface | Positive rows | Positive labels kept |
| --- | ---: | ---: |
| raw qrels in rows | 1000 / 1000 | 4928 |
| native admitted (`bm25`/`sae`/`bm25_sae`) | 899 / 1000 | 2467 |
| dense-control admitted | 936 / 1000 | 2704 |

The valid native scorer uses only runtime-safe BM25/SAE/BM25+SAE score/rank
features. Dense-control is an oracle/control run: it adds dense score/rank
features and expands reachable labels to dense-admitted candidates.

| Run | Feature mode | Best step | Hit@20 | MRR@20 | Notes |
| --- | --- | ---: | ---: | ---: | --- |
| baseline BM25+SAE | native source score | n/a | 0.6910 | 0.3463 | Same native-admitted labels. |
| `ii42-m212-source-scorer-native-admitted-v1` | native | 900 | 0.7570 | 0.3558 | Best admission gain. |
| `ii42-m212-source-scorer-native-teacherdistill-v1` | native | 700 | 0.7490 | 0.3575 | Slightly better MRR, lower admission. |
| `ii42-m212-source-scorer-native-pairwise-v1` | native | 800 | 0.7450 | 0.3574 | Similar to teacher-distill. |
| baseline dense | dense source score | n/a | 0.7350 | 0.3808 | Dense-control admission labels. |
| `ii42-m212-source-scorer-dense-control-v1` | dense-control | 1100 | 0.8420 | 0.4245 | Upper bound, not a product path. |

Interpretation:

1. A small learned scorer can improve native BM25+SAE top-20 admission
   substantially (`+0.066 Hit@20`) while preserving a modest MRR gain
   (`+0.0095 MRR@20`).
2. Dense-control is much stronger, which means the scorer architecture is not
   the blocker. The blocker is missing native source signal for top-rank
   ordering and dense-only representation gaps.
3. The native scorer cannot fix categories whose positives were not admitted by
   native sources. For example, `dense_only_representation_gap` remains zero
   under native masking by construction.

Decision: learned scorer/fusion-head is a viable next component, but only as a
runtime scoring layer over already-admitted BM25/SAE candidates. It should not
be treated as a replacement for better Stage-B admission training. The next
useful step is to connect the native scorer to the official evaluator as a fixed
admission/fusion policy and measure whether row-level gains transfer to
official full-corpus BEIR. If transfer fails, the next training target must add
new native-admissible signals, not another scalar fusion sweep.

### M212 v5: Official-Ranking Transfer Test

Run:

```text
/home/huoju/leask/runs/ii42-m212-source-scorer-transfer-canary-v1
```

This test reused existing official full-corpus rankings from:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1-official-full-corpus-gate/all-test
```

The scorer was applied as a postprocessor over the natural native candidate
union from each query's `bm25`, `sae`, and `bm25_sae_score_fusion` rankings.
This avoids rerunning dense/SAE/BM25 retrieval and directly tests whether the
row-level scorer transfers to the official surface.

Representative results:

| Dataset | BM25+SAE R@100 | Scorer R@100 | BM25+SAE MRR@20 | Scorer MRR@20 | Dense R@100 | Dense MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.9929 | 0.9900 | 0.2892 | 0.1942 | 1.0000 | 0.3033 |
| `fiqa` | 0.7457 | 0.6559 | 0.5311 | 0.4365 | 0.8289 | 0.6017 |
| `nfcorpus` | 0.3208 | 0.3136 | 0.5626 | 0.5658 | 0.3270 | 0.5786 |
| `scidocs` | 0.4487 | 0.4003 | 0.3592 | 0.3223 | 0.4958 | 0.3867 |
| `scifact` | 0.9727 | 0.9719 | 0.7786 | 0.7253 | 0.9633 | 0.7223 |
| `trec-covid` | 0.1371 | 0.1228 | 0.9190 | 0.8819 | 0.1673 | 0.9700 |
| `webis-touche2020` | 0.5740 | 0.4356 | 0.5934 | 0.5986 | 0.4928 | 0.5152 |

Interpretation:

1. The M211/M212 row-trained scorer does not transfer to natural official
   full-corpus rankings. It often degrades both recall and ranking metrics.
2. The row-level gain was not a product-ready scorer signal. It was a useful
   diagnostic that the official-error candidate rows contain learnable local
   patterns, but their candidate construction is too different from natural
   full-corpus admission.
3. The dense-control upper bound is still useful: the scorer architecture can
   learn when the right source signal exists. The failed part is the source
   surface and target, not the idea of a small scoring head.

Decision: stop the direct M211-row-trained scorer path. Do not distill this
checkpoint into runtime. If a scorer/fusion head is pursued, it must be trained
on natural official-ranking rows generated from the same candidate union used at
query time, with a heldout split that measures transfer before any full BEIR
gate. Otherwise, focus should return to Stage-B admission training that creates
better native-admissible candidates.

### M212 v6: Natural Official-Surface Scorer

Run:

```text
/home/huoju/leask/runs/ii42-m212-natural-source-scorer-v1
/home/huoju/leask/runs/ii42-m212-natural-source-scorer-apply-v1
```

This canary rebuilt source-scorer rows directly from official full-corpus
rankings. Unlike M211 rows, it did not inject qrels into the candidate list.
The row contract was the natural runtime candidate union:

```text
bm25 + sae + bm25_sae_score_fusion
```

Rows were split by query hash. This is still a diagnostic, not a promotion
gate, because the same seven datasets were used to create the natural scorer
canary.

Natural row summary:

| Split | Rows | Rows with native positive |
| --- | ---: | ---: |
| train | 3055 | 2582 |
| eval | 716 | 605 |

Heldout hash-split result:

| Model | Hit@20 | MRR@20 |
| --- | ---: | ---: |
| BM25+SAE source score | 0.7891 | 0.3990 |
| dense source score | 0.7947 | 0.4191 |
| natural source scorer | 0.7877 | 0.4612 |

This is the first clean signal that a learned source scorer can improve
top-rank ordering on the natural candidate surface.

However, applying the learned scorer back to full per-dataset official rankings
showed unstable transfer:

| Model | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5121 | 0.4800 | 0.3904 | 0.2272 |
| SAE | 0.5844 | 0.5427 | 0.4581 | 0.2795 |
| BM25+SAE source score | 0.5988 | 0.5762 | 0.4832 | 0.2959 |
| dense | 0.6107 | 0.5826 | 0.4979 | 0.3014 |
| natural source scorer | 0.5441 | 0.5415 | 0.3919 | 0.2680 |

Dataset-level shape:

| Dataset | Result |
| --- | --- |
| `arguana` | Large scorer gain; likely learned style-specific ranking. |
| `scifact` | Scorer beats dense MRR/MAP but loses recall vs BM25+SAE/SAE. |
| `nfcorpus` | Mixed; MRR near BM25+SAE but recall lower. |
| `fiqa`, `scidocs`, `trec-covid`, `webis-touche2020` | Scorer degrades. |

Interpretation:

1. The scorer direction is not dead, but the current fixed source-feature head
   does not generalize.
2. M211-row-trained scorer is the wrong path and should stay closed.
3. Natural-surface scorer is the only scorer path worth keeping, but the next
   version must be explicitly LODO / family-heldout and regularized for
   robustness before any official gate.
4. If that robust scorer cannot beat the source score, the model should stop
   adding scorer heads and return to admission/representation training.

Decision: park direct source-scorer promotion. Continue only one scorer branch:
natural official-surface scorer with strict generalization checks. Do not run
more scorer sweeps on M211 candidate rows.

### M212 v7: LODO Source-Scorer And Residual Blend

Runs:

```text
/home/huoju/leask/runs/ii42-m212-natural-source-scorer-lodo-v1
/home/huoju/leask/runs/ii42-m212-natural-source-scorer-lodo-blend-v1
```

The next check trained one natural source scorer per heldout dataset, excluding
that dataset from scorer training. This directly tested whether the source
head generalizes beyond query-hash split.

LODO scorer result:

| Holdout | BM25+SAE Hit@20 | Scorer Hit@20 | BM25+SAE MRR@20 | Scorer MRR@20 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 0.9251 | 0.9165 | 0.2857 | 0.2910 |
| `fiqa` | 0.6605 | 0.6636 | 0.4813 | 0.4816 |
| `nfcorpus` | 0.7709 | 0.7368 | 0.5577 | 0.4617 |
| `scidocs` | 0.6430 | 0.6470 | 0.3521 | 0.3488 |
| `scifact` | 0.8733 | 0.8800 | 0.7586 | 0.5545 |
| `trec-covid` | 1.0000 | 1.0000 | 0.9267 | 0.9300 |
| `webis-touche2020` | 0.9592 | 0.9592 | 0.6046 | 0.5804 |

This confirms the fixed scorer is not robust. It sometimes improves admission,
but can badly damage MRR on `nfcorpus`, `scifact`, and `webis-touche2020`.

Residual blend then tested whether a small learned-scorer residual can safely
correct the existing BM25+SAE source score. Macro result over the seven current
official datasets:

| Alpha | R@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| ---: | ---: | ---: | ---: | ---: | --- |
| base | 0.5988 | 0.5762 | 0.4832 | 0.2959 | baseline |
| 0.05 | 0.5985 | 0.5762 | 0.4828 | 0.2960 | no clear gain |
| 0.10 | 0.5986 | 0.5759 | 0.4829 | 0.2961 | no clear gain |
| 0.20 | 0.6008 | 0.5746 | 0.4817 | 0.2959 | recall up, ranking down |
| 0.35 | 0.6008 | 0.5707 | 0.4794 | 0.2943 | reject |
| 1.00 | 0.5877 | 0.5105 | 0.4278 | 0.2472 | reject |

Decision: do not use a fixed learned scorer or simple residual blend as final
ranking. The scorer can move candidates, but it is not safe to let it reorder
the top of the list.

### M213: Admission-Only Overlay

Run:

```text
/home/huoju/leask/runs/ii42-m213-admission-overlay-v1
```

The useful part of the scorer is candidate admission, not top-rank scoring.
M213 therefore keeps the first 50 documents from the existing
`bm25_sae_score_fusion` ranking and only fills the tail of top-100 from a
recall-oriented scorer blend:

```text
blend_base = bm25_sae_score_fusion
blend_alpha = 0.20
preserve_base_top_k = 50
```

Official transfer result over the same seven current datasets:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25+SAE source score | 0.5988 | 0.5762 | 0.4832 | 0.2959 |
| M213 admission overlay | 0.6009 | 0.5762 | 0.4832 | 0.2962 |
| Delta | +0.0021 | +0.0000 | +0.0000 | +0.0003 |

Dataset-level recall deltas:

| Dataset | R@100 delta | MAP@100 delta |
| --- | ---: | ---: |
| `arguana` | +0.0000 | -0.0000 |
| `fiqa` | +0.0035 | +0.0002 |
| `nfcorpus` | -0.0012 | -0.0002 |
| `scidocs` | +0.0001 | -0.0000 |
| `scifact` | +0.0067 | +0.0001 |
| `trec-covid` | +0.0024 | +0.0016 |
| `webis-touche2020` | +0.0033 | +0.0006 |

Interpretation:

1. The safe transferable signal is admission tail expansion, not replacement
   scoring.
2. Preserving top-50 prevents the scorer from hurting MRR/NDCG.
3. The gain is small but clean enough to keep as a runtime candidate policy.
4. `nfcorpus` still has a small recall regression, so this is not a final
   promotion. It is the current best M212/M213 direction for further iteration.

Decision: keep M213 admission-only overlay as the active scorer/fusion branch.
Stop fixed scorer replacement and learned alpha gate until better runtime-safe
features exist. Next useful work should test whether this admission overlay can
be integrated with the official full BEIR gate and whether a similarly
rank-preserving policy helps the larger datasets.

### M214/M215: 10-Dataset Safety Check

The seven-dataset M213 result was too optimistic. Extending the same official
surface to ten datasets added `cqadupstack`, `quora`, and `nq`, and exposed a
clear safety problem: residual scorer signals help many datasets but can still
damage `webis-touche2020`.

10-dataset base:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Dense | 0.7015 | 0.5954 | 0.5426 | 0.3907 |
| BM25+SAE source score | 0.6826 | 0.5763 | 0.5168 | 0.3715 |

Direct residual blend at `alpha=0.20`:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25+SAE source score | 0.6826 | 0.5763 | 0.5168 | 0.3715 |
| Residual blend | 0.6830 | 0.5778 | 0.5175 | 0.3726 |
| Delta | +0.0004 | +0.0015 | +0.0007 | +0.0011 |

The macro gain is real, but the per-dataset behavior is not safe. In particular,
`webis-touche2020` loses recall and top-rank quality. Therefore direct residual
blend is still not promotable.

M214 then trained a binary residual gate, choosing between base and
`alpha=0.20`. The best macro candidate used `positive_margin=0.002` and
`threshold=0.50`:

| Model | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25+SAE source score | 0.6826 | 0.5763 | 0.5168 | 0.3715 |
| M214 binary gate | 0.6839 | 0.5783 | 0.5185 | 0.3731 |
| Delta | +0.0014 | +0.0020 | +0.0017 | +0.0016 |

Per-dataset deltas for this best macro setting:

| Dataset | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.0000 | +0.0001 | +0.0003 | +0.0001 |
| `cqadupstack` | +0.0021 | +0.0009 | +0.0011 | +0.0011 |
| `fiqa` | +0.0078 | +0.0065 | +0.0049 | +0.0061 |
| `nfcorpus` | +0.0014 | -0.0017 | +0.0003 | +0.0002 |
| `nq` | -0.0006 | +0.0071 | +0.0068 | +0.0064 |
| `quora` | -0.0001 | +0.0007 | +0.0009 | +0.0011 |
| `scidocs` | +0.0039 | +0.0029 | +0.0019 | +0.0011 |
| `scifact` | +0.0067 | +0.0042 | +0.0051 | +0.0042 |
| `trec-covid` | +0.0003 | +0.0010 | +0.0023 | +0.0011 |
| `webis-touche2020` | -0.0076 | -0.0022 | -0.0072 | -0.0055 |

This confirms that query-level gating has signal, but current runtime-safe
features do not reliably predict harmful `webis`-style cases. Adding hashed
query text reduced the number of selected queries, but it also removed most of
the useful recall/NDCG gain and still left `webis-touche2020` negative.

M215 tested two no-harm variants:

1. A no-harm binary gate with gain/harm weighted loss and dataset-balanced
   examples.
2. A tail-admission policy that preserves the existing top ranks and only
   admits scorer candidates into the bottom of top-100.

The no-harm binary gate over-constrained the model. The safe threshold avoided
some harm, but macro Recall/NDCG became flat or negative, so it is not a better
gate.

Tail-admission result:

| Policy | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: | ---: |
| preserve top-80 | +0.0013 | +0.0000 | +0.0000 | +0.0000 |
| preserve top-90 | +0.0011 | +0.0000 | +0.0000 | +0.0000 |
| preserve top-95 | +0.0009 | +0.0000 | +0.0000 | -0.0000 |
| preserve top-98 | +0.0005 | +0.0000 | +0.0000 | +0.0000 |
| preserve top-99 | -0.0001 | +0.0000 | +0.0000 | -0.0000 |

Even this conservative tail policy still slightly hurts
`webis-touche2020` recall. At preserve top-98, the macro recall gain is only
`+0.0005` and `webis-touche2020` is still `-0.0015`.

Decision:

1. M214 proves that residual scorer/admission has a real signal.
2. Current query-level gate features are not sufficient for no-collapse final
   top-100 promotion.
3. Tail admission is safe for top-rank metrics but not strictly safe for recall.
4. Do not promote M214/M215 as final ranking policy.
5. The useful next role for this branch is candidate expansion before reranking,
   where tail additions can help recall without replacing the final top-100
   order. If final SQL/runtime ranking must stay self-contained, the next model
   work needs a deeper no-harm scorer trained directly on family-balanced
   official-surface failures, not another threshold sweep.

### M216: Feature-Type Bucket Gate

M216 tested a stricter version of the adaptive idea: do not use dataset id, but
separate queries by runtime-safe data characteristics. The buckets include:

```text
query length
question vs keyword style
BM25/SAE overlap
BM25+SAE/scorer overlap
BM25, SAE, fusion, and scorer score concentration/entropy
basic conjunctions such as long-query + low-overlap
```

This is intentionally not a dataset profile. Each heldout dataset is evaluated
with bucket statistics learned from the other datasets, using only query/ranking
shape features.

The first single-feature bucket gate was too conservative and selected no
residual queries. A relaxed version with lower harm penalty produced macro
gain, but still fully selected harmful `webis-touche2020` cases:

| Gate | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Failure |
| --- | ---: | ---: | ---: | ---: | --- |
| single bucket, harm 1.0 | +0.0005 | +0.0012 | +0.0003 | +0.0008 | `webis` -0.0304 R |
| single bucket, harm 2.0 | +0.0001 | +0.0012 | -0.0005 | +0.0004 | `webis` -0.0304 R |

Adding feature conjunction buckets improved separation slightly, but still did
not solve the no-harm problem:

| Gate | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Failure |
| --- | ---: | ---: | ---: | ---: | --- |
| conjunction, harm 1.0 | +0.0013 | +0.0015 | +0.0002 | +0.0010 | `webis` -0.0259 R |
| conjunction, harm 2.0 | -0.0009 | +0.0009 | -0.0007 | +0.0003 | `webis` -0.0304 R |
| conjunction, harm 4.0 | -0.0013 | +0.0005 | -0.0005 | -0.0004 | `webis` -0.0224 R |

Interpretation:

1. Feature-type adaptation is conceptually correct and not cheating, but the
   current hand-built runtime features are not discriminative enough.
2. `webis`/argument-like failures are not separable by simple overlap,
   entropy, query length, or shallow conjunctions.
3. More threshold sweeps or bucket combinations are unlikely to fix this.
4. If this line continues, it needs a learned scorer/fusion model trained with
   feature-type no-harm objectives and richer text/query intent features, not a
   hand-written bucket prior.

Decision: close M216 as a useful diagnostic but do not promote it. The next
valid direction is a learned feature-type objective that uses no dataset id but
can infer intent/type from richer query and candidate-surface features. Until
that exists, M214/M215 should only be used as optional candidate expansion
before reranking, not as final top-100 ranking.

### M217: Learned Feature-Group Adaptive Policy

M217 tested the next version of the same idea: learn a query-level action
policy, but make the objective feature-group aware rather than dataset-aware.
The model is still LODO and never receives dataset id. Runtime-safe inputs are:

```text
query text hash features
BM25/SAE/fusion/scorer overlap and score-shape features
action-surface overlap/score features
feature groups such as query length, query style, BM25/SAE overlap,
score entropy, fusion anchor, scorer shift, and selected conjunctions
```

The first learned policy confirmed that there is a real adaptive signal, but
global harm penalties alone were still not enough:

| Policy | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Failure |
| --- | ---: | ---: | ---: | ---: | --- |
| learned action policy | +0.0003 | +0.0012 | +0.0019 | +0.0006 | `webis` -0.0203 R |
| high harm penalty | +0.0006 | +0.0004 | +0.0003 | +0.0004 | `webis` -0.0157 R |

This showed that a single global no-harm penalty still sacrifices a small set
of query/data types for macro gain.

M217 then added a feature-group action-safety mask. For each training fold, the
policy estimates whether an action is safe for each runtime feature group. At
evaluation time, a query can only use an action if all matched feature groups
allow it. This is deliberately not a benchmark profile; it is a runtime
surface-type constraint.

Best current M217 result:

```text
/home/huoju/leask/runs/
ii42-m217-adaptive-policy-fg-conj-blendonly-safe0-v1
```

| Policy | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| feature conjunction + safety, blend-only | +0.0002 | +0.0000 | +0.0008 | +0.0004 | -0.0007 |

Selected dataset deltas:

| Dataset | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.0000 | +0.0000 | +0.0000 | +0.0000 |
| `fiqa` | +0.0003 | +0.0013 | +0.0016 | +0.0017 |
| `nfcorpus` | +0.0011 | +0.0005 | -0.0001 | +0.0001 |
| `nq` | -0.0001 | +0.0008 | +0.0008 | +0.0006 |
| `webis-touche2020` | -0.0007 | -0.0034 | -0.0021 | -0.0010 |

Important negative controls:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| blend-only, no conjunction safety | -0.0002 | +0.0001 | +0.0011 | +0.0006 | still unsafe |
| conjunction safety with all actions | +0.0002 | +0.0002 | +0.0003 | +0.0001 | tail actions hurt balance |
| stricter action floor `0.0005` | +0.0001 | -0.0001 | +0.0006 | +0.0003 | over-constrained |

Interpretation:

1. The user's concern is correct: the objective needs to adapt by data/query
   characteristics, not by dataset id.
2. Feature conjunctions are necessary. Single runtime features were too coarse;
   failures concentrated in intersections such as BM25-anchored, low scorer
   shift, and flat SAE heads.
3. Tail admission is still not safe as a final ranking action. It may remain
   useful as reranker candidate expansion, but should not be promoted into the
   self-contained top-100 policy yet.
4. The current best safe direction is a learned blend-only policy with
   feature-conjunction action safety.
5. M217 is a meaningful improvement over M216, but it is still a small
   correction layer. To get a larger breakthrough, the same feature-group
   objective should be moved into the scorer/fusion training itself, not just
   applied as a post-hoc action mask.

Decision: keep M217 as the active adaptive-policy branch. Do not continue
manual bucket sweeps. The next model work should train the scorer/fusion head
with feature-group no-harm and positive-admission objectives directly, while
keeping the M217 action-safety report as the promotion gate.

### M218: Query-Conditioned Adaptive Fusion Scorer

M218 expanded the adaptive range beyond fixed actions. Instead of selecting
among precomputed `blend` or `tail` actions, it trains a query-conditioned
source-feature scorer:

```text
query/surface features -> source-feature weights
doc features = BM25, SAE, BM25+SAE fusion, learned scorer ranks/scores
```

The goal was to test whether the adaptive idea can be moved closer to the
scorer/fusion layer itself.

The first model-only canary was a clear negative result. On the five diagnostic
datasets (`arguana`, `fiqa`, `nfcorpus`, `nq`, `webis-touche2020`), replacing
the final ranking with the adaptive scorer damaged all aggregate metrics:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: | ---: |
| model-only | -0.0100 | -0.0183 | -0.0222 | -0.0162 |

This confirms the same pattern seen in M212/M214/M217: the learned scorer has
signal, but it must not own top-rank ordering.

M218 then tested the scorer as a small residual over the existing
`BM25+SAE source score` ranking:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| residual alpha 0.05 | +0.0002 | +0.0000 | -0.0008 | +0.0001 | top-rank unsafe |
| residual alpha 0.10 | +0.0004 | +0.0004 | -0.0008 | +0.0002 | top-rank unsafe |

The residual scorer preserved or improved recall, but still hurt NDCG on some
query types. The useful role is therefore not general scoring, but admission.

The final M218 canary preserved the base top-50 and only let the adaptive
scorer affect the tail:

```text
/home/huoju/leask/runs/
ii42-m218-adaptive-fusion-10ds-alpha10-preserve50-v1
```

10-dataset LODO result:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta | Min NDCG delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| alpha 0.10, preserve top-50 | +0.0002 | +0.0000 | +0.0000 | +0.0000 | +0.0000 | +0.0000 |

Selected dataset deltas:

| Dataset | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.0007 | +0.0000 | +0.0000 | +0.0000 |
| `nfcorpus` | +0.0008 | +0.0000 | +0.0000 | +0.0001 |
| `trec-covid` | +0.0000 | +0.0000 | +0.0000 | +0.0002 |
| `webis-touche2020` | +0.0000 | +0.0000 | +0.0000 | +0.0000 |

Interpretation:

1. The adaptive scorer can learn useful tail/admission corrections.
2. Letting it reorder the top of the list is still unsafe.
3. The stable abstraction is now clearer:
   `base BM25+SAE owns top-rank`, adaptive scorer/gate owns
   `tail admission / candidate expansion`.
4. M217 and M218 agree on the same design boundary. Self-contained final
   ranking can use adaptive logic only under top-rank preservation or
   feature-group action safety.

Decision: keep M218 as a small positive branch, but not as a replacement
ranker. The next meaningful expansion is to train the scorer with this role
explicitly: admission-positive loss, top-rank preservation loss, and
feature-group no-harm loss, instead of training a free scorer and constraining
it after the fact.

### M219: Tail-Specific Adaptive Admission Training

M219 moved the M218 finding into the training objective. Instead of training a
free adaptive scorer and only preserving top ranks at inference time, M219
trains the scorer for the role it is allowed to play:

```text
preserve base top-K
train loss mainly on candidates outside the preserved base region
optimize tail positive admission
keep base distillation to avoid score drift
```

The same `research_sae_m218_adaptive_fusion_scorer.py` runner now supports
tail-specific losses:

```text
loss_tail_listwise
loss_tail_pairwise
tail_preserve_k
```

Five-dataset canary, `alpha=0.10`, preserve top-50:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| M218 free scorer, preserve top-50 | +0.0006 | +0.0000 | +0.0000 | +0.0001 | +0.0000 |
| M219 tail-specific loss, preserve top-50 | +0.0006 | +0.0000 | +0.0000 | +0.0000 | +0.0001 |

The canary showed that the tail-specific objective is at least as safe as M218
and slightly cleaner by per-dataset recall minimum.

The best current 10-dataset M219 run is:

```text
/home/huoju/leask/runs/
ii42-m219-tail-adaptive-fusion-10ds-a10p80-v1
```

10-dataset LODO result:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta | Min NDCG delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M218 alpha 0.10, preserve top-50 | +0.0002 | +0.0000 | +0.0000 | +0.0000 | +0.0000 | +0.0000 |
| M219 alpha 0.10, preserve top-50 | +0.0005 | +0.0000 | +0.0000 | +0.0001 | -0.0004 | +0.0000 |
| M219 alpha 0.05, preserve top-50 | -0.0000 | +0.0000 | +0.0000 | +0.0000 | -0.0001 | +0.0000 |
| M219 alpha 0.10, preserve top-80 | +0.0008 | +0.0000 | +0.0000 | +0.0000 | +0.0000 | +0.0000 |

Selected deltas for the best current run:

| Dataset | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.0014 | +0.0000 | +0.0000 | +0.0000 |
| `nfcorpus` | +0.0001 | +0.0000 | +0.0000 | +0.0001 |
| `scifact` | +0.0033 | +0.0000 | +0.0000 | +0.0000 |
| `webis-touche2020` | +0.0015 | +0.0000 | +0.0000 | +0.0001 |

M219 also tested whether the small gain was caused by a shallow candidate
surface. A deeper five-dataset canary used `source_top_k=150` and
`candidate_k=360`. It did not improve the tradeoff:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5ds, source top-150, candidate 360 | +0.0005 | +0.0000 | +0.0000 | +0.0000 | -0.0003 |

Interpretation:

1. The adaptive scorer can learn a stable admission-only role when the loss is
   aligned with that role.
2. The best current preserve boundary is top-80, not top-50. It gives a larger
   safe recall gain while fully preserving MRR/NDCG.
3. Lower alpha over-constrains the signal. Deeper candidate surfaces are not
   currently the bottleneck and can reintroduce small per-dataset regressions.
4. This still does not close the gap to dense retrieval; it is a safe tail
   improvement, not a representation breakthrough.

Decision: promote M219 as the current active adaptive-admission branch. The
next useful expansion should not be a wider candidate surface. It should train
feature-group-aware tail admission more directly, especially distinguishing
`BM25-only relevant hits`, `SAE-only relevant hits`, and `high-BM25 false
positives` inside the preserved-tail boundary.

### M220: Source-Aware Tail Loss Check

M220 tested whether the M219 tail-admission loss should explicitly weight
tail positives by source type. The runner now has neutral-by-default knobs for:

```text
tail_sae_positive_weight
tail_scorer_positive_weight
tail_bm25_only_positive_weight
tail_high_bm25_negative_weight
```

The aggressive first canary upweighted SAE/scorer-only positives and
high-BM25 negatives. It produced a positive macro recall delta, but regressed
`webis-touche2020`, so it failed the no-harm gate:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5ds aggressive source-aware | +0.0003 | +0.0000 | +0.0000 | +0.0000 | -0.0023 |

A softer version only mildly upweighted SAE/scorer-only positives and removed
the high-BM25 negative penalty. That fixed the `webis` recall regression on
the five-dataset canary:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5ds soft source-aware | +0.0008 | +0.0000 | +0.0000 | +0.0000 | +0.0000 |

However, the 10-dataset LODO check did not beat the simpler M219 top-80 tail
objective:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta | Min MAP delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M219 top-80 tail loss | +0.0008 | +0.0000 | +0.0000 | +0.0000 | +0.0000 | +0.0000 |
| M220 soft source-aware | +0.0006 | +0.0000 | +0.0000 | +0.0000 | +0.0000 | -0.0000 |

Interpretation:

1. Source-aware tail weighting has a real signal on some datasets, especially
   `fiqa`, `nfcorpus`, `scidocs`, and `trec-covid`.
2. The same weighting can remove useful `webis-touche2020` tail documents or
   lower MAP, so the source buckets are not stable enough as direct loss
   multipliers.
3. M219 remains the best current adaptive-admission result because it is
   simpler, slightly stronger on 10-dataset recall, and has cleaner no-harm
   behavior.

Decision: park source-aware loss weighting. If this signal is reused, it should
be a runtime diagnostic or a small calibration feature, not a direct
positive/negative loss multiplier.

### M221: Bounded Score Calibrator Check

M221 tested the hypothesis that BM25 and SAE do have enough combined signal,
but their scores need a learned calibration layer instead of a fixed fusion
weight. Unlike M218's free scorer, M221 constrains the model to a bounded
residual over the existing BM25+SAE score:

```text
calibrated_score = base_bm25_sae_score + alpha * tanh(residual)
```

The model also preserves the base head of the ranking, so it cannot freely
replace top-rank ordering. This is a score-calibration test, not a new
representation model.

Five-dataset canary:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta | Failure |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| alpha 0.10, preserve top-80 | +0.0007 | +0.0000 | +0.0000 | -0.0002 | -0.0112 | `webis` collapse |
| alpha 0.05, preserve top-95 | +0.0001 | +0.0000 | +0.0000 | -0.0001 | -0.0060 | `webis` still negative |

Per-dataset signal from the first canary was very uneven:

| Dataset | R@100 delta | MAP@100 delta |
| --- | ---: | ---: |
| `arguana` | +0.0014 | +0.0000 |
| `fiqa` | +0.0038 | +0.0001 |
| `nfcorpus` | +0.0069 | +0.0002 |
| `nq` | +0.0023 | +0.0000 |
| `webis-touche2020` | -0.0112 | -0.0013 |

Interpretation:

1. The user's hypothesis is partially validated: a learned calibrator can find
   large tail-admission gains that fixed BM25/SAE weights miss.
2. The same calibrator still cannot be trusted as a final top-100 scorer. Even
   bounded residuals and top-95 preservation allow bad tail swaps on `webis`.
3. The issue is not only model capacity; it is missing no-harm awareness for
   query/surface types where the base BM25+SAE ranking is already safer than
   the learned correction.

Decision: do not promote M221 as a final scoring calibrator. Keep it as
evidence that calibration/admission signal exists. The next useful route is a
two-stage policy:

```text
calibrator proposes tail candidates
feature-group safety gate decides whether those candidates are admissible
base BM25+SAE remains the final head owner
```

This is closer to rerank-candidate expansion than to self-contained final
ranking. If final SQL-only ranking remains required, the calibrator needs a
feature-group no-harm gate before it can be reconsidered.

### M222: Calibrator + Feature-Group Safety Gate

M222 tested the natural follow-up from M221: let the bounded calibrator propose
tail candidates, but only apply those candidates when runtime-safe feature
groups looked non-harmful on the training fold. This combines:

```text
M221 bounded score calibrator
M217-style feature-group safety
base BM25+SAE remains top-rank owner
```

The gate uses no dataset id. It only sees groups such as query shape,
BM25/SAE overlap, scorer shift, score entropy, fusion anchor, and their
interpretable conjunctions.

Five-dataset canary:

| Variant | R@100 delta | MRR@20 delta | NDCG@10 delta | MAP@100 delta | Min R delta | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| raw M221 calibrator | +0.0007 | +0.0000 | +0.0000 | -0.0002 | -0.0112 | unsafe |
| M222 gate floor 0.000 | +0.0001 | +0.0000 | +0.0000 | -0.0001 | -0.0072 | unsafe |
| M222 gate floor 0.001 | +0.0005 | +0.0000 | +0.0000 | +0.0000 | -0.0008 | still unsafe |
| M222 gate floor 0.002 | -0.0001 | +0.0000 | +0.0000 | -0.0000 | -0.0008 | no useful gain |

Interpretation:

1. The calibrator's raw signal is strong on some datasets (`fiqa`, `nfcorpus`,
   `nq`), but the harmful cases are not filtered cleanly by the current feature
   groups.
2. Raising the group safety floor reduces `webis-touche2020` damage, but it
   also blocks most useful tail admissions.
3. This confirms the earlier pattern: learned calibration can find candidates,
   but it is still not reliable enough as a self-contained final ranking
   policy.

Decision: do not promote M222 for final SQL ranking. The calibrator route is
only useful if the downstream layer is allowed to rerank or validate admitted
tail candidates. For the current self-contained top-100 ranking goal, M219
remains the best safe branch.
