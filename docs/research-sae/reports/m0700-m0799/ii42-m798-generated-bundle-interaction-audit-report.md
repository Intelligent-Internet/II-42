# M798 Generated Bundle Interaction Audit

M798 adds M784-style lexical moved-document witnesses to the M794
generated bundle pool.  It is a feature diagnostic only: qrels are
used for labels, not as selector inputs.

## score_mean_margin

| Scope | Split | Rows | Positives | Positive Queries | Tasks |
| --- | --- | ---: | ---: | ---: | ---: |
| all | dev | 1662 | 166 | 87 | 11 |
| all | test | 1658 | 203 | 89 | 11 |
| cross_zero | dev | 1425 | 166 | 87 | 11 |
| cross_zero | test | 1446 | 203 | 89 | 11 |

### Cross-Zero Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| demoted_lex_coverage_mean | 0.661366 | 0.758906 | +1 |
| boosted_lex_coverage_mean | 0.666287 | 0.753932 | +1 |
| boosted_lex_coverage_max | 0.658036 | 0.730005 | +1 |
| demoted_lex_coverage_max | 0.643215 | 0.724822 | +1 |
| top_demoted_lex_coverage | 0.576835 | 0.645631 | +1 |
| top_boosted_lex_coverage | 0.575514 | 0.636094 | +1 |
| entropy_delta_at_100 | 0.536015 | 0.585838 | -1 |
| rr_min_delta_at_100 | 0.606209 | 0.584483 | -1 |
| demoted_count | 0.550281 | 0.579369 | -1 |
| boosted_lex_jaccard_mean | 0.534326 | 0.576894 | +1 |
| affected_count | 0.550872 | 0.575562 | -1 |
| boosted_count | 0.546382 | 0.572092 | -1 |

### All-Row Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| demoted_lex_coverage_mean | 0.664571 | 0.757307 | +1 |
| boosted_lex_coverage_mean | 0.669617 | 0.753965 | +1 |
| boosted_lex_coverage_max | 0.661277 | 0.727481 | +1 |
| demoted_lex_coverage_max | 0.644534 | 0.723969 | +1 |
| top_demoted_lex_coverage | 0.580830 | 0.647971 | +1 |
| top_boosted_lex_coverage | 0.576515 | 0.636201 | +1 |
| demoted_count | 0.567042 | 0.596738 | -1 |
| affected_count | 0.568629 | 0.594500 | -1 |

## margin_bundle

| Scope | Split | Rows | Positives | Positive Queries | Tasks |
| --- | --- | ---: | ---: | ---: | ---: |
| all | dev | 1697 | 188 | 94 | 11 |
| all | test | 1665 | 197 | 91 | 11 |
| cross_zero | dev | 1499 | 188 | 94 | 11 |
| cross_zero | test | 1471 | 197 | 91 | 11 |

### Cross-Zero Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| boosted_lex_coverage_mean | 0.698135 | 0.769127 | +1 |
| demoted_lex_coverage_mean | 0.674623 | 0.764029 | +1 |
| boosted_lex_coverage_max | 0.687140 | 0.756447 | +1 |
| demoted_lex_coverage_max | 0.658298 | 0.742862 | +1 |
| top_boosted_lex_coverage | 0.629418 | 0.669782 | +1 |
| top_demoted_lex_coverage | 0.536139 | 0.636422 | +1 |
| demoted_lex_jaccard_mean | 0.549422 | 0.596574 | +1 |
| boosted_lex_jaccard_mean | 0.566516 | 0.582449 | +1 |
| boosted_count | 0.592972 | 0.573257 | -1 |
| demoted_lex_jaccard_max | 0.548268 | 0.572186 | +1 |
| boosted_lex_jaccard_max | 0.557261 | 0.572066 | +1 |
| affected_count | 0.595554 | 0.570142 | -1 |

### All-Row Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| boosted_lex_coverage_mean | 0.703340 | 0.768897 | +1 |
| demoted_lex_coverage_mean | 0.680971 | 0.762716 | +1 |
| boosted_lex_coverage_max | 0.691674 | 0.754364 | +1 |
| demoted_lex_coverage_max | 0.660022 | 0.741772 | +1 |
| top_boosted_lex_coverage | 0.630719 | 0.667518 | +1 |
| top_demoted_lex_coverage | 0.540803 | 0.638795 | +1 |
| demoted_lex_jaccard_mean | 0.558768 | 0.599310 | +1 |
| boosted_count | 0.609971 | 0.593428 | -1 |

## conservative_margin

| Scope | Split | Rows | Positives | Positive Queries | Tasks |
| --- | --- | ---: | ---: | ---: | ---: |
| all | dev | 1686 | 179 | 91 | 11 |
| all | test | 1639 | 185 | 91 | 11 |
| cross_zero | dev | 1454 | 179 | 91 | 11 |
| cross_zero | test | 1428 | 185 | 91 | 11 |

### Cross-Zero Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| boosted_lex_coverage_mean | 0.661049 | 0.750247 | +1 |
| demoted_lex_coverage_mean | 0.655009 | 0.745618 | +1 |
| boosted_lex_coverage_max | 0.651145 | 0.736166 | +1 |
| demoted_lex_coverage_max | 0.636677 | 0.719321 | +1 |
| top_boosted_lex_coverage | 0.574652 | 0.648318 | +1 |
| top_demoted_lex_coverage | 0.552376 | 0.626192 | +1 |
| entropy_delta_at_100 | 0.539766 | 0.583788 | -1 |
| rr_min_delta_at_100 | 0.593658 | 0.581690 | -1 |
| demoted_lex_jaccard_mean | 0.556909 | 0.566274 | +1 |
| boosted_lex_jaccard_mean | 0.552398 | 0.564576 | +1 |
| demoted_count | 0.556709 | 0.559166 | -1 |
| boosted_count | 0.554494 | 0.557224 | -1 |

### All-Row Transfer

| Feature | Train AUC | Eval AUC | Sign |
| --- | ---: | ---: | ---: |
| boosted_lex_coverage_mean | 0.665201 | 0.752476 | +1 |
| demoted_lex_coverage_mean | 0.659131 | 0.746498 | +1 |
| boosted_lex_coverage_max | 0.653392 | 0.734695 | +1 |
| demoted_lex_coverage_max | 0.638403 | 0.720783 | +1 |
| top_boosted_lex_coverage | 0.575675 | 0.649770 | +1 |
| top_demoted_lex_coverage | 0.557467 | 0.630072 | +1 |
| entropy_delta_at_100 | 0.535879 | 0.578728 | -1 |
| demoted_count | 0.575619 | 0.577869 | -1 |

## Decision

M798 finds transferable interaction witnesses on generated bundles. Use them in one bounded M799 selector.
