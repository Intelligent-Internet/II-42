# M1227 CUB-Specific Failure Mechanics

## Question

M1225 found the strongest recent macro-positive CUB-specific replay signal.
M1226 showed that post-hoc query guards can make the macro surface safer, but
cannot remove row-level fragility.  M1227 does not add another policy.  It
audits why M1225/M1226 still fail on specific rows and whether current
query-time features can separate safe movement from harm.

This run also applies the latest review constraint: before training or tuning,
check target/harm separability.  If current features cannot separate harm, do
not run another threshold or classifier sweep.

## Surface

- Dataset surface: full `shared15`
- Query count: `1342`
- Fixed teacher/selector: M1224 CUB-specific teacher,
  `rule_source_abs_top8`, scale `1.0`
- Replay path: native P1 query atoms path, same as M1225/M1226

## Macro Replay

| Variant | Selected | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1225_cub_source_abs_top8_s1` | 7.970 | +0.001685 | +0.002312 | +0.002033 | +0.002252 | +0.000012 | 0.023946 |
| `m1226_cub_safe_p05` | 5.753 | +0.000072 | +0.001576 | +0.001523 | +0.001600 | +0.000156 | 0.011491 |
| `m1226_cub_rank_safe_p05` | 4.656 | +0.000272 | +0.001102 | +0.001007 | +0.000928 | +0.000157 | 0.008694 |
| `m1226_all_safe_p05` | 4.692 | +0.000272 | +0.001102 | +0.001007 | +0.000928 | +0.000144 | 0.008679 |
| `m1226_rank_safe_p05` | 4.544 | +0.000333 | +0.000734 | +0.000690 | +0.000493 | +0.000171 | 0.006405 |

M1225 remains the strongest macro route.  M1226 guards improve CUB safety but
discard too much useful movement.

## Row Failure Taxonomy

M1225 base still has six row failures:

| Dataset | Class | dRecall | dMAP | dNDCG | dMRR | dCUB | AnyHarmQ |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | `recall+rank` | -0.000590 | -0.002651 | -0.002138 | -0.005526 | +0.000076 | 0.160 |
| `fiqa` | `rank` | +0.000000 | -0.002940 | -0.000867 | +0.000588 | +0.000000 | 0.110 |
| `webis-touche2020` | `rank` | +0.000475 | +0.000434 | -0.001547 | +0.000000 | +0.000000 | 0.388 |
| `trec-covid` | `rank` | +0.000249 | +0.000532 | -0.001530 | +0.000000 | +0.000691 | 0.640 |
| `dbpedia-entity` | `cub` | +0.001910 | +0.004096 | +0.003206 | +0.000000 | -0.000334 | 0.340 |
| `nfcorpus` | `cub` | +0.000056 | +0.002756 | +0.005636 | +0.009430 | -0.000456 | 0.330 |

The failures are not a single class:

- `cqadupstack` is the main rank/recall failure.
- `fiqa`, `webis-touche2020`, and `trec-covid` are rank-boundary failures.
- `dbpedia-entity` and `nfcorpus` are CUB/support failures despite ranking
  gains.

## Separability Audit

Query-time selector features are weak:

| Label | Best query-time feature | Best AUC |
| --- | --- | ---: |
| `all_safe` | `score_max` | 0.5761 |
| `cub_safe` | `score_max` | 0.6276 |
| `rank_safe` | `score_max` | 0.5691 |
| `recall_safe` | `score_max` | 0.7145 |

The first version of this audit briefly included baseline eval metrics.  That
was diagnostic leakage, not deployable evidence.  After separating the tables,
the conclusion is clear:

- Query-time selector features do not cleanly separate safe movement from harm.
- Oracle diagnostic features do separate better:
  `baseline_map_at_100` reaches `0.7730` for `all_safe`, and
  `baseline_recall_at_100` reaches `0.9452` for `cub_safe`.
- Therefore the missing information is not in the current source-abs query
  features; it is in score/rank/context geometry that the current proposal
  source does not expose.

## Decision

The review opinion is useful and should change the working mode:

1. Keep M1225 as the strongest current macro signal.
2. Do not continue threshold/grid/classifier variants over the current
   query-level features.
3. Do not call M1226 a deployment solution; it is safer but weaker.
4. The next valid branch must add observability, not just training depth.

## Next Step

The next branch should be one of these, in this order:

1. Add native rank-context features to the candidate construction:
   boundary margin, top100/top256 rank movement, doc-side atom fanout, BM25
   term coverage, candidate score z-score, and whether selected atoms hit
   already-protected top docs.
2. Re-run a separability-only audit before training.  Acceptance requires
   query-time AUC at least `0.70` for `all_safe/rank_safe` and no worse CUB
   separation.
3. Only if that passes, train an atom-level objective with explicit row-risk
   supervision.  Do not train another shallow query guard on the current
   features.

If added native context still cannot separate safe/harm, the route should move
from post-hoc selection to candidate/source redesign.

## Artifacts

- Script: `scripts/audit_m1227_cub_specific_failure_mechanics.py`
- Smoke JSON:
  `runs/m1227_cub_specific_failure_mechanics_smoke_v1/m1227_cub_specific_failure_mechanics.json`
- Full JSON:
  `runs/m1227_cub_specific_failure_mechanics_v1/m1227_cub_specific_failure_mechanics.json`
- Query records:
  `runs/m1227_cub_specific_failure_mechanics_v1/m1227_query_records.jsonl`
- Generated markdown:
  `runs/m1227_cub_specific_failure_mechanics_v1/m1227_cub_specific_failure_mechanics.md`
