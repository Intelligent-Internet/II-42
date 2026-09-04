# M710 Target Rank Budget Ceiling Report

M710 measures how deep M704 target atoms are in the existing expanded candidate
order.

This is a no-training first-stage audit:

- no BM25
- no reranker
- no qrels loss
- no native reranking sweep
- no learned gate

## Why This Was Run

M705-M709 all failed to recover M704 target coverage with budget 384:

| Version | Mechanism | b384 target recall | Oracle target recall |
| --- | --- | ---: | ---: |
| M706 | pointwise selector attribution | 0.701639 | 0.982671 |
| M707 | query-conditioned/risk selector | 0.596205 | 0.982671 |
| M708 | simple feature ordering | 0.537679 | 0.982671 |
| M709 | oracle-order distillation | 0.700603 | 0.982671 |

M710 asks whether this is source coverage failure or budget compression/order
failure.

## Run

```bash
python3 scripts/audit_m710_target_rank_budget_ceiling.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --budgets 24,48,96,192,384,512,768,1024,1536 \
    --output-root runs/m710_target_rank_budget_ceiling_v1
```

Output:

- `runs/m710_target_rank_budget_ceiling_v1/m710_summary.json`
- `runs/m710_target_rank_budget_ceiling_v1/m710_report.md`

## Eval Rank Distribution

| Queries | Target visibility | Target rank p50 | p90 | p95 | p99 | Query max-rank p95 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 146 | 0.982671 | 174.0 | 859.4 | 1074.2 | 1393.8 | 1513.8 |

## Eval Budget Curve

| Budget | Target recall | Visible target recall | Negative-only |
| ---: | ---: | ---: | ---: |
| 24 | 0.153136 | 0.155837 | 0.193208 |
| 48 | 0.240158 | 0.244393 | 0.161958 |
| 96 | 0.363345 | 0.369753 | 0.122788 |
| 192 | 0.515351 | 0.524439 | 0.089291 |
| 384 | 0.688830 | 0.700978 | 0.061733 |
| 512 | 0.765681 | 0.779183 | 0.051209 |
| 768 | 0.860049 | 0.875216 | 0.039018 |
| 1024 | 0.922961 | 0.939237 | 0.031678 |
| 1536 | 0.982671 | 1.000000 | 0.022653 |

## Dataset Eval Budget Curve

| Dataset | Visibility | b384 | b768 | b1024 | b1536 | Target rank p95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.978326 | 0.627304 | 0.817140 | 0.898605 | 0.978326 | 1176.0 |
| `cqadupstack` | 0.985850 | 0.724626 | 0.885442 | 0.939048 | 0.985850 | 1013.7 |
| `fiqa` | 0.988487 | 0.720482 | 0.888086 | 0.937617 | 0.988487 | 1037.4 |
| `scidocs` | 0.985200 | 0.695057 | 0.864842 | 0.924881 | 0.985200 | 1088.0 |

## Interpretation

M710 resolves an important ambiguity.

The expanded source is not the main blocker:

- eval target visibility is `0.982671`;
- at full candidate cap `1536`, target recall reaches `0.982671`;
- visible target recall reaches `1.0`.

The blocker is budget compression/order:

- budget 384 only captures `0.688830` target recall;
- budget 1024 still only captures `0.922961`;
- budget 1536 is needed to exceed `0.95` target recall under current ordering;
- eval target rank p95 is `1074.2`;
- query max-rank p95 is `1513.8`.

This explains why M705/M709 plateau near `0.70`: they mostly reproduce the
current order's b384 coverage. They do not learn the M704 oracle compression
that selects the right target atoms from the full 1536 candidates.

## Decision

The next step should not be longer training on current scalar features.

The next useful route is a two-stage compiler:

1. Wide source admission: keep the expanded cap high enough to expose target
   atoms, likely `1024-1536`.
2. Learned compression/pruning: train a model specifically to compress from
   wide source to a support-safe budget such as 384.

The compression model cannot be trained from the current scalar atom features
alone. It needs additional interaction features or a different representation:

- query text/dense-root to atom compatibility;
- source-doc interaction features;
- pair-impact labels from native dense-boundary movement;
- setwise diversity/coverage objective over the whole candidate set.

M704 remains valid as the oracle target for this compression stage. M710 shows
why simple selectors fail and what the next architecture must solve.

