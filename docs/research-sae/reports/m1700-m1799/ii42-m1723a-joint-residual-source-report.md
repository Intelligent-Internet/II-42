# M1723A Joint Residual Source Report

## Decision

**Retain the joint residual source as a real positive mechanism signal, but do
not scale the current objective unchanged.** The residual arm clearly exceeds
the matched dense-control arm and improves every source-admission metric. Its
best quality checkpoint exceeds the fixed posting-read ceiling, while the
best cost-safe checkpoint narrowly misses two quality thresholds.

This differs materially from M1600 and M1722:

- M1600 joint training regressed the heldout source;
- M1722 changed only the teacher over a frozen source and captured below
  `0.34%` of the oracle gap;
- M1723 changes the source geometry and uses the runtime-matched asymmetric
  `1 document key / 8 query probes` forward surface.

The result proves that lexical-residual source co-design is learnable. The
remaining blocker is query-selected posting cost, not source capacity, query
observability, or maximum DF.

## Controlled Canary

- train/validation: M1600 4,000/500 disjoint pools;
- documents: 35,831 train, 4,498 validation;
- initial source: M1600 8x512 codebook;
- BM25: frozen `k1=0.9`, `b=0.4`, top256;
- query/document runtime: 8 probes/group and 1 key/group;
- candidate rows: identical between control and residual arms;
- training: 2,000 updates, batch 16, 128 candidates, 96 teacher docs;
- learning rate `1e-4`, seed 1723;
- qrels, pair labels, cross-encoder scores, and per-query tuning: unused.

ClearML task: `d07b31926313462594cd4f6d0d9713b5`.

The initial source is:

| O@100 | O@256 | Residual R@100 | Residual R@256 | Reads | Max DF |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.815620 | 0.664820 | 0.734089 | 0.564892 | 0.155242x | 0.012228 |

## Matched Dense Control

The dense-control arm never produced a checkpoint that improved all protected
metrics simultaneously. Its strongest local quality point at step 400 was:

| O@100 | O@256 | Residual R@100 | Reads | Max DF |
| ---: | ---: | ---: | ---: | ---: |
| 0.815260 | 0.671703 | 0.734887 | below 0.18x | 0.012672 |

O@100 remains below initialization, so the arm correctly selects epoch zero.
The residual result therefore cannot be attributed only to the asymmetric
forward shape or extra source capacity.

## Residual Frontier

| Step | O@100 | O@256 | R@100 | R@256 | Reads | Max DF | Interpretation |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 300 | 0.820400 | 0.679617 | 0.740343 | 0.584535 | 0.174202x | 0.012228 | cost-safe early gain |
| 400 | 0.830340 | 0.697266 | 0.755935 | 0.608801 | 0.191940x | best quality, cost fail |
| 500 | 0.829140 | 0.696164 | 0.755708 | 0.608952 | 0.187651x | cost fail |
| 700 | 0.819540 | 0.682297 | 0.743234 | 0.592482 | 0.175205x | selected cost-safe |
| 1,500 | 0.810980 | 0.670219 | 0.729338 | 0.577091 | 0.168223x | depth regression |
| 1,900 | 0.819720 | 0.687781 | 0.743296 | 0.599130 | 0.182283x | max DF 0.051134 fail |

At step 400, candidate-minus-base deltas are:

- O@100 `+0.014720`;
- O@256 `+0.032445`;
- residual recovery@100 `+0.021846`;
- residual recovery@256 `+0.043909`.

These are substantially larger than the M1722 frozen-router deltas and are
not a loss-only effect. However, reads rise to `0.191940x`, above the `0.18x`
gate.

The selected cost-safe step 700 remains strongly above the dense control, but
its O@100 gain is `+0.003920` and R@100 gain is `+0.009145`. The latter misses
the threshold by only `0.000855`; the former is the material miss. O@256 and
R@256 pass comfortably.

## Failure Mechanics

Maximum DF is not the blocker at the quality optimum: step 400 max DF is only
`0.014006`. Document-side balance and DF-FLOPS constraints therefore work.
The excess arises because query probes become correlated with above-average
DF keys. Eight groups x eight probes read `0.191940x` even though no single
key violates the DF floor.

The training objective controls document marginal DF but contains no term for
the joint quantity actually paid at runtime:

```text
expected reads(q) = sum over selected query keys of document_df(key)
```

This is the one justified structural repair. It is not a generic sparsity
weight or a query-probe sweep.

## Next Decision

Authorize M1724 as one fixed query-cost-aware rerun:

- retain the exact M1723 architecture, data, targets, initialization, probes,
  and quality gates;
- estimate query-selected posting reads from straight-through query probes and
  straight-through document marginals in each batch;
- penalize only excess above `0.18x`;
- use one predeclared coefficient, no sweep;
- keep the equal-capacity dense control.

If M1724 cannot retain the step-400 quality while meeting cost, stop this joint
objective rather than changing probe count or relaxing the product budget.

## Reproducibility

- host: `spark-1`;
- formal run:
  `/home/huoju/leask/runs/ii42-m1723-joint-residual-v1/runs/m1723a-joint-residual-source4k-seed1723-v2`;
- failed pre-training materializer run: `v1`, excluded;
- summary SHA-256:
  `4e2c00f9d68cae941a68f794b0f652edb35723e67102a8d57c9d9b2aa256e629`;
- local result: `ii42-m1723a-joint-residual-source-result.json`.
