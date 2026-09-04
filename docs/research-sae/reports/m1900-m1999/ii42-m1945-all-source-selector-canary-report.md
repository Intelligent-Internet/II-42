# M1945 All-Source Selector Canary

Decision: **stop_sampled_separability_not_deployable**

## Held-Out Ranking

| Dataset | Residual | SourceMean | Method | K | TargetRecall | AnyTarget | HarmRecall | AnyHarm |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| fiqa | 250 | 7381.5 | proposal | 8 | 0.0025 | 0.0200 | 0.1320 | 0.6640 |
| fiqa | 250 | 7381.5 | selector | 8 | 0.0010 | 0.0080 | 0.0000 | 0.0000 |
| fiqa | 250 | 7381.5 | proposal | 64 | 0.0141 | 0.0840 | 0.2970 | 0.9320 |
| fiqa | 250 | 7381.5 | selector | 64 | 0.0020 | 0.0160 | 0.0000 | 0.0000 |
| arguana | 16 | 9321.2 | proposal | 8 | 0.0000 | 0.0000 | 0.1094 | 0.6250 |
| arguana | 16 | 9321.2 | selector | 8 | 0.0078 | 0.0625 | 0.0000 | 0.0000 |
| arguana | 16 | 9321.2 | proposal | 64 | 0.0000 | 0.0000 | 0.1875 | 0.6875 |
| arguana | 16 | 9321.2 | selector | 64 | 0.0078 | 0.0625 | 0.0000 | 0.0000 |
| nfcorpus | 271 | 10572.0 | proposal | 8 | 0.0042 | 0.0295 | 0.1642 | 0.7749 |
| nfcorpus | 271 | 10572.0 | selector | 8 | 0.0000 | 0.0000 | 0.0000 | 0.0000 |
| nfcorpus | 271 | 10572.0 | proposal | 64 | 0.0475 | 0.2251 | 0.3012 | 0.9336 |
| nfcorpus | 271 | 10572.0 | selector | 64 | 0.0000 | 0.0000 | 0.0000 | 0.0000 |
| scifact | 14 | 9963.4 | proposal | 8 | 0.0000 | 0.0000 | 0.1429 | 0.6429 |
| scifact | 14 | 9963.4 | selector | 8 | 0.0000 | 0.0000 | 0.0000 | 0.0000 |
| scifact | 14 | 9963.4 | proposal | 64 | 0.0089 | 0.0714 | 0.1964 | 0.7143 |
| scifact | 14 | 9963.4 | selector | 64 | 0.0000 | 0.0000 | 0.0000 | 0.0000 |

## Global

| Method | K | TargetRecall | AnyTarget | HarmRecall | AnyHarm |
| --- | ---: | ---: | ---: | ---: | ---: |
| proposal | 8 | 0.0032 | 0.0236 | 0.1475 | 0.7169 |
| selector | 8 | 0.0007 | 0.0054 | 0.0000 | 0.0000 |
| proposal | 64 | 0.0300 | 0.1506 | 0.2933 | 0.9201 |
| selector | 64 | 0.0011 | 0.0091 | 0.0000 | 0.0000 |

## Gate

| Check | Passed |
| --- | --- |
| `dataset_count` | `true` |
| `finite_scores` | `true` |
| `residual_queries_per_dataset` | `true` |
| `top8_target_recall_global` | `false` |
| `top8_target_recall_per_dataset` | `false` |
| `top8_any_target_global` | `false` |
| `top8_any_target_per_dataset` | `false` |
| `top8_harm_recall_global` | `true` |
| `top8_harm_recall_per_dataset` | `true` |
| `top64_target_recall_global` | `false` |
| `top64_target_recall_per_dataset` | `false` |
| `top64_any_target_global` | `false` |
| `top64_any_target_per_dataset` | `false` |
| `top64_harm_recall_global` | `true` |
| `top64_harm_recall_per_dataset` | `true` |
| `selector_beats_proposal_top8` | `false` |
| `selector_beats_proposal_top64` | `false` |

## Interpretation

M1944 sampled AUC does not survive the complete held-out action space under the declared safety gates. Do not train or sweep this selector family.

The selector sees every qrels-free source dimension. Qrels are used only for held-out labels and metrics.

## Diagnosis

The four rows contain 551 residual queries, 4,398 target assignments and 4,408
harm assignments. The selector finds only 3 targets at top 8 and 5 at top 64.
The original proposal finds 14 and 132 respectively, so the learned selector
is not merely below the absolute gate: it is worse than the fixed proposal.

Harm recall is exactly zero, but this is not a safety success. The mean
per-query maximum target-minus-harm selector score remains strongly positive
on every row (`+0.882` to `+0.972`), while neither class reaches the selected
head. Unseen neutral dimensions dominate the top ranks. M1944's label-aware
sample of 32 hard neutrals did not represent the extreme-value competition
among 7,381-10,572 source dimensions per query.

The failure is therefore attributed to action-space observability and negative
support, not insufficient optimizer steps. M1945 performs no neural training,
and its complete-source counterexample invalidates the sampled-AUC promotion
argument directly.

## Decision

- Do not run native replay.
- Do not train the M1944 selector or sweep classifier, features or thresholds.
- Close the current full-tail residual-selector family.
- Retain M1934 `b1`/`b1.125` as the learned-sparse frontier.
- Any future residual route must first provide a qrels-free source whose fixed
  budget has useful target coverage before a learned selector is introduced.
