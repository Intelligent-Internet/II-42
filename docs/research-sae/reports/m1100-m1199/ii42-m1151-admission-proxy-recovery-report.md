# ii42 M1151 Admission Proxy Recovery Report

## Purpose

M1150 proved that strict rank-constrained atom admission is strongly positive,
but it used native metric feedback and is therefore an oracle.  M1151 tests
whether the admitted atoms can be approximated from inference-time proposal
features.

This is a recovery audit only.  It does not replay predicted atoms through the
native index.

## Inputs

- M1150 oracle admission:
  `runs/m1150_rank_constrained_atom_admission_v1/admission_replay.json`
- M1151 output:
  `runs/m1151_admission_proxy_recovery_v1/admission_proxy.json`
- M1151 summary:
  `runs/m1151_admission_proxy_recovery_v1/summary.md`
- Script:
  `scripts/train_m1151_admission_proxy_recovery.py`

## Method

M1151 reuses the M1148 inference-time proposal features:

- query atom presence and weight;
- atom support in native proposal docs;
- top20/top100/top500 support counts;
- impact, rank-discounted impact, and fused/P1/BM25 weighted support.

Labels are M1150 accepted atoms.  The model is trained with
leave-one-dataset-out validation.

## Result

Top-K recovery:

| K | Recall on target queries | Precision on all queries |
| ---: | ---: | ---: |
| 8 | 0.654271 | 0.170302 |
| 16 | 0.815226 | 0.111577 |
| 32 | 0.937072 | 0.066485 |
| 64 | 0.966113 | 0.034920 |

Threshold sweep:

| Threshold | Selected atoms | Atom precision | Atom recall | Query accept rate |
| ---: | ---: | ---: | ---: | ---: |
| 0.500 | 5521 | 0.057236 | 0.913295 | 1.000000 |
| 0.700 | 5499 | 0.057283 | 0.910405 | 1.000000 |
| 0.800 | 5482 | 0.057278 | 0.907514 | 1.000000 |
| 0.900 | 5460 | 0.057509 | 0.907514 | 1.000000 |
| 0.950 | 5437 | 0.057752 | 0.907514 | 1.000000 |
| 0.980 | 5402 | 0.057942 | 0.904624 | 1.000000 |
| 0.990 | 5385 | 0.058124 | 0.904624 | 1.000000 |
| 0.995 | 5366 | 0.058330 | 0.904624 | 1.000000 |

## Interpretation

M1151 is mixed:

- Positive: accepted atoms are learnable enough to rank near the top.
  Top8 recovers `65.4%` of M1150 accepted atoms on target-bearing queries.
- Negative: raw precision is low.  Top8 precision is only `17.0%`, and
  probability thresholding is not usable because scores saturate.

This means a simple probability-threshold admission proxy is not sufficient.
However, the top-ranked atoms are much more enriched than random, so a bounded
native replay is still justified:

- use top1/top2/top4 predicted atoms;
- use small scales;
- stop if the M1149 tradeoff returns.

## Decision

Proceed to M1152 learned-proxy native replay.

M1151 should not be promoted as a gate.  It is a candidate-ranker, not a
calibrated admission model.

## M1152 Gate

M1152 should replay M1151 predictions on the same 149 event queries:

1. Test top1/top2/top4/top8 predicted atoms.
2. Test small scales.
3. Compare against M1129 event-query baseline.
4. Require non-negative CUB/MAP/NDCG/MRR and positive Recall or MAP.

If top-ranked proxy atoms still spend rank quality, the deployable proxy needs
a second-stage risk model or query-level abstention before any full shared15
evaluation.
