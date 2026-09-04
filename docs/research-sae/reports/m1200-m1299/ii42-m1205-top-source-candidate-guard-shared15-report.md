# M1205 Top-Source Candidate Guard Shared15

M1205 tests whether the revived `top3` source from M1204 can be made safe by a
qrels-free top1000 overlap gate.  The policy is:

- use aggressive `top3` posting delta when candidate-set overlap is high;
- otherwise fall back to conservative `top1`, not baseline.

## Setup

- Surface: shared15 native replay
- Query count: 1342
- Gate feature: baseline-vs-aggressive top1000 overlap
- Thresholds: 0.98, 0.99, 0.995, 0.999
- Baseline: frozen P1 native query

## Macro Result

| Variant | Take | Macro negative metrics | Dataset negative cells | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `aggressive` | 1.000 | 1 | 10 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `conservative` | 1.000 | 0 | 11 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |
| `gated_aggressive_to_conservative_t0.98` | 0.540 | 1 | 9 | +0.000677 | +0.002634 | +0.001898 | +0.002399 | -0.000029 |
| `gated_aggressive_to_conservative_t0.99` | 0.121 | 0 | 11 | +0.000229 | +0.002644 | +0.001731 | +0.002653 | +0.000073 |
| `gated_aggressive_to_conservative_t0.995` | 0.027 | 0 | 10 | +0.000311 | +0.002177 | +0.001564 | +0.002424 | +0.000018 |

## Interpretation

`gated_aggressive_to_conservative_t0.99` is the best deployable-looking point
in this branch so far.  It has all five macro deltas positive and beats the
M1201/M1202 consensus branch by a large margin on MAP/NDCG/MRR while keeping
CUB positive.

This is not final.  The t0.99 policy still has 11 dataset-level negative metric
cells, mostly inherited from the conservative fallback on `cqadupstack`,
`fiqa`, `scidocs`, `trec-covid`, and `webis-touche2020`.  The macro gain is
real, but the row-level safety problem remains.

## Decision

- Promote `top3 -> top1` candidate-boundary gating as the new active branch.
- Keep `gated_aggressive_to_conservative_t0.99` as the current best macro-safe
  candidate.
- Do not call it final until row-level harm is reduced and broader/native
  engineering surfaces confirm the gain.

## Next Step

The next experiment should train or audit a row-local risk policy on top of the
`top3 -> top1` source pair.  The target is to keep the M1205 macro gain while
reducing dataset negative cells below the M1202 learned guard level.

## Artifacts

- JSON: `runs/m1205_top_source_candidate_guard_shared15_v1/m1189_context_delta_overlap_gate.json`
- Markdown: `runs/m1205_top_source_candidate_guard_shared15_v1/m1189_context_delta_overlap_gate.md`
- Script: `scripts/audit_m1189_context_delta_overlap_gate.py`
