# M1262 Precision-Head Recall-Tail Source

## Question

M1261 rejected movement acceptance as an objective label.  M1262 tests a
different source construction:

- use a learned high-precision CUB target head;
- fill the remaining budget with source_abs / signed-magnitude recall-tail
  atoms;
- audit target/harm frontier before any native replay.

This is meant to test whether the M1224 `logistic_top3` precision signal can
combine with the M1251/M1255 source_abs recall signal.

## Runs

Smoke:

- `runs/m1262_precision_head_recall_tail_source_smoke_v1/`
- initially exposed an interpretation bug: equal frontier was treated as
  improvement.  The script was fixed to require strict improvement.

Full `shared15`:

- JSON:
  `runs/m1262_precision_head_recall_tail_source_v1/m1262_precision_head_recall_tail_source.json`
- Markdown:
  `runs/m1262_precision_head_recall_tail_source_v1/m1262_precision_head_recall_tail_source.md`

## Full Shared15 Frontier

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | HarmQuery | PredCount |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `logistic_top3` | 0.3205 | 0.2338 | 0.0313 | 0.2025 | 0.2720 | 0.0380 | 2.999 |
| `logistic_top5` | 0.5187 | 0.2272 | 0.0297 | 0.1975 | 0.2802 | 0.0402 | 4.996 |
| `source_abs_top8` | 0.7793 | 0.2138 | 0.0298 | 0.1840 | 0.2928 | 0.0432 | 7.975 |
| `log2_source_fill8` | 0.7776 | 0.2133 | 0.0298 | 0.1835 | 0.2936 | 0.0432 | 7.975 |
| `log3_source_fill8` | 0.7759 | 0.2129 | 0.0299 | 0.1830 | 0.2936 | 0.0440 | 7.975 |
| `log4_source_fill8` | 0.7755 | 0.2128 | 0.0298 | 0.1830 | 0.2943 | 0.0440 | 7.975 |
| `logistic_top8` | 0.7568 | 0.2076 | 0.0298 | 0.1778 | 0.2966 | 0.0440 | 7.975 |
| `prior_x_source_top8` | 0.6856 | 0.1881 | 0.0298 | 0.1583 | 0.2914 | 0.0432 | 7.975 |

## Interpretation

The precision head is real, but it does not compose with source_abs into a
better top8 source.

Observed pattern:

- `logistic_top3/top5` have better precision/gap but too little recall.
- filling the remaining budget from source_abs does not preserve the precision
  benefit.
- every `log*_source_fill8` variant is slightly worse than `source_abs_top8` on
  target recall and gap.
- `prior_x_source_top8` is much worse, confirming that directly weighting by
  contrast strength is not the missing piece.

This means the learned precision head is useful as an analysis signal, but not
as a replacement or prefix for the current source_abs/reserve0 source.

## Decision

Do not run native replay for M1262.

Retain `source_abs_top8` / `reserve0_s1` as the cleanest source target:

- target recall: `0.7793`
- precision: `0.2138`
- harm precision: `0.0298`
- gap: `0.1840`

Stop the precision-head recall-tail hybrid branch.

## Next Step

The recent stop signals now rule out:

- movement threshold exception (`M1260`)
- movement accepted/rejected objective label (`M1261`)
- learned precision-head plus source_abs fill (`M1262`)
- direct contrast weighting over source_abs (`M1224`, reconfirmed here)

The retained core is:

- CUB-specific source_abs / reserve0 is the best source target;
- signed_sum/native scoring can improve macro but causes row instability;
- low-tail reserve has recall value but degrades source precision gap;
- current query-time features cannot safely choose the harmful/helpful cases.

The next valuable branch should change the scoring/objective used after source
selection, not the source selection itself.  A reasonable M1263 direction is:

> keep the source_abs/reserve0 atom set fixed, and learn/calibrate atom weights
> or score scaling under dense-equivalence and CUB/ranking constraints, rather
> than adding or replacing atoms.

This targets the current bottleneck more directly: source atoms are good enough,
but their native score geometry is still not row-safe.

## Artifacts

- Script: `scripts/audit_m1262_precision_head_recall_tail_source.py`
- Smoke JSON:
  `runs/m1262_precision_head_recall_tail_source_smoke_v1/m1262_precision_head_recall_tail_source.json`
- Full JSON:
  `runs/m1262_precision_head_recall_tail_source_v1/m1262_precision_head_recall_tail_source.json`
