# M703 Expanded-Budget Native Feasibility

## Purpose

M701 proved that dense-boundary target atoms are visible under an expanded
qrels-free interface. M702 showed the current linear atom-admission model cannot
compress that interface back to top384.

M703 asks the next first-stage question: if we directly append expanded atoms
to the query and run the native semantic posting scorer, can the extra atoms
move M653 dense-boundary pairs in the right direction while preserving the
baseline head?

This is still not a retrieval/reranker experiment:

- no qrels objective
- no BM25 objective
- no learned gate
- no reranker
- frozen doc posting/index geometry

## Method

For each M653 dense-boundary query, M703 compares:

- baseline semantic native ranking from `P1.3 / M549U native signed-dot`
- expanded query rankings generated from `source384/head48/cap1536`

Each variant appends top candidate atoms with a fixed global scale:

- append counts: `384`, `768`, `1536`
- scales: `0.005`, `0.01`, `0.02`, `0.05`

Metric is pairwise dense-boundary success: whether the dense-positive document
outranks the false-P1 negative document under native semantic scoring. It also
reports top95/top100/top256 overlap with the baseline semantic ranking.

Outputs:

- JSON: `runs/m703_expanded_budget_native_feasibility_v1/m703_summary.json`
- Markdown: `runs/m703_expanded_budget_native_feasibility_v1/m703_report.md`

## Aggregate Result

| Variant | Pair success | Baseline success | Fixed | Regressed | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: |
| `append384_s0.02` | 0.543464 | 0.541176 | 32 | 25 | 0.960164 |
| `append384_s0.01` | 0.542157 | 0.541176 | 18 | 15 | 0.976441 |
| `append768_s0.02` | 0.542157 | 0.541176 | 27 | 24 | 0.959794 |
| `append1536_s0.005` | 0.541176 | 0.541176 | 2 | 2 | 0.995568 |
| `append1536_s0.01` | 0.541176 | 0.541176 | 6 | 6 | 0.990634 |
| `append1536_s0.02` | 0.540850 | 0.541176 | 15 | 16 | 0.982984 |
| `append384_s0.05` | 0.539869 | 0.541176 | 63 | 67 | 0.916343 |

## Interpretation

M703 finds a real but weak native movement signal.

The best aggregate variant, `append384_s0.02`, improves pair success from
`0.541176` to `0.543464`, with top95 overlap `0.960164`. However, fixed and
regressed pairs are close: `32` fixed versus `25` regressed.

This means raw append can move boundary pairs, but it is not a reliable
dense-preserving compiler objective. Larger scale damages the baseline head,
and larger append budgets do not produce stable gains.

## Decision

Do not start long compiler training from raw expanded append.

The current route remains alive, but the bottleneck has moved:

1. M701: target atoms are visible under expanded interface.
2. M702: independent linear atom admission is too weak.
3. M703: raw appended atoms can move native ranks, but the effect is small and
   not safe enough.

The next useful step is M704: a budgeted set-selection objective over the
expanded interface. It should optimize coverage of dense-boundary target atoms
while explicitly penalizing false-P1 negative atoms and protecting baseline
head overlap.

M704 should not be a long end-to-end compiler training run yet. It should first
prove that a selector can beat raw append on:

- pair success lift
- fixed/regressed ratio
- top95 overlap
- query atom budget

Only if M704 produces a stronger and safer native movement signal should we
launch deeper dense-preserving compiler training.
