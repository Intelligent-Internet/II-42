# SAE M46 Runtime Profile Selector Results Report

Status: promoted as the current best quality-cost runtime profile.

## Summary

M46 tested runtime-safe per-query profile selection across the M44/M45
frontier. It found that a simple fanout fallback beats all static profiles on
the quality-cost objective:

```text
policy = fanout_to_m44
if M45-high predicted SAE postings >= 3000:
    use m44_low_cost
else:
    use m45_high_quality
```

This chooses `m44_low_cost` for only 48 of 1,342 queries and keeps
`m45_high_quality` for the rest. Despite the small number of rerouted queries,
it improves NDCG@10, recovers most of M44's `trec-covid` stability, and lowers
SAE postings versus M45 high-quality.

## Static Frontier

| Profile | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M44 low-cost | 0.869071 | 0.909016 | 0.794456 | 0.767658 | 0.464084 | 2822.831 | 1157.431 |
| M45 balanced | 0.873270 | 0.917110 | 0.798249 | 0.771885 | 0.459633 | 2830.266 | 1324.638 |
| M45 high-quality | 0.871866 | 0.917690 | 0.801211 | 0.774830 | 0.456731 | 2849.424 | 1465.359 |

## Best M46 Rows

| Row | Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | TREC MAP | Candidate docs | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Best quality | `df_or_fanout_to_balanced` | 0.872417 | 0.917915 | 0.801303 | 0.775852 | 0.459633 | 2846.287 | 1400.683 |
| Best quality-cost | `fanout_to_m44` | 0.871949 | 0.917221 | 0.801862 | 0.775242 | 0.463246 | 2844.088 | 1312.060 |

The best quality row gives the highest MAP@100, but the best quality-cost row
is the better canonical profile:

- NDCG@10 is higher than M45 high-quality;
- MAP@100 remains above M45 high-quality;
- TREC MAP recovers from `0.456731` to `0.463246`, close to M44 `0.464084`;
- SAE postings drop from M45 high-quality `1465.359` to `1312.060`;
- it remains clearly better quality than M44 and M45 balanced.

## Evidence

Primary output:

```text
results/sae/m46/profile-selector-sweep-full15-macro/m46_profile_selector_sweep.md
results/sae/m46/profile-selector-sweep-full15-macro/m46_profile_selector_sweep.json
```

The runner first produced a micro-averaged cost report, but was corrected to
dataset-macro physical averaging before the final result. The final static
profiles exactly match the M44/M45 reported physical costs.

## Decision

M46 replaces M45 high-quality as the current canonical research profile:

```text
M40 checkpoint
+ M46 runtime selector:
    if predicted SAE postings >= 3000:
        use M44 low-cost profile
    else:
        use M45 high-quality profile
```

This is still not a product-gate pass. The selector improves the current
frontier, but hard-dataset collapse remains, especially relative to the
fixed-doc teacher.
