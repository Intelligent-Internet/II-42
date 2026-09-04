# M555 Pairwise-Order Posting Encoder Report

M555 returns from DREAM-lite proxy experiments to the larger encoder/posting
route, while keeping M551 as the stable BM25-free first-stage baseline.

The change is narrow:

- keep M551's locked-support residual posting encoder;
- keep M551's candidate-set/listwise dense-teacher KL;
- add qrels-free pairwise dense-order loss from the earlier M398 family;
- test whether a larger hidden head can absorb the order signal without
  breaking dense-derived top128 support.

Qrels are used only for held-out evaluation.

## Implementation

Code:

```text
scripts/research_sae_m551_dream_lite_posting.py
scripts/run_m555_pairwise_order_posting_spark.sh
```

New arguments:

- `--pairwise-weight`
- `--pairwise-winners`
- `--pairwise-scale`

Default `pairwise-weight=0.0`, so earlier M551-M554 behavior remains
reproducible unless the M555 runner enables the loss.

## Baseline

Same-task M551 promoted seed552 baseline on:

```text
FiQA2018, SCIDOCS, TRECCOVID
```

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m551_shared_locked_support_residual_dream_lite` | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |

## Seed552 Smoke

All rows are deltas against M551 seed552 on the same tasks/split.

| Variant | Selected epochs | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `pairwise_w0.20` | `[3, 2, 4]` | -0.00019 | +0.00005 | +0.00032 | -0.00013 | -0.00014 |
| `pairwise_w0.10` | `[3, 2, 4]` | -0.00021 | -0.00001 | +0.00015 | -0.00012 | -0.00006 |
| `pairwise_w0.20_h768` | `[2, 1, 4]` | -0.01050 | +0.00068 | +0.00196 | -0.02035 | +0.00235 |
| `pairwise_w0.20_h768_s0125` | `[3, 2, 4]` | -0.00040 | +0.00140 | +0.00087 | +0.00158 | +0.00094 |

Remote results:

```text
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_seed552/m555_pairwise_w0.20_seed552.json
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.10_seed552/m555_pairwise_w0.10_seed552.json
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_seed552/m555_pairwise_w0.20_h768_seed552.json
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_seed552/m555_pairwise_w0.20_h768_s0125_seed552.json
```

## Interpretation

The small-head pairwise variants are almost identical to M551.  The larger
`h768` head proves capacity can improve admission metrics, but at the default
residual scale it oversteers and damages top-rank behavior.

The first genuinely useful shape is:

```text
PAIRWISE_WEIGHT=0.20
HIDDEN_DIMS=768
RESIDUAL_SCALE=0.0125
```

This improves MAP, Recall, MRR, and dense overlap versus M551 on seed552, with
only a small NDCG drop.  It is not a promoted version yet, but it is the first
post-M551 experiment with a plausible scale-up path.

## Three-Seed Smoke

`pairwise_w0.20_h768_s0125` was repeated on seeds `551`, `552`, and `553`.
Rows below are deltas against the matching M551 seed on the same three-task
surface.

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.00047 | -0.00119 | -0.00025 | -0.00197 | -0.00011 |
| 552 | -0.00040 | +0.00140 | +0.00087 | +0.00158 | +0.00094 |
| 553 | +0.00224 | -0.00042 | +0.00007 | -0.00019 | -0.00132 |
| Mean | +0.00077 | -0.00007 | +0.00023 | -0.00019 | -0.00017 |

Remote seed results:

```text
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_seed551/m555_pairwise_w0.20_h768_s0125_seed551.json
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_seed552/m555_pairwise_w0.20_h768_s0125_seed552.json
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_seed553/m555_pairwise_w0.20_h768_s0125_seed553.json
```

Interpretation: the three-task signal is weak but not dead.  The mean improves
NDCG and Recall slightly, while MAP/MRR/overlap are essentially flat.  This is
not enough for promotion, but it is stronger than M553/M554 and justifies one
broad10 verification run.

## Broad10 Seed552

The first broad10 validation completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_broad10_seed552/
```

Macro comparison against matching M551 seed552:

| Metric | Dense top128 sparse | M551 | M555 | M555 - M551 |
| --- | ---: | ---: | ---: | ---: |
| NDCG@10 | 0.49713 | 0.50141 | 0.50226 | +0.00085 |
| MAP@100 | 0.35647 | 0.36002 | 0.36183 | +0.00181 |
| Recall@100 | 0.65734 | 0.65980 | 0.66170 | +0.00189 |
| MRR@20 | 0.58886 | 0.59670 | 0.59906 | +0.00236 |
| Dense O@100 | 0.52737 | 0.52988 | 0.53062 | +0.00074 |

Per-task deltas are mixed, but the macro surface improves every tracked metric.
The largest positive task is `HotpotQAHardNegatives`; the main regressions are
small losses on CQADupstack, SCIDOCS, TRECCOVID NDCG, and Touche NDCG.

## Broad10 Seed553

