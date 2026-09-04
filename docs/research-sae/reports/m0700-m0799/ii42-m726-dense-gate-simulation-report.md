# M726 Dense-Only Gate Simulation

## Question

M725 showed that global movement scale is not enough.  M726 asks a narrower
question: does a safe subset of M721b movement exist if movement is selected by
dense-equivalence signals instead of applying one global scale to every query?

This is a diagnostic replay, not a deployment-ready gate.  It reads prior
native per-query outputs and mixes `M721b` rows with `P1.3` rows only when a
gate passes.

Gate inputs are dense-side metrics only:

- dense overlap deltas at `@10/@50/@100/@256`
- query support cosine delta

Qrels-derived metrics are used only after selection to evaluate the mixed
result.

## Artifacts

- Script: `scripts/audit_m726_dense_gate_simulation.py`
- Output JSON: `runs/m726_dense_gate_simulation_v1/m726_summary.json`
- Output report: `runs/m726_dense_gate_simulation_v1/m726_report.md`

## Key Result

No nontrivial dense-only gate clears the strict full promotion gate.

The best near-pass is:

| Root | Gate | Accepted | O@100-gain accepted | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 | dO@100 | dCUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scale=0.005` | `o100_gain_dense_all_nonneg` | 0.180 | 0.945 | +0.000068 | +0.000001 | +0.000009 | +0.000000 | +0.001880 | -0.000004 |

This is not promotable because strict candidate upper bound is still negative.
But it is materially better than global scale:

- `scale=0.005 all_candidate`: MAP `-0.000079`, MRR `-0.000336`
- gated `o100_gain_dense_all_nonneg`: MAP `+0.000001`, MRR `+0.000000`

## Interpretation

M726 changes the route status from "global scale is exhausted" to "safe
movement appears to exist, but the current gate is still an oracle-style
post-hoc dense-overlap selector."

The result is not a first-stage promotion.  It is a useful mechanism result:

- pair-interaction movement is not purely harmful
- applying it to all queries is unsafe
- selecting only dense-safe O@100 movements almost removes the rank harm
- the remaining CUB regression is tiny but still violates the strict gate

This means the line is not a dead end yet, but the next work must learn or
derive a deployable confidence proxy.  Continuing with only global scale or
more epochs is not justified.

## Decision

Do not promote M726.

Keep M726 as a diagnostic milestone.  It proves that confidence selection is
the next meaningful question.  If a deployable confidence proxy cannot
reproduce the near-pass without using post-retrieval dense-overlap signals, the
pair-interaction route should stop and the work should return to a direct
dense-topK/rank-margin preserving compiler.

## Next Step

M727 should test a pre-retrieval confidence proxy using features available
before native retrieval:

- pair model score margin
- selected atom count
- selected target-share proxy from training records
- selected negative-only share proxy
- query support cosine/magnitude before retrieval
- movement norm
- top atom replacement count

Acceptance for M727 must remain strict:

- no dense overlap@100 regression
- no CUB regression
- no Recall@100 regression
- no MAP/MRR regression
- nontrivial accepted movement, not a zero-change gate
