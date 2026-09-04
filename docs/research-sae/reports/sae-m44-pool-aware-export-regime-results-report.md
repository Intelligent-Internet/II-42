# SAE M44 Pool-Aware Export Regime Results Report

Status: closed as the current strongest quality-cost profile.

## Summary

M44 found a new best profile:

```text
M40 checkpoint
pool96 -> export48
fanout_power = 0.25
DF gate threshold = 0.12
low/high SAE = 0.40 / 1.00
```

This improves both quality and physical cost versus M43:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M40 baseline | 0.867007 | 0.898518 | 0.788710 | 0.759559 | 0.470121 | 2948.465 | 2734.691 |
| M43 canonical | 0.866596 | 0.899967 | 0.788874 | 0.759661 | 0.459507 | 2845.310 | 1388.685 |
| M44 canonical | 0.869071 | 0.909016 | 0.794456 | 0.767658 | 0.464084 | 2822.831 | 1157.431 |

M44 is the strongest quality-cost point so far. It improves full15 NDCG/MAP
over M40 and M43 while reducing SAE postings by roughly `58%` versus M40.

## Implementation

M44 extends:

```text
scripts/research_sae_m31_joint_final_ranking_train.py
```

New training/evaluation support:

```text
--train-exported-query-scoring
```

This optional mode computes training ranking scores from the same exported atom
subset used by runtime evaluation.

M44 also fixes two implementation details:

- MPS-safe entropy: sparse exported vectors can contain many zeros, so entropy
  now uses only positive probabilities.
- M31 head loading: finetuning now loads `m31_joint_weight_head_state` from the
  checkpoint when available.

## Eval-Only Pool Results

The useful discovery came before training:

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| pool64/export48/fp0.5 | 0.866596 | 0.899967 | 0.788874 | 0.759661 | 0.459507 | 2845.310 | 1388.685 |
| pool96/export48/fp0.5 | 0.863930 | 0.897445 | 0.783684 | 0.754109 | 0.455596 | 2772.792 | 734.276 |
| pool96/export48/fp0.25 | 0.869071 | 0.909016 | 0.794456 | 0.767658 | 0.464084 | 2822.831 | 1157.431 |
| pool96/export56/fp0.5 | 0.866800 | 0.902100 | 0.789900 | 0.761600 | 0.453700 | 2817.900 | 1059.000 |

Interpretation:

- pool96 is useful;
- fanout power `0.5` is too aggressive for export48 and loses ranking atoms;
- fanout power `0.25` is the right balance for the larger pool.

## Gate Sweep

The best M44 gate sweep row is:

```text
threshold = 0.12
low_sae = 0.40
high_sae = 1.00
```

| Threshold | Low SAE | High SAE | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC NDCG | TREC MAP | SAE postings |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.12 | 0.40 | 1.00 | 0.8691 | 0.9090 | 0.7945 | 0.7677 | 0.6456 | 0.4641 | 1157.4 |
| 0.12 | 0.35 | 1.00 | 0.8677 | 0.9084 | 0.7939 | 0.7664 | 0.6555 | 0.4690 | 1157.4 |
| 0.12 | 0.30 | 1.00 | 0.8660 | 0.9064 | 0.7920 | 0.7644 | 0.6569 | 0.4734 | 1157.4 |

The lower low-SAE settings recover more `trec-covid` MAP but reduce full15
quality. The canonical choice is `0.40/1.00` because the current goal is the
best general quality-cost profile, not dataset-specific TREC recovery.

## Export-Aware Training Result

M44 also tested finetuning with exported-query scoring:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M44 eval-only canonical | 0.869071 | 0.909016 | 0.794456 | 0.767658 | 0.464084 | 2822.831 | 1157.431 |
| M44 export-aware train | 0.863282 | 0.905412 | 0.785630 | 0.753691 | 0.457381 | 2809.998 | 1110.283 |

The finetune is not promoted. It lowers postings slightly, but ranking quality
regresses materially. The best current model remains:

```text
M40 checkpoint + M44 export regime
```

## Decision

M44 replaces M43 as the canonical research profile:

```text
M40-trained query atom pool
+ pool96/export48/fanout_power0.25 payload allocation
+ lexical-DF gate threshold0.12 low0.40 high1.00
```

This is still not a dense-removal product gate pass because hard-dataset
collapses remain, but it is the strongest full15 quality-cost result to date.

## Next Direction

M45 should not blindly finetune the same objective. The failed M44 finetune
shows that the training objective still does not optimize the final exported
ranking surface well enough.

Useful next steps:

1. design listwise training directly over exported rankings, not just exported
   candidate-set dot products;
2. add model selection by `NDCG + MAP - lambda * normalized_sae_postings`;
3. test whether a larger emitted pool, such as pool128/export48, gives more
   headroom before changing the model;
4. keep M44 canonical as the default benchmark profile.
