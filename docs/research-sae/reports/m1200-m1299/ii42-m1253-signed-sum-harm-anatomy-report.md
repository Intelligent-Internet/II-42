# M1253 Signed-Sum Harm Anatomy

## Goal

M1252 showed that `signed_sum_s1` is macro-positive on full `shared15`,
but not row-stable enough to become a default native policy.  M1253 checks
whether those harmful rows have observable qrels-free structure before adding
another guard, threshold, or classifier.

This directly follows the current stop rule:

- do not scale a selector while target/harm remain mixed
- ask whether the harm is observable first
- only replay a guard if the observable split is strong enough

## Runs

Smoke:

- JSON:
  `runs/m1253_signed_sum_harm_anatomy_smoke_v1/m1253_signed_sum_harm_anatomy.json`
- Markdown:
  `runs/m1253_signed_sum_harm_anatomy_smoke_v1/m1253_signed_sum_harm_anatomy.md`
- Datasets:
  `cqadupstack`, `scidocs`, `webis-touche2020`
- Query count: `249`

Full `shared15`:

- JSON:
  `runs/m1253_signed_sum_harm_anatomy_v1/m1253_signed_sum_harm_anatomy.json`
- Markdown:
  `runs/m1253_signed_sum_harm_anatomy_v1/m1253_signed_sum_harm_anatomy.md`
- Query count: `1342`

## Full Shared15 Buckets

| Bucket | Count | AnyNeg | CubNeg | RankNeg | RecallNeg | MeanScoreDelta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `delta_neg_count:0` | 1342 | 0.1803 | 0.0298 | 0.1602 | 0.0201 | -0.090227 |
| `delta_pos_count:1-2` | 1 | 1.0000 | 1.0000 | 1.0000 | 0.0000 | -0.074579 |
| `delta_pos_count:3+` | 1341 | 0.1797 | 0.0291 | 0.1596 | 0.0201 | -0.090239 |
| `delta_sum:positive` | 1342 | 0.1803 | 0.0298 | 0.1602 | 0.0201 | -0.090227 |
| `mean_presence:consensus` | 12 | 0.5833 | 0.1667 | 0.4167 | 0.1667 | -0.062866 |
| `mean_presence:mixed` | 1218 | 0.1806 | 0.0304 | 0.1609 | 0.0189 | -0.097149 |
| `mean_presence:singleish` | 112 | 0.1339 | 0.0089 | 0.1250 | 0.0179 | -0.017882 |

## Result

There is no useful qrels-free harm separator in the current feature view.

The most direct guard candidates fail:

- `delta_sum` is always positive on full `shared15`, so it cannot separate harm.
- `delta_neg_count` is always zero, so it cannot separate harm.
- `delta_pos_count` is almost always `3+`, so it provides no actionable split.
- `mean_presence:singleish` is less risky, but its mean `NDCG@10` delta is
  slightly negative and it covers only `112` queries.
- `mean_presence:consensus` is too small and riskier, with `58.33%` any-negative
  rows across only `12` queries.

The full-run macro remains the M1251/M1252 signed-sum result:

| Policy | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `signed_sum_s1` | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 |

That result is real, but the observable buckets do not justify turning it into
a guarded native default.

## Decision

Do not proceed to an M1254 simple guard replay from these features.

Keep `signed_sum_s1` as:

- a positive native replay signal
- a teacher/action source for later objective design
- evidence that query-time signed deltas can move ranking safely at macro level

Do not treat it as:

- an engineering default
- a row-safe policy
- a candidate for more threshold/grid/classifier tweaking using these features

## Next Step

The next useful branch must change the candidate construction or teacher, not
just guard the current signed-sum policy.

Minimum requirement for the next branch:

1. Show target/harm separation on full `shared15` before replay.
2. Use features that vary across queries and can explain harmful movement.
3. Avoid another threshold-only policy unless the bucket audit shows a clear
   prior split.

The practical direction is to use `signed_sum_s1` as a training/objective signal
inside a stronger proposal source, then rerun the same anatomy gate before any
native replay.
