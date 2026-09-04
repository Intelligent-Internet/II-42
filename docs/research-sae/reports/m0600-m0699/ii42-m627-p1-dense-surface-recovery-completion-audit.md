# M627 / P1 Dense-Surface Recovery Completion Audit

Status: M606/M607 objective satisfied; broader retrieval bottleneck remains.

## Scope

This audit closes the M606/M607 P1.2 dense-equivalent score-surface recovery
loop.  It does not claim that the final retrieval problem is solved.  It checks
whether the specific goal was completed:

- diagnose dense-vs-P1 first-stage score geometry,
- train or construct a stronger first-stage dense-equivalent surface,
- gate it before native replay,
- replay through the native DB/plugin path,
- rerun M604 scorer-gap audit if the dense gate passes,
- decide whether to continue first-stage training, reopen M605, or stop the
  current loss family.

## Requirement Evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Freeze P1-a0125 / M549U / M603 / M604 / M605, do not promote M605 | M606, M607, M622 reports freeze those baselines and reject M605/M605.2 as a component. | done |
| M606 dense-surface diagnostic | `docs/research-sae/reports/m0600-m0699/ii42-m606-p1-dense-surface-diagnostic-report.md`; JSON at `runs/m606_p1_dense_surface_diagnostic_v1/m606_p1_dense_surface_diagnostic.json`. | done |
| Locate initial failure mode | M606 shows P1-a0125 had poor dense top100 in P1 top100 (`0.421238`) and weak dense/P1 rank correlation (`0.344016`). | done |
| M607 first-stage dense-equivalent candidate | M607 capacity line identifies active512 root-identity sparse surface as the valid P1.2 candidate. | done |
| Keep BM25/qrels out of first-stage training | M607/M608 first-stage candidate is dense-root/active-support based; BM25/qrels are only used in downstream native evaluation. | done |
| Dense-only gate before native replay | Shared15 active512 dense-only gate reaches O@100 `0.94500`; active384 remains below gate. | done |
| Native DB/plugin replay after gate | Native shared15 replay artifacts under `runs/m608_p1p2_native_shared15_v1/` and `runs/m608_p1p3_native_shared15_aligned_dense_v1/`. | done |
| Updated M604 audit after gate | P1.2 audit at `runs/m608_p1p2_m604_scorer_gap_shared15_v1/`; P1.3 audit at `runs/m608_p1p3_m604_scorer_gap_shared15_v1/` and report `docs/research-sae/reports/m0600-m0699/ii42-m604-p1p3-native-scorer-gap-audit-report.md`. | done |
| Clear recommendation | M607/M608 promote P1.3-a010 as current frozen first-stage candidate; M612-M626 reject further same-family first-stage training. | done |

## Main Positive Result

The first-stage dense-equivalent surface did improve materially.

P1.3-a010 native shared15 macro, aligned dense reference:

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.3-a010 | 0.94166 | 0.92843 | 0.77881 | 0.70001 | 0.85426 | 0.87182 |

Compared with the older P1-a0125 native baseline:

| Metric | Delta |
| --- | ---: |
| CUB | +0.00435 |
| NDCG@10 | +0.03387 |
| MAP@100 | +0.03726 |
| R@100 | +0.01077 |
| MRR@20 | +0.03208 |

The large fix was not a residual MLP.  It was:

1. active support capacity moving to active512,
2. native signed-dot query emission that restores the signed sparse-dot score
   interface while keeping document postings compatible with the existing
   native lifecycle.

## What Remains Unsolved

M604 after P1.3 shows the remaining top100 bottleneck is still real:

| Category | P1.3-a010 |
| --- | ---: |
| top100 hit, qrels-weighted | 0.34266 |
| candidate present but under-ranked, qrels-weighted | 0.48835 |
| candidate miss, qrels-weighted | 0.16899 |

M615 explains why more dense-only first-stage training is not the main lever:

| Macro | Under-ranked dense-miss share |
| --- | ---: |
| Micro | 0.980678 |
| Dataset macro | 0.944688 |

The dominant remaining under-ranked positives are not aligned-dense top100
documents.  A dense-only teacher can polish residual dense-hit misses, but it
cannot teach the dominant dense-miss positives into top100.

## Stopped Lines

The following were tested and rejected or reduced to audit-only tools:

| Line | Decision |
| --- | --- |
| M607 residual output-head training | rejected: overlap/recall did not improve safely |
| M608 support weighting | rejected: qrels-facing movement without dense-overlap improvement |
| M612 coordinate-gain calibration | rejected: dense-overlap/selector regression |
| M613/M614 local packing and topK recall surrogate | rejected: coverage/overlap instability |
| M617/M619/M621 scorer/admission probes | rejected: effect size too small |
| M623/M624 weak auxiliary teacher | rejected: weak noisy labels |
| M625/M626 cross-encoder teacher | rejected for training: qrels-blind coverage too low |

## Final Decision For This Goal

The M606/M607 dense-equivalent recovery loop is complete.

Keep:

- `P1.3-a010` as the current frozen first-stage/native benchmark candidate.
- M605, M623-M626 as regression/audit tools only.
- native DB/plugin evaluation as the required benchmark route.

Do not continue:

- dense-only micro-training against aligned dense top100,
- coordinate-gain or local-packing output-head variants,
- direct cross-encoder teacher training,
- larger M605 scorer grids over the old feature family.

## Next Objective, Outside This Goal

The next useful project is no longer M606/M607.  It should be a new objective:

Design a native, global, non-dataset-specific top100 promotion model for
candidate-present dense-miss positives, with P1.3-a010 frozen as the candidate
pool and with explicit guards for NDCG@10/MRR@20.  This is a second-stage
retrieval objective, not first-stage dense-equivalence recovery.

If that new scorer cannot improve Recall@100/MAP@100 without damaging
NDCG@10/MRR@20 through the native DB/plugin path, the remaining route requires
a new retrieval teacher/objective rather than another P1 head tweak.

Concrete next-stage plan:

- `docs/research-sae/reports/m0600-m0699/ii42-m628-p1p3-global-top100-promotion-plan.md`
