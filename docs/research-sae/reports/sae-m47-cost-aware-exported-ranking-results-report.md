# SAE M47 Cost-Aware Exported Ranking Results Report

Status: parked. The focused training arm lowers cost, but loses too much
quality and worsens hard-dataset stability.

## Summary

M47 tested a focused cost-aware exported-ranking finetune from the M40
checkpoint. It successfully reduced SAE postings, but did not beat the M46
runtime selector.

The direct M47 finetune result:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M46 `fanout_to_m44` | 0.871949 | 0.917221 | 0.801862 | 0.775242 | 0.463246 | 2844.088 | 1312.060 |
| M47 direct DF-gate | 0.867890 | 0.908219 | 0.796081 | 0.766390 | 0.449518 | 2800.233 | 1049.822 |

M47 direct training lowers postings by about 20% relative to M46, but the
quality loss is too large:

- NDCG@10 drops by `0.005781`;
- MAP@100 drops by `0.008852`;
- TREC MAP drops by `0.013728`;
- the result falls below M44 on MAP and hard-dataset stability.

## Selector On M47 Checkpoint

To avoid rejecting the trained checkpoint unfairly, M47 also ran the M46
selector sweep on the M47 checkpoint:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M47 selector best | 0.866810 | 0.910967 | 0.797325 | 0.767389 | 0.451237 | 2782.813 | 906.341 |

This is a cheaper point, but it still does not dominate M44 or M46:

- lower postings than M44;
- NDCG above M44;
- MAP slightly below M44;
- TREC MAP materially below M44 and M46.

Therefore M47 is useful as cost-control evidence, but not a better canonical
research profile.

## Evidence

Training output:

```text
results/sae/m47/cost-aware-export-train-pool96-fp0p15/m31_joint_final_ranking_train.md
results/sae/m47/cost-aware-export-train-pool96-fp0p15/m31_joint_final_ranking_train.json
```

Selector-on-trained-checkpoint output:

```text
results/sae/m47/profile-selector-on-cost-aware-train/m46_profile_selector_sweep.md
results/sae/m47/profile-selector-on-cost-aware-train/m46_profile_selector_sweep.json
```

## Decision

Do not continue the current M47 loss family. It is optimizing cost by removing
ranking-useful semantic atoms and worsening broad-query stability.

The next training attempt, if any, must be structurally different:

- train the selector/routing decision, not only atom export weights;
- include collapse-aware model selection directly;
- treat `trec-covid`-like broad high-DF semantic-neighborhood queries as a
  separate validation slice, without dataset-specific rules.
