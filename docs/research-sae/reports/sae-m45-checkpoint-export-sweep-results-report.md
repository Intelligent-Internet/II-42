# SAE M45 Checkpoint Export Sweep Results Report

Status: closed as a new quality/cost frontier, not a product-gate pass.

## Summary

M45 tested whether the M40 checkpoint has additional runtime payload headroom
beyond the M44 canonical profile. It added a reusable checkpoint export sweep
runner and confirmed the best candidates through the normal M31 evaluation
path.

M45 found two useful profiles:

```text
M45 high-quality profile
M40 checkpoint
pool96 -> export48
fanout_power = 0.15
DF gate threshold = 0.12
low/high SAE = 0.45 / 1.00
```

```text
M45 balanced profile
M40 checkpoint
pool128 -> export48
fanout_power = 0.10
DF gate threshold = 0.12
low/high SAE = 0.45 / 1.00
```

The high-quality profile is the strongest full15 NDCG/MAP result so far. The
balanced profile is lower quality than the high-quality profile, but it keeps a
smaller postings increase versus M44.

## Results

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M40 baseline | 0.867007 | 0.898518 | 0.788710 | 0.759559 | 0.470121 | 2948.465 | 2734.691 |
| M44 canonical | 0.869071 | 0.909016 | 0.794456 | 0.767658 | 0.464084 | 2822.831 | 1157.431 |
| M45 balanced | 0.873270 | 0.917110 | 0.798249 | 0.771885 | 0.459633 | 2830.266 | 1324.638 |
| M45 high-quality | 0.871866 | 0.917690 | 0.801211 | 0.774830 | 0.456731 | 2849.424 | 1465.359 |

Compared with M44 canonical, M45 high-quality changes:

| Metric | Delta | Relative |
| --- | ---: | ---: |
| Recall@100 | +0.002795 | +0.32% |
| MRR@20 | +0.008674 | +0.95% |
| NDCG@10 | +0.006755 | +0.85% |
| MAP@100 | +0.007172 | +0.93% |
| TREC MAP | -0.007353 | -1.58% |
| Candidate docs | +26.593 | +0.94% |
| SAE postings | +307.928 | +26.60% |

Compared with M44 canonical, M45 balanced improves aggregate ranking quality
with a smaller postings increase:

| Metric | Delta |
| --- | ---: |
| Recall@100 | +0.004199 |
| MRR@20 | +0.008094 |
| NDCG@10 | +0.003793 |
| MAP@100 | +0.004227 |
| TREC MAP | -0.004452 |
| Candidate docs | +7.435 |
| SAE postings | +167.207 |

## Sweep Evidence

Focused sweep output:

```text
results/sae/m45/checkpoint-export-sweep-m40-focused/m45_checkpoint_export_sweep.md
results/sae/m45/checkpoint-export-sweep-m40-focused/m45_checkpoint_export_sweep.json
```

Exact M31 confirmations:

```text
results/sae/m45/confirm-pool96-export48-fp0p15-gate045/m31_joint_final_ranking_train.md
results/sae/m45/confirm-pool128-export48-fp0p10-gate045/m31_joint_final_ranking_train.md
```

The sweep and exact M31 path match for both confirmed candidates.

## Interpretation

M45 answers the pool128 question narrowly:

- pool128 does not produce the highest quality row;
- pool128 is still useful as a balanced quality/cost point;
- pool96 with weaker fanout discounting is better for maximum aggregate
  ranking quality;
- lower low-SAE settings recover more `trec-covid` MAP but lose full15 NDCG/MAP;
- M45 does not solve the broad-query hard-dataset collapse.

The new recommended frontier is therefore:

```text
M44 canonical:
    lower-cost baseline, SAE postings 1157.431

M45 balanced:
    modest cost increase, better full15 quality, SAE postings 1324.638

M45 high-quality:
    best full15 quality, larger postings increase, SAE postings 1465.359
```

## Decision

M45 promotes the high-quality profile as the best aggregate-quality research
profile, and keeps M44/M45-balanced as lower-cost comparison points.

This is still not a product model closure. `trec-covid` MAP and NDCG remain
below M44 and far below the fixed-doc teacher. The next useful work should not
be another broad grid sweep; it should either:

1. add cost-aware model selection around this frontier; or
2. return to training objective work that specifically targets broad high-DF
   semantic-neighborhood collapse without dataset-specific rules.
