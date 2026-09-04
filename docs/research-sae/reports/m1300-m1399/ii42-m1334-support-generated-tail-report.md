# M1334 Support-Generated Tail

## Question

M1333 rejected candidate-self tail admission: candidate-only tail atoms outside
`rank_prefix` were not separable enough. M1334 moves one step earlier and
generates tail candidates from native document-support instead of admitting the
existing candidate-self tail.

This tests whether query-time document support can produce a cleaner tail
candidate source.

This is a target/harm frontier audit only. It does not run native retrieval.

## Smoke Result

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

Tail label distribution:

| Label | Count |
| --- | ---: |
| `target` | 191 |
| `harm` | 29712 |

The source is extremely imbalanced. Target base rate is about `0.64%`.

Model separability:

| Metric | Value |
| --- | ---: |
| ROC-AUC | 0.9739 |
| Average precision | 0.2075 |

The high ROC-AUC means document-support features can rank targets above many
harms. But the absolute candidate universe is so noisy that top selections are
still not precise enough.

## Frontier

| Policy | TargetRecall | Precision | HarmPrecision | HarmRecall | PredCount | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `model_score_top2` | 0.5759 | 0.2209 | 0.7791 | 0.0131 | 2.000 | -0.7680 |
| `model_score_top1` | 0.3298 | 0.2530 | 0.7470 | 0.0063 | 1.000 | -0.9143 |
| `model_thr_p50_cap2` | 0.3246 | 0.2222 | 0.7778 | 0.0073 | 1.120 | -1.0124 |
| `model_thr_p75_cap2` | 0.1152 | 0.1818 | 0.8182 | 0.0033 | 0.486 | -1.3410 |
| `support_top100_top2` | 0.0890 | 0.0341 | 0.9659 | 0.0162 | 2.000 | -1.8167 |
| `prior_score_top2` | 0.0157 | 0.0060 | 0.9940 | 0.0167 | 2.000 | -1.9746 |

Train-fold threshold/abstain did not solve the problem. It lowers coverage but
does not improve precision enough.

Per-dataset best policies:

| Dataset | BestPolicy | TargetRecall | Precision | HarmPrecision | PredCount |
| --- | --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | `model_score_top2` | 0.4545 | 0.1750 | 0.8250 | 2.000 |
| `scidocs` | `model_score_top2` | 0.8030 | 0.2650 | 0.7350 | 2.000 |
| `webis-touche2020` | `model_thr_p75_cap2` | 0.1667 | 0.3636 | 0.6364 | 0.449 |

## Interpretation

Support-generated tail atoms are not clean enough for native replay.

This is not because support features have no signal. They have a strong ranking
signal. The failure is that the generated support-tail universe is too broad:
even after modeling and abstention, the selected atoms remain mostly harm.

This gives a sharper boundary than M1237:

- document support can help rank target atoms;
- document support alone is not a safe candidate generator;
- adding threshold/abstain does not make it deployable.

## Decision

Do not run native replay for support-generated tail.

Do not continue this exact source with more thresholds or classifier depth.

The next branch must reduce the candidate universe before scoring, not only
score a huge support universe. Viable next sources must be structurally
narrower than all support atoms from native top256.

## Next Valid Work

M1335 should be an anatomy audit, not another replay:

1. Compare target base rate and target coverage across possible tail universes:
   candidate-self tail, rank-fill tail, support top100, support boundary/tail,
   and intersections between these sources.
2. Identify whether any pre-scoring universe has a materially better target
   base rate than support-all and candidate-self.
3. Only if such a universe exists, train a selector/generator inside that
   narrower source.
4. If no narrower universe exists, stop tail-generation work and switch to a
   different structural route.

## Artifacts

- Script:
  `scripts/audit_m1334_support_generated_tail.py`
- Smoke JSON:
  `runs/m1334_support_generated_tail_smoke_v2/m1334_support_generated_tail.json`
- Smoke Markdown:
  `runs/m1334_support_generated_tail_smoke_v2/m1334_support_generated_tail.md`
