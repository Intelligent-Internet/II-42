# M633 / P1.7 Compiler Augmentation Smoke

## Objective

Test a deterministic representation-side correction before training any learned
correction. The correction preserves top20 and only reorders ranks 21..1000.

Config artifact:

`runs/m633_p1p7_representation_compiler_smoke_v1/m633_deterministic_correction_config.json`

## Gate

The smoke gate requires all of the following:

- Recall@100 positive delta.
- NDCG@10 and MRR@20 regression <= 0.001.
- MAP@100 non-negative delta.
- Dense overlap@100 does not decrease.

## Result

Status: `stop_deterministic_augmentation_failed`

Reason: no deterministic representation correction passed the smoke gate.

Best candidate:

| Field | Value |
| --- | ---: |
| Channel | head_artifact_penalty |
| Strength | -0.35 |
| Gate status | dense_overlap_guard_failed |
| dNDCG@10 | +0.000000 |
| dMAP@100 | +0.000165 |
| dRecall@100 | +0.003271 |
| dMRR@20 | +0.000000 |
| dDense overlap@100 | -0.004118 |
| dDense KL | +0.154245 |

Macro metrics:

| Metric | P1.3-a010 baseline | M633 best | Delta |
| --- | ---: | ---: | ---: |
| NDCG@10 | 0.673264 | 0.673264 | +0.000000 |
| MAP@100 | 0.410853 | 0.411018 | +0.000165 |
| Recall@100 | 0.563729 | 0.567001 | +0.003271 |
| MRR@20 | 0.792249 | 0.792249 | +0.000000 |
| dense overlap@100 | 0.932157 | 0.928039 | -0.004118 |
| dense KL | 1.138930 | 1.293175 | +0.154245 |

Per-dataset best candidate:

| Dataset | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | +0.000000 | +0.000509 | +0.009804 | +0.000000 | -0.011765 |
| scifact | +0.000000 | -0.000029 | +0.000000 | +0.000000 | -0.001053 |
| trec-covid | +0.000000 | +0.000020 | +0.000011 | +0.000000 | +0.000667 |

## Comparison To Recent Stops

| Line | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap@100 | Verdict |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| M629-B | +0.000000 | +0.000078 | +0.001923 | +0.000000 | not tracked | finite positive, not enough |
| M630 anchor010 | -0.000922 | +0.001235 | +0.000000 | +0.004724 | +0.000000 | calibration anchor |
| M631 lexical | -0.138108 | -0.045380 | -0.001821 | -0.112151 | -0.087451 | stopped |
| M632 linear | -0.077996 | -0.087987 | +0.003231 | -0.092591 | -0.027451 | stopped |
| M633 best | +0.000000 | +0.000165 | +0.003271 | +0.000000 | -0.004118 | stopped by dense-overlap guard |

## Decision

M633 produces a useful diagnostic signal but does not pass promotion. Penalizing
head artifacts can recover Recall@100 without collapsing NDCG@10 or MRR@20, but
it loses dense overlap and worsens dense KL. Under the M633 acceptance criteria,
this is a stop condition.

M633-D learned correction, shared15, and native replay are not reached.

## Next Direction

Do not continue by adding another scorer on the same surface. If this line is
continued, the next step should repair the compiler/basis itself: identify and
suppress over-strong false-head artifact atoms while keeping dense overlap
within guard.
