# M1152 Admission Proxy Native Replay

## Objective

Replay the M1151 learned admission proxy through the native shared/event-query
path and test whether a deployable fixed topK/scale policy can preserve the
M1150 rank-constrained admission signal.

## Inputs

- Proxy artifact:
  `runs/m1151_admission_proxy_recovery_v1/admission_proxy.json`
- Native replay:
  `runs/m1152_admission_proxy_native_replay_v1/native_replay.json`
- Summary:
  `runs/m1152_admission_proxy_native_replay_v1/summary.md`

The replay used the same native SQL feature path as M1149/M1150 and evaluated
149 event queries. Variant failures were zero after filtering numerically tiny
proxy scores before casting impacts to PostgreSQL `real[]`.

## Macro Result

Baseline:

| Metric | Value |
| --- | ---: |
| CUB | 0.971258 |
| Recall@100 | 0.457891 |
| MAP@100 | 0.320512 |
| NDCG@10 | 0.483830 |
| MRR@20 | 0.779512 |

Best conservative fixed policy:

| Variant | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| top4_s0.01 | +0.000100 | +0.001216 | +0.000614 | +0.001320 | +0.000112 |
| top4_s0.02 | +0.000061 | +0.001322 | +0.000509 | +0.001411 | +0.000112 |
| top1_s0.05 | +0.000020 | +0.001564 | +0.000809 | +0.000432 | +0.000000 |

High-gain but not CUB-safe:

| Variant | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| top4_s0.05 | -0.000359 | +0.012081 | +0.001922 | +0.002228 | +0.001790 |
| top8_s0.05 | -0.001308 | +0.007512 | -0.001412 | -0.001013 | -0.002671 |

## Interpretation

This is the first deployable-ish positive signal after M1150:

- M1150 proved the oracle admission shape can improve all metrics together.
- M1151 learned a high-recall but poorly calibrated admission proxy.
- M1152 shows a fixed conservative policy, `top4_s0.01`, survives native replay
  with all macro metrics positive.

The signal is real but still small. The larger `top4_s0.05` move recovers much
more Recall@100, MAP@100, NDCG@10, and MRR@20, but spends CUB and has dataset
level risk on rows such as `trec-covid`, `dbpedia-entity`, and
`webis-touche2020`.

## Decision

Keep `top4_s0.01` as the current conservative learned-admission baseline. Do
not promote `top4_s0.05` as a fixed policy because it violates the CUB floor.

The next useful experiment is not another blind scale sweep. It should be a
risk-aware admission policy:

1. Use inference-time proxy-score shape features to decide whether a query can
   safely use the larger `top4_s0.05` move.
2. Fall back to `top4_s0.01` or no-op when risk is high.
3. Require LODO validation and preserve CUB/MAP/NDCG/MRR floors.
