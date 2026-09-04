# M1252 Signed-Sum Policy Stability

## Goal

M1251 promoted `all-actions top8 + signed_sum query-time delta, scale=1.0`
as the best native replay candidate.  M1252 checks whether it is stable enough
to become a native policy candidate by adding:

- per-dataset deltas
- query-level negative counts
- worst-row localization

## Run

- Full shared15:
  `runs/m1252_signed_sum_policy_stability_v1/m1252_signed_sum_policy_stability.json`
- Markdown:
  `runs/m1252_signed_sum_policy_stability_v1/m1252_signed_sum_policy_stability.md`

## Macro Result

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m1252_signed_sum_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |

Macro is clean: all five metrics improve.

## Per-Dataset Risk

Negative datasets:

| Dataset | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `cqadupstack` | 3 | +0.000037 | -0.001615 | -0.000430 | -0.006181 | +0.000191 | -0.099961 |
| `fiqa` | 2 | +0.000000 | -0.001622 | -0.000120 | +0.001846 | +0.000000 | -0.018825 |
| `webis-touche2020` | 1 | +0.000475 | +0.001229 | -0.000776 | +0.000000 | +0.000000 | -0.003252 |
| `nfcorpus` | 1 | -0.010010 | +0.004632 | +0.011868 | +0.013645 | +0.001793 | +0.016663 |
| `dbpedia-entity` | 1 | -0.000278 | +0.008269 | +0.004432 | +0.000500 | +0.000099 | +0.033381 |
| `scidocs` | 1 | +0.012000 | +0.006986 | +0.004396 | +0.008581 | -0.002500 | +0.041914 |
| `trec-covid` | 1 | +0.001024 | +0.001348 | -0.000244 | +0.000000 | +0.002560 | +0.008800 |

Query-level negative counts:

| Metric | NegativeQueries |
| --- | ---: |
| `candidate_upper_bound` | 40 |
| `map_at_100` | 188 |
| `mrr_at_20` | 36 |
| `ndcg_at_10` | 104 |
| `recall_at_100` | 27 |

## Interpretation

This is a real positive signal, but not yet a default policy.

The macro result is strong enough to continue:

- all macro metrics improve
- the policy is deployable in principle because it uses query-time deltas
- it beats the mean-teacher shape from M1251

But the row/dataset stability is not good enough for engineering default:

- 7 of 15 datasets have at least one negative metric
- `cqadupstack` and `fiqa` are negative overall
- query-level MAP/NDCG regressions are common even when macro improves

## Decision

Keep `signed_sum_s1` as the new primary signal, but do not promote it as a
default policy yet.

Next step: M1253 harm anatomy.  The question is whether the negative rows have
observable qrels-free structure.  If yes, test a guard.  If no, use signed_sum
as a training signal/objective component rather than a direct policy.
