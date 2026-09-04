# M712 Banded Budget Coverage Report

M712 tests whether deterministic head+tail budget selection can recover target
atoms missed by b384.

This is a no-training first-stage audit:

- no BM25
- no reranker
- no qrels loss
- no native reranking sweep
- no learned gate

## Why This Was Run

M710 showed visible target atoms extend deep into the cap1536 candidate list.
M711 showed b384 is dominated by high-ranked non-target atoms. A simple
alternative is to keep some head atoms and reserve part of the budget for deep
tail atoms.

M712 tests that alternative before training another model.

## Run

```bash
python3 scripts/audit_m712_banded_budget_coverage.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --budget 384 \
    --heads 0,64,128,192,256,320,384 \
    --tail-starts 1,193,385,513,769 \
    --output-root runs/m712_banded_budget_coverage_v1
```

Output:

- `runs/m712_banded_budget_coverage_v1/m712_summary.json`
- `runs/m712_banded_budget_coverage_v1/m712_report.md`

## Eval Result

| Variant | Target recall | Oracle target recall | Negative-only | Oracle overlap |
| --- | ---: | ---: | ---: | ---: |
| `head384` | 0.688830 | 0.982671 | 0.061733 | 0.935467 |
| `head320_tail385` | 0.660011 | 0.982671 | 0.058505 | 0.780751 |
| `head320_tail321` | 0.660011 | 0.982671 | 0.058826 | 0.789062 |
| `head256_tail257` | 0.632888 | 0.982671 | 0.054955 | 0.655198 |
| `head192_tail193` | 0.581277 | 0.982671 | 0.050371 | 0.534336 |
| `head128_tail129` | 0.534941 | 0.982671 | 0.046001 | 0.426013 |

Best variant is still the original `head384`.

## Interpretation

M712 rules out fixed banded compression.

Reserving budget for tail atoms reduces negative-only share slightly, but it
also removes too many useful head target atoms. The net target recall is worse
than pure head384.

This means the missed target atoms are not recoverable by a global rank-band
policy. They require query-specific or interaction-specific selection.

## Decision

Do not use fixed head+tail compression.

The current evidence chain is now:

1. M704: oracle compression works.
2. M710: source coverage is sufficient at cap1536.
3. M711: missed targets look weak under current scalar features.
4. M712: fixed tail sampling cannot recover them.

The next compiler should be query-specific and interaction-aware. The most
direct next audit is pair-impact labeling: for a bounded sample, measure which
candidate atoms actually improve dense-boundary pair margins under native
scoring or dot-product-equivalent pair contribution. If that label is
separable, train the compression model from those features. If not, move to
query-text/dense-root generated posting rather than candidate selection.

