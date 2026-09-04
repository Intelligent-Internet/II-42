# M711 Missed Target Feature Profile Report

M711 profiles the atoms that current b384 compression misses.

This is a no-training first-stage audit:

- no BM25
- no reranker
- no qrels loss
- no native reranking sweep
- no learned gate

## Why This Was Run

M710 showed the expanded source has enough coverage but current ordering needs
very large budgets:

| Budget | Eval target recall |
| ---: | ---: |
| 384 | 0.688830 |
| 768 | 0.860049 |
| 1024 | 0.922961 |
| 1536 | 0.982671 |

M711 asks why b384 misses the remaining target atoms.

## Run

```bash
python3 scripts/audit_m711_missed_target_feature_profile.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --capture-budget 384 \
    --output-root runs/m711_missed_target_feature_profile_v1
```

Output:

- `runs/m711_missed_target_feature_profile_v1/m711_summary.json`
- `runs/m711_missed_target_feature_profile_v1/m711_report.md`

## Eval Group Profile

| Group | Count | Rank mean | Rank p50 | Rank p90 | In query | Boundary | Abs vote | Score weight |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `captured_target` | 3657 | 119.9 | 88.0 | 290.0 | 0.251846 | 0.146021 | 0.383921 | 4.903971 |
| `missed_target` | 1560 | 758.8 | 691.5 | 1210.0 | 0.071154 | 0.014744 | 0.055731 | 0.770416 |
| `selected_non_target` | 52407 | 197.6 | 199.0 | 348.0 | 0.168756 | 0.086382 | 0.206796 | 2.782970 |
| `visible_target` | 5217 | 310.9 | 174.0 | 859.4 | 0.197815 | 0.106766 | 0.285785 | 3.667945 |

## Largest Missed-vs-NonTarget Gaps

| Feature | Missed target mean | Selected non-target mean | Delta |
| --- | ---: | ---: | ---: |
| `candidate_rank` | 758.816026 | 197.568836 | 561.247189 |
| `score_weight_sum` | 0.770416 | 2.782970 | -2.012554 |
| `vote_count_log` | 2.174321 | 3.188711 | -1.014390 |
| `candidate_rank_frac` | 0.494021 | 0.128626 | 0.365395 |
| `abs_vote_sum` | 0.055731 | 0.206796 | -0.151065 |
| `in_query` | 0.071154 | 0.168756 | -0.097602 |
| `boundary_atom` | 0.014744 | 0.086382 | -0.071638 |
| `same_sign_vote_sum` | 0.004603 | 0.047947 | -0.043343 |

## Interpretation

The b384 compression failure is not random.

The missed target atoms look weaker than the non-target atoms selected by b384
under almost every existing scalar feature:

- they are much deeper in the candidate list;
- they are less likely to already be in the query;
- they are less likely to be boundary atoms;
- they have lower vote count and lower score weight;
- they have lower same-sign support.

This explains why M705-M709 plateau. The current scalar features rank
high-confidence non-target atoms above deep target atoms. A classifier trained
on these features cannot reliably recover the oracle compression without
additional information.

## Decision

Do not continue training on the same scalar candidate feature space.

The next compiler needs a representation that can explain why a deep, weak
atom is useful for the dense-boundary pair. Candidate directions:

1. Pair-impact features: direct score contribution against dense-positive and
   dense-negative boundary docs.
2. Source-doc interaction features: whether the atom is supported by documents
   that explain the dense boundary miss, not just by high-ranking current
   source docs.
3. Text/dense-root compatibility: generate atom probabilities from the query
   encoder output rather than selecting from popularity-like atom features.

M711 strengthens the current diagnosis: M704 is a valid upper bound, but the
selector needs new information, not more steps.

