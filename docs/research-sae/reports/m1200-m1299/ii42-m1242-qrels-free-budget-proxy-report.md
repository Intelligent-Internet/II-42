# M1242 Qrels-Free Budget Proxy Report

M1242 tested whether source score curves can choose a variable atom budget
without qrels or teacher counts.  This is the deployable counterpart to the
M1241 oracle variable-budget upper bound.

## Inputs

- Label surface: M1224 CUB-specific target/harm atoms.
- Score source: `source_abs`.
- Policies:
  - fixed top4/top6/top8/top12/top16.
  - relative-to-max cutoffs.
  - cumulative-mass cutoffs.
  - largest-drop elbow cutoffs.
- Smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.
- Output:
  `runs/m1242_qrels_free_budget_proxy_smoke_v1/m1242_qrels_free_budget_proxy.json`

## Smoke Result

Baseline `fixed_top8`:

| TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| ---: | ---: | ---: | ---: | ---: |
| 0.9770 | 0.1850 | 0.0061 | 0.1789 | 4.602 |

Best qrels-free proxies:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: |
| `mass_0.90` | 0.9862 | 0.1864 | 0.0061 | 0.1803 | 4.610 |
| `rel_0.50` | 0.8940 | 0.1960 | 0.0061 | 0.1899 | 3.976 |
| `mass_0.75` | 0.8756 | 0.2095 | 0.0066 | 0.2029 | 3.643 |
| `elbow_ratio_drop` | 0.6959 | 0.3159 | 0.0063 | 0.3096 | 1.920 |

## Interpretation

The qrels-free score curve does not recover the M1241 oracle budget signal.
The only policy preserving recall is `mass_0.90`, but its gain over fixed top8
is tiny: gap improves by about `0.0014`.  Policies with visibly better gap
lose too much recall.

This means the source score curve alone is not enough to choose a useful
variable budget.  M1241 remains important as an upper bound, but the deployable
budget signal must be learned from richer query/source features or obtained
from a different proposal source.

## Decision

Stop M1242 at smoke.

- Do not run full shared15 for these hand-built qrels-free proxies.
- Do not run native replay.
- Proceed only if the next step uses richer learned budget prediction with
  LODO validation, or changes proposal source.