Seed553 broad10 completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_broad10_seed553/
```

Macro comparison against matching M551 seed553:

| Metric | M551 | M555 | M555 - M551 |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.51178 | 0.51143 | -0.00034 |
| MAP@100 | 0.36874 | 0.36747 | -0.00127 |
| Recall@100 | 0.66179 | 0.65922 | -0.00257 |
| MRR@20 | 0.62304 | 0.62231 | -0.00072 |
| Dense O@100 | 0.53817 | 0.53836 | +0.00020 |

Two-seed broad10 mean delta, seeds `552` and `553`:

| Metric | Mean M555 - M551 |
| --- | ---: |
| NDCG@10 | +0.00025 |
| MAP@100 | +0.00027 |
| Recall@100 | -0.00034 |
| MRR@20 | +0.00082 |
| Dense O@100 | +0.00047 |

Interpretation: seed552 was clearly positive, but seed553 is negative on
MAP/Recall/MRR.  The two-seed average is slightly positive for most metrics,
but not strong enough to promote.

## Broad10 Seed551

Seed551 broad10 completed on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m555-pairwise-order-posting-v1/pairwise_w0.20_h768_s0125_broad10_seed551/
```

Macro comparison against matching M551 seed551:

| Metric | M551 | M555 | M555 - M551 |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.51689 | 0.51789 | +0.00100 |
| MAP@100 | 0.37026 | 0.37024 | -0.00002 |
| Recall@100 | 0.66407 | 0.66442 | +0.00035 |
| MRR@20 | 0.60899 | 0.60867 | -0.00032 |
| Dense O@100 | 0.53148 | 0.53186 | +0.00038 |

Seed551 is positive on NDCG/Recall/overlap, flat on MAP, and slightly weaker
on MRR.

## Three-Seed Broad10 Verdict

Rows below are deltas against matching M551 seeds on the same broad10 surface.

| Seed | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dO@100 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 551 | +0.00100 | -0.00002 | +0.00035 | -0.00032 | +0.00038 |
| 552 | +0.00085 | +0.00181 | +0.00190 | +0.00236 | +0.00074 |
| 553 | -0.00035 | -0.00127 | -0.00257 | -0.00072 | +0.00019 |
| Mean | +0.00050 | +0.00017 | -0.00011 | +0.00044 | +0.00044 |

Interpretation: M555 is a weak positive over M551, not a dead route.  It
improves NDCG, MAP, MRR, and dense overlap on the three-seed broad10 mean.
Recall is down by only `0.00011`, which is not a material loss, but the margin
is too small to call this a final usable version.

M555 can be kept as the current best M551-family broad10 candidate, but the
route still needs a stronger stabilization pass before broader claims.

## Follow-Up Smokes

Two cheap follow-ups were run after the three-seed broad10 verdict.

### Lower Pairwise Weight

`PAIRWISE_WEIGHT=0.10`, `HIDDEN_DIMS=768`, `RESIDUAL_SCALE=0.0125`,
seed552, same three-task smoke:

| Model | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 `w0.20` | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M555 `w0.10` | 0.44616 | 0.20232 | 0.43638 | 0.59105 | 0.54901 |

Lowering the pairwise weight does not rescue the route.  It slightly improves
NDCG/MRR versus `w0.20`, but loses MAP/Recall and remains below M551 on NDCG
and Recall.

### More Epochs

`PAIRWISE_WEIGHT=0.20`, `HIDDEN_DIMS=768`, `RESIDUAL_SCALE=0.0125`,
`EPOCHS=8`, seed552, same three-task smoke:

| Model | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 | 0.44646 | 0.20171 | 0.43691 | 0.58941 | 0.54823 |
| M555 `e4` | 0.44606 | 0.20311 | 0.43778 | 0.59099 | 0.54917 |
| M555 `e8` | 0.43923 | 0.20323 | 0.43797 | 0.57015 | 0.55133 |

More epochs increase dense overlap and slightly improve MAP/Recall, but they
badly damage top-rank quality.  This is not a training-depth problem; deeper
training oversteers the ranking surface.

## Current Route Decision

Keep M555 `w0.20_h768_s0125_e4` as a weak positive M551-family candidate.
Stop the immediate micro-sweeps on lower pairwise weight, shallow
selection-pairwise weighting, and more epochs.  The remaining useful next step
needs a stronger objective/interface change, not more local tuning around the
same gate.

## Follow-Up Gate Fix

While waiting for seed551, M555 exposed a selection-design issue.  The training
objective includes pairwise dense-order loss, but the M555 validation payload
does not record validation pairwise loss and the checkpoint selection key does
not use it.  This means the new dense-order signal can affect optimization
without directly affecting checkpoint selection.

M556 addresses only this gate problem:

```text
validation_listwise_loss
    + selection_pairwise_weight * validation_pairwise_loss
```

The model, support lock, candidate construction, and training loss remain the
same.  M556 is therefore a targeted test of whether M555's seed instability is
mostly a checkpoint-selection issue.
