# M821 Proposal-Diversity Rebuild Smoke

M821 rebuilds proposal diversity and tests only the oracle ceiling.
It does not train a selector.  The baseline selected set is the same
M816-style floor-0.50 residual selector used by M819/M820.

## Proposal Pool

| Mode | Surface | Queries | Bundles | Eval Candidates | Mean/query | Max/query |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| same_direction | original | 510 | 2115 | 458 | 4.1 | 34 |
| same_direction | seed7642 | 507 | 2113 | 557 | 4.2 | 31 |
| same_direction | seed7643 | 507 | 2090 | 531 | 4.1 | 27 |
| mixed_direction | original | 510 | 4792 | 1054 | 9.4 | 66 |
| mixed_direction | seed7642 | 507 | 4772 | 1262 | 9.4 | 55 |
| mixed_direction | seed7643 | 507 | 4798 | 1235 | 9.5 | 55 |

## Expanded Oracle Replay

| Mode | Held-out | Selected | Safe Rate | No Safe | Expanded Cand | Families | Safe Cand | Base Clean | Base Utility | Oracle Clean | Oracle Utility | Negative Tasks |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| same_direction_expanded_only | original | 3 | 1.000 | 2 | 2.3 | 2.3 | 1.0 | 1 | +0.000076 | 1 | +0.000003 | none |
| same_direction_expanded_only | seed7642 | 3 | 1.000 | 2 | 9.0 | 9.0 | 4.7 | 1 | +0.000012 | 1 | +0.000034 | none |
| same_direction_expanded_only | seed7643 | 8 | 0.875 | 3 | 4.4 | 4.4 | 2.0 | 0 | +0.000007 | 1 | +0.000016 | none |
| same_direction_old_plus_expanded | original | 3 | 1.000 | 0 | 8.0 | 4.0 | 5.7 | 1 | +0.000076 | 1 | +0.000076 | none |
| same_direction_old_plus_expanded | seed7642 | 3 | 1.000 | 0 | 21.0 | 12.0 | 8.7 | 1 | +0.000012 | 1 | +0.000036 | none |
| same_direction_old_plus_expanded | seed7643 | 8 | 0.875 | 1 | 10.4 | 5.1 | 5.9 | 0 | +0.000007 | 1 | +0.000021 | none |
| mixed_direction_expanded_only | original | 3 | 1.000 | 2 | 5.0 | 5.0 | 2.0 | 1 | +0.000076 | 1 | +0.000005 | none |
| mixed_direction_expanded_only | seed7642 | 3 | 1.000 | 2 | 18.3 | 18.3 | 8.3 | 1 | +0.000012 | 1 | +0.000278 | none |
| mixed_direction_expanded_only | seed7643 | 8 | 0.875 | 2 | 10.2 | 10.2 | 4.4 | 0 | +0.000007 | 1 | +0.000055 | none |
| mixed_direction_old_plus_expanded | original | 3 | 1.000 | 0 | 10.7 | 6.7 | 6.7 | 1 | +0.000076 | 1 | +0.000077 | none |
| mixed_direction_old_plus_expanded | seed7642 | 3 | 1.000 | 0 | 30.3 | 21.3 | 12.3 | 1 | +0.000012 | 1 | +0.000280 | none |
| mixed_direction_old_plus_expanded | seed7643 | 8 | 0.875 | 0 | 16.2 | 11.0 | 8.2 | 0 | +0.000007 | 1 | +0.000059 | none |

## Residual Rows

