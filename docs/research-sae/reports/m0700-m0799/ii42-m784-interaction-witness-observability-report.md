# M784 Interaction Witness Observability

M784 adds lexical query/document interaction witnesses to M779 bundle
rows and tests transfer both globally and inside the cross-zero safety
set identified by M781.

## Counts

| Scope | Split | Rows | Positives | Positive Queries | Tasks |
| --- | --- | ---: | ---: | ---: | ---: |
| all | dev | 10413 | 318 | 105 | 12 |
| all | test | 10569 | 332 | 118 | 12 |
| cross_zero | dev | 2193 | 318 | 105 | 12 |
| cross_zero | test | 2303 | 332 | 118 | 12 |

## Cross-Zero Feature Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| boosted_lex_coverage_max | 0.685203 | 0.719687 | +1 |
| boosted_lex_coverage_mean | 0.655800 | 0.715463 | +1 |
| demoted_lex_coverage_mean | 0.642464 | 0.700310 | +1 |
| demoted_lex_coverage_max | 0.654465 | 0.691949 | +1 |
| top_demoted_lex_coverage | 0.547670 | 0.650037 | +1 |
| top_boosted_lex_coverage | 0.544111 | 0.600527 | +1 |
| demoted_count | 0.574570 | 0.586694 | -1 |
| affected_count | 0.580414 | 0.586416 | -1 |
| boosted_count | 0.579519 | 0.579734 | -1 |
| lex_coverage_gain_weighted | 0.513739 | 0.559431 | +1 |
| lex_coverage_delta_mean | 0.538506 | 0.559128 | +1 |
| boosted_lex_jaccard_mean | 0.512122 | 0.547989 | +1 |
| entropy_delta_at_100 | 0.553842 | 0.545243 | -1 |
| demoted_lex_jaccard_mean | 0.503784 | 0.534409 | +1 |

## All-Row Feature Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| cross_rate_at_100 | 0.907132 | 0.903732 | -1 |
| boosted_lex_coverage_mean | 0.741897 | 0.773306 | +1 |
| affected_count | 0.772551 | 0.765274 | -1 |
| demoted_count | 0.763938 | 0.763305 | -1 |
| demoted_lex_coverage_mean | 0.727148 | 0.756517 | +1 |
| boosted_count | 0.765142 | 0.753954 | -1 |
| boosted_lex_coverage_max | 0.722987 | 0.742817 | +1 |
| demoted_lex_coverage_max | 0.698688 | 0.718991 | +1 |
| top_demoted_lex_coverage | 0.585735 | 0.682109 | +1 |
| top_boosted_lex_coverage | 0.581084 | 0.625129 | +1 |
| tail_abs_share_at_100 | 0.638940 | 0.616826 | +1 |
| boosted_lex_jaccard_mean | 0.612226 | 0.616602 | +1 |
| total_scale | 0.607292 | 0.614821 | -1 |
| demoted_lex_jaccard_mean | 0.598856 | 0.598014 | +1 |

## Decision

M784 finds interaction witnesses beyond native safety; run one bounded M785 selector smoke.