| Mode | Held-out | Task | Query | Action | Selected Utility | Best Safe Utility | Expanded Candidates | Families | Safe Candidates |
| --- | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| same_direction_expanded_only | original | msmarco | 182539 | drop_no_safe | +0.000649 | +0.000000 | 0 | 0 | 0 |
| same_direction_expanded_only | original | msmarco | 1113437 | drop_no_safe | +0.018957 | +0.000000 | 0 | 0 | 0 |
| same_direction_expanded_only | original | trec-covid | 38 | upgrade_safe | +0.000865 | +0.000921 | 7 | 7 | 3 |
| same_direction_expanded_only | seed7642 | climate-fever | 67 | drop_no_safe | +0.000198 | +0.000000 | 0 | 0 | 0 |
| same_direction_expanded_only | seed7642 | climate-fever | 97 | drop_no_safe | +0.000375 | +0.000000 | 0 | 0 | 0 |
| same_direction_expanded_only | seed7642 | webis-touche2020 | 17 | upgrade_safe | +0.002568 | +0.009246 | 27 | 27 | 14 |
| same_direction_expanded_only | seed7643 | msmarco | 182539 | drop_no_safe | +0.000649 | +0.000000 | 0 | 0 | 0 |
| same_direction_expanded_only | seed7643 | msmarco | 1114646 | drop_no_safe | +0.000534 | +0.000000 | 0 | 0 | 0 |
| same_direction_expanded_only | seed7643 | nfcorpus | PLAIN-270 | upgrade_safe | +0.000063 | +0.000242 | 2 | 2 | 2 |
| same_direction_expanded_only | seed7643 | nfcorpus | PLAIN-344 | upgrade_safe | +0.000007 | +0.000267 | 4 | 4 | 2 |
| same_direction_expanded_only | seed7643 | nfcorpus | PLAIN-660 | drop_no_safe | -0.000981 | +0.000000 | 1 | 1 | 0 |
| same_direction_expanded_only | seed7643 | trec-covid | 1 | upgrade_safe | +0.000067 | +0.001592 | 16 | 16 | 8 |
| same_direction_expanded_only | seed7643 | trec-covid | 7 | upgrade_safe | +0.000071 | +0.000071 | 9 | 9 | 2 |
| same_direction_expanded_only | seed7643 | trec-covid | 33 | upgrade_safe | +0.001465 | +0.002222 | 3 | 3 | 2 |
| same_direction_old_plus_expanded | original | trec-covid | 38 | upgrade_safe | +0.000865 | +0.000921 | 13 | 7 | 7 |
| same_direction_old_plus_expanded | seed7642 | webis-touche2020 | 17 | upgrade_safe | +0.002568 | +0.009246 | 39 | 27 | 21 |
| same_direction_old_plus_expanded | seed7643 | nfcorpus | PLAIN-270 | upgrade_safe | +0.000063 | +0.000242 | 8 | 2 | 8 |
| same_direction_old_plus_expanded | seed7643 | nfcorpus | PLAIN-344 | upgrade_safe | +0.000007 | +0.000267 | 8 | 4 | 5 |
| same_direction_old_plus_expanded | seed7643 | nfcorpus | PLAIN-660 | drop_no_safe | -0.000981 | +0.000000 | 4 | 1 | 0 |
| same_direction_old_plus_expanded | seed7643 | trec-covid | 1 | upgrade_safe | +0.000067 | +0.001592 | 23 | 16 | 15 |
| same_direction_old_plus_expanded | seed7643 | trec-covid | 33 | upgrade_safe | +0.001465 | +0.002222 | 12 | 3 | 8 |
| mixed_direction_expanded_only | original | msmarco | 182539 | drop_no_safe | +0.000649 | +0.000000 | 0 | 0 | 0 |
| mixed_direction_expanded_only | original | msmarco | 1113437 | drop_no_safe | +0.018957 | +0.000000 | 0 | 0 | 0 |
| mixed_direction_expanded_only | original | trec-covid | 38 | upgrade_safe | +0.000865 | +0.001284 | 15 | 15 | 6 |
| mixed_direction_expanded_only | seed7642 | climate-fever | 67 | drop_no_safe | +0.000198 | +0.000000 | 0 | 0 | 0 |
| mixed_direction_expanded_only | seed7642 | climate-fever | 97 | drop_no_safe | +0.000375 | +0.000000 | 0 | 0 | 0 |
| mixed_direction_expanded_only | seed7642 | webis-touche2020 | 17 | upgrade_safe | +0.002568 | +0.075221 | 55 | 55 | 25 |
| mixed_direction_expanded_only | seed7643 | msmarco | 182539 | drop_no_safe | +0.000649 | +0.000000 | 0 | 0 | 0 |
| mixed_direction_expanded_only | seed7643 | msmarco | 1114646 | drop_no_safe | +0.000534 | +0.000000 | 0 | 0 | 0 |
| mixed_direction_expanded_only | seed7643 | nfcorpus | PLAIN-270 | upgrade_safe | +0.000063 | +0.000507 | 6 | 6 | 4 |
| mixed_direction_expanded_only | seed7643 | nfcorpus | PLAIN-344 | upgrade_safe | +0.000007 | +0.008221 | 10 | 10 | 4 |
| mixed_direction_expanded_only | seed7643 | nfcorpus | PLAIN-660 | replace_with_safe | -0.000981 | +0.002158 | 3 | 3 | 1 |
| mixed_direction_expanded_only | seed7643 | trec-covid | 1 | upgrade_safe | +0.000067 | +0.001592 | 36 | 36 | 16 |
| mixed_direction_expanded_only | seed7643 | trec-covid | 7 | upgrade_safe | +0.000071 | +0.000074 | 21 | 21 | 6 |
| mixed_direction_expanded_only | seed7643 | trec-covid | 33 | upgrade_safe | +0.001465 | +0.002222 | 6 | 6 | 4 |
| mixed_direction_old_plus_expanded | original | trec-covid | 38 | upgrade_safe | +0.000865 | +0.001284 | 21 | 15 | 10 |
| mixed_direction_old_plus_expanded | seed7642 | webis-touche2020 | 17 | upgrade_safe | +0.002568 | +0.075221 | 67 | 55 | 32 |
| mixed_direction_old_plus_expanded | seed7643 | nfcorpus | PLAIN-270 | upgrade_safe | +0.000063 | +0.000507 | 12 | 6 | 10 |
| mixed_direction_old_plus_expanded | seed7643 | nfcorpus | PLAIN-344 | upgrade_safe | +0.000007 | +0.008221 | 14 | 10 | 7 |
| mixed_direction_old_plus_expanded | seed7643 | nfcorpus | PLAIN-660 | replace_with_safe | -0.000981 | +0.002158 | 6 | 3 | 1 |
| mixed_direction_old_plus_expanded | seed7643 | trec-covid | 1 | upgrade_safe | +0.000067 | +0.001592 | 43 | 36 | 23 |
| mixed_direction_old_plus_expanded | seed7643 | trec-covid | 7 | upgrade_safe | +0.000071 | +0.000074 | 29 | 21 | 10 |
| mixed_direction_old_plus_expanded | seed7643 | trec-covid | 33 | upgrade_safe | +0.001465 | +0.002222 | 15 | 6 | 10 |

## Decision

M821 found a clean useful expanded oracle ceiling in mixed_direction_old_plus_expanded. The proposal pool can be widened before training a new selector.
