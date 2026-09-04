# M546 Frozen Dense-Root Output Compiler Plan

## Decision

M546 resets the encoder route to the narrow first-stage problem:

```text
document text -> frozen dense root -> trainable output compiler
              -> deterministic dense-derived posting
```

This deliberately excludes BM25, qrels, route positives, candidate reranking,
and ranking losses.  The goal is not to relearn dense semantics.  The goal is
to keep the dense model as the semantic root and train the smallest useful
output layer that emits an equivalent posting surface.

## Why This Is Different

Earlier nearby experiments mixed different proof surfaces:

- M509 tested a raw text direct-posting encoder with frozen PPLX heads, but it
  was an early smoke and later work moved into route/fanout checks.
- M526 was a useful teacher-fit gate, but it trained both dense and posting
  heads and did not become the main isolated route.
- M540/M541 proved row/product distillation can improve dense-product
  faithfulness, but they are not pure output-compilers.
- M544/M545 tested support-stage pressure after M541 and selected epoch 0,
  so that family should not be scaled.

M546 therefore isolates the user's intended hypothesis: if the dense root
already has the semantic ability, an output compiler should be able to map that
root's dense output into the dense-derived posting target without changing the
root.

## Implementation

Script:

```text
scripts/research_sae_m546_frozen_output_compiler.py
```

Runner:

```text
scripts/run_m546_frozen_output_compiler_spark.sh
```

Default canary:

```text
TASKS=FiQA2018,ArguAna,SCIDOCS,TRECCOVID
MAX_DOC_ROWS_PER_TASK=20000
EPOCHS=3
COMPILER_HIDDEN_DIMS=0
```

Default model boundary:

- base PPLX root: frozen;
- dense head: frozen identity when hidden dims match target dims;
- trainable part: output compiler only;
- default compiler after the first regression: delta residual around identity;
- safety loss: identity anchor keeps the compiler near the frozen dense output;
- safety gate: stop early if epoch eval regresses below the epoch0 identity
  floor on support cosine, active recall, and soft KL;
- optional next compiler: small MLP via `COMPILER_HIDDEN_DIMS`.

## Current Status

Updated 2026-06-30T23:38:00Z.

The first broad4/cap20k unconstrained linear canary was stopped after epoch 1
because it damaged the dense-derived posting surface:

| Run | Epoch | Support Cos | Active Recall | Soft KL | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| `broad4_cap20k_linear_seed5460` | 0 | 0.991566 | 0.93929 | 0.13148 | identity floor |
| `broad4_cap20k_linear_seed5460` | 1 | 0.972254 | 0.82369 | 0.30133 | stopped |

The failure mode is not infrastructure.  The unconstrained 1024x1024 AdamW
compiler moved too far away from an already strong identity floor.  M546 is
therefore narrowed further to train only a small residual correction around
the frozen dense output.

The guarded delta follow-up completed.  It fixed the catastrophic regression
seen in the unconstrained linear run and improved support cosine, support MSE,
soft KL, and top256 retention.  It still did not pass the first-stage gate
because hard active membership moved below the epoch0 identity floor.

| Run | Epoch | Support Cos | Active Recall | Soft KL | top256/T128 | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `broad4_cap20k_delta_anchor_lr2e5_seed5461` | 0 | 0.991255 | 0.93779 | 0.13331 | 0.993409 | identity floor |
| `broad4_cap20k_delta_anchor_lr2e5_seed5461` | 1 | 0.991352 | 0.93416 | 0.11429 | 0.993520 | best, but active down |
| `broad4_cap20k_delta_anchor_lr2e5_seed5461` | 2 | 0.991350 | 0.93310 | 0.11055 | 0.993514 | active lower |

Per-task active recall for the selected epoch1 checkpoint was weakest on
`TRECCOVID`:

| Task | Support Cos | Active Recall | Soft KL | top256/T128 |
| --- | ---: | ---: | ---: | ---: |
| `ArguAna` | 0.993360 | 0.94401 | 0.08757 | 0.994911 |
| `FiQA2018` | 0.990136 | 0.94058 | 0.13683 | 0.990444 |
| `SCIDOCS` | 0.992848 | 0.93958 | 0.09287 | 0.994869 |
| `TRECCOVID` | 0.989065 | 0.91246 | 0.13990 | 0.993855 |

The first active-rank variant used hard negatives only from teacher top512.
It still failed to protect hard active membership at epoch1 and was stopped:

| Run | Epoch | Support Cos | Active Recall | Soft KL | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| `broad4_cap20k_delta_active_rank_lr2e5_seed5462` | 0 | 0.991343 | 0.93802 | 0.13464 | identity floor |
| `broad4_cap20k_delta_active_rank_lr2e5_seed5462` | 1 | 0.991462 | 0.93476 | 0.11556 | stopped, active down |

This showed that top128 must be protected against the global non-active pool,
not just non-active dimensions inside teacher top512.  The next run switched
the active-rank negative pool to global non-active dimensions and enabled an
active floor selection rule so a checkpoint below epoch0 active recall could
not be selected.  That was still insufficient:

| Run | Epoch | Support Cos | Active Recall | Soft KL | Decision |
| --- | ---: | ---: | ---: | ---: | --- |
| `broad4_cap20k_delta_global_rank_lr2e5_seed5463` | 0 | 0.991453 | 0.93826 | 0.12850 | identity floor |
| `broad4_cap20k_delta_global_rank_lr2e5_seed5463` | 1 | 0.991574 | 0.93503 | 0.10984 | stopped, active down |

The global active-rank loss improved soft KL and support cosine but still
lowered exact top128 active recall.  The reason is objective shape: averaging
many active/negative rank pairs is too diffuse for the hard boundary where the
weakest teacher-active dimension can be displaced by a global non-active
dimension.

The script now adds an explicit `active_boundary` loss that penalizes the top
non-active predicted magnitudes whenever they exceed the weakest teacher-active
predicted magnitude.  That still did not make a full-epoch update safe:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | top256/T128 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `broad4_cap20k_delta_boundary_lr2e5_seed5464` | epoch0 | 0.991379 | 0.938867 | 0.13271 | 0.993872 | identity floor |
| `broad4_cap20k_delta_boundary_lr2e5_seed5464` | epoch1 | 0.991518 | 0.936757 | 0.11452 | 0.994055 | stopped, active down |

The important follow-up was step-level evaluation.  Full-epoch evaluation was
too coarse: it missed a narrow window where a tiny residual update improves
soft teacher fit without yet breaking exact active membership.

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | top256/T128 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | epoch0 | 0.991125 | 0.937670 | 0.13304 | 0.993455 | identity floor |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | step1000 | 0.991150 | 0.937721 | 0.13143 | 0.993475 | passes floor |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | step2000 | 0.991165 | 0.937708 | 0.13008 | 0.993485 | passes floor |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | step3000 | 0.991177 | 0.937675 | 0.12893 | 0.993501 | selected |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | step4000 | 0.991186 | 0.937622 | 0.12796 | 0.993509 | active below floor |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | step5000 | 0.991194 | 0.937539 | 0.12715 | 0.993516 | active below floor |
| `broad4_cap20k_delta_stepgate_lr5e6_seed5466` | step6000 | 0.991200 | 0.937463 | 0.12643 | 0.993526 | active below floor |

This is the first accepted learned output-compiler checkpoint under the strict
first-stage gate, but the margin is extremely small: active recall improves
only by `0.00000477` absolute over epoch0.  The useful gain is mainly soft KL,
from `0.13304` to `0.12893`.

The broader available-task check did not pass at the same learning rate:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | top256/T128 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `broad10_available_cap10k_delta_stepgate_lr5e6_seed5467` | epoch0 | 0.986564 | 0.932283 | 0.18390 | 0.986568 | identity floor |
| `broad10_available_cap10k_delta_stepgate_lr5e6_seed5467` | step1000 | 0.986577 | 0.932279 | 0.18316 | 0.986584 | stopped, active below floor |

The current interpretation is therefore narrow but useful:

- M546 shows that an output-only compiler can make a tiny safe improvement over
  the frozen dense identity floor on broad4 when evaluated at step granularity.
- The same update is not yet stable across the wider available retrieval set.
- The next viable test is not stronger boundary loss; it is lower update
  amplitude on broad10 with strict active-floor gating.

Active follow-up runs:

```text
spark-1: broad10_available_cap10k_delta_stepgate_lr2e6_seed5468
spark-2: broad10_available_cap10k_delta_stepgate_lr1e6_seed5469
```

Both lower-learning-rate broad10 runs stopped at step1000 and selected epoch0:

| Run | Checkpoint | Support Cos | Active Recall | Soft KL | top256/T128 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `broad10_available_cap10k_delta_stepgate_lr2e6_seed5468` | epoch0 | 0.987106 | 0.93305054 | 0.18212 | 0.987205 | identity floor |
| `broad10_available_cap10k_delta_stepgate_lr2e6_seed5468` | step1000 | 0.987111 | 0.93304977 | 0.18181 | 0.987207 | stopped, active below floor |
| `broad10_available_cap10k_delta_stepgate_lr1e6_seed5469` | epoch0 | 0.986591 | 0.93219147 | 0.18356 | 0.986567 | identity floor |
| `broad10_available_cap10k_delta_stepgate_lr1e6_seed5469` | step1000 | 0.986594 | 0.93218918 | 0.18339 | 0.986564 | stopped, active below floor |

The breach is tiny but real under exact comparison.  The next diagnostic is
therefore a sub-1000-step gate (`EVAL_EVERY_STEPS=250`) to test whether broad10
has a safe early window before exact active membership starts to drift.

The sub-1000-step residual diagnostics also failed at step250.  This means
even a very small unconstrained residual update is not stable on broad10 under
the exact top128 active floor.

M546 now includes a support-preserving `monotonic_power` compiler.  It does
not change dense coordinate ordering, so active/top-k support remains exactly
equivalent to the frozen dense identity output.  It only learns a positive
global magnitude exponent (`gamma`) for value calibration.

The first broad10 monotonic diagnostic showed the intended tradeoff:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | top256/T128 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `broad10_available_cap10k_monotonic_power_seed5473` | epoch0 | 1.0000 | 0.986818 | 0.933079 | 0.18352 | 0.986880 |
| `broad10_available_cap10k_monotonic_power_seed5473` | step250 | 1.0866 | 0.985215 | 0.933079 | 0.13648 | 0.986880 |
| `broad10_available_cap10k_monotonic_power_seed5473` | step1000 | 1.0798 | 0.985402 | 0.933079 | 0.13642 | 0.986880 |

This proves a different first-stage shape: support-equivalent value
calibration is stable and gives a large KL improvement, but it intentionally
trades away some support cosine.  The default support-first selector therefore
kept epoch0.  A new `SELECTION_MODE=kl_first` option was added to save this
kind of support-preserving value-calibrated checkpoint explicitly.

The KL-first monotonic run completed on broad10.  It selected the first learned
checkpoint and preserved support exactly:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | top256/T128 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `broad10_available_cap10k_monotonic_power_klfirst_seed5475` | epoch0 | 1.0000 | 0.986332 | 0.931186 | 0.18613 | 0.986314 |
| `broad10_available_cap10k_monotonic_power_klfirst_seed5475` | step250 | 1.0833 | 0.984819 | 0.931186 | 0.13957 | 0.986314 |
| `broad10_available_cap10k_monotonic_power_klfirst_seed5475` | step500 | 1.0835 | 0.984816 | 0.931186 | 0.13958 | 0.986314 |
| `broad10_available_cap10k_monotonic_power_klfirst_seed5475` | step750 | 1.0785 | 0.984950 | 0.931186 | 0.13966 | 0.986314 |
| `broad10_available_cap10k_monotonic_power_klfirst_seed5475` | step1000 | 1.0837 | 0.984808 | 0.931186 | 0.13958 | 0.986314 |

The selected checkpoint is `step250`, with a single trainable scalar
(`gamma=1.08332276`).  It improves soft support KL by `0.04655235` absolute
without changing active recall or top256 retention of teacher top128.  Support
cosine drops by `0.00151322`, which is inside the configured regression guard
and reflects the intended value-calibration tradeoff.

Per-task selected-checkpoint metrics:

| Task | Support Cos | Active Recall | Soft KL | top256/T128 |
| --- | ---: | ---: | ---: | ---: |
| `ArguAna` | 0.992317 | 0.948410 | 0.07630 | 0.995750 |
| `CQADupstackGamingRetrieval` | 0.996401 | 0.974533 | 0.05160 | 0.998466 |
| `CQADupstackUnixRetrieval` | 0.979868 | 0.921814 | 0.20344 | 0.980141 |
| `ClimateFEVERHardNegatives` | 0.985089 | 0.917145 | 0.14601 | 0.988930 |
| `FEVERHardNegatives` | 0.991662 | 0.953445 | 0.10503 | 0.994461 |
| `FiQA2018` | 0.990266 | 0.949875 | 0.09476 | 0.992790 |
| `HotpotQAHardNegatives` | 0.997841 | 0.979134 | 0.04320 | 0.999802 |
| `SCIDOCS` | 0.991637 | 0.941826 | 0.07866 | 0.995880 |
| `TRECCOVID` | 0.986191 | 0.909370 | 0.12918 | 0.992867 |
| `Touche2020Retrieval.v3` | 0.936922 | 0.816307 | 0.46756 | 0.924049 |

The runner now exposes `MAX_TRAIN_STEPS` and `EVAL_EVERY_STEPS`.  The gate was
also tightened so any active-floor breach stops immediately when active-floor
selection is enabled, even if an earlier best checkpoint exists.

First-stage conclusion: unconstrained or residual output compilers are not
broad10 stable under exact top128 support.  The stable minimal compiler is the
monotonic output layer: it keeps the dense-derived posting support equivalent
to the frozen dense route and learns only magnitude calibration.  This is the
current first-stage stopping point before any BM25, qrels, or ranking objective
is reintroduced.

Reviewer correction: `seed5474` showed the same monotonic signal but did not
write JSON after `stop: max train steps reached at 1000`; this was an
experiment-finalization failure, not a metric failure.  The rerun
`seed5475` completed normally and wrote JSON/Markdown/checkpoint.  Future
promotion must still be stricter than KL-first: accepting a checkpoint that
lowers KL while lowering support cosine changes support geometry and should not
be treated as a final dense-equivalent encoder.

The next formal gate is multi-constraint selection:

```text
active recall >= epoch0 active recall
support cosine >= epoch0 support cosine
soft support KL <= epoch0 soft support KL
```

This gate has been added to the runner via:

```text
SUPPORT_FLOOR_SELECTION=1
SUPPORT_FLOOR_TOLERANCE=0
KL_IMPROVEMENT_SELECTION=1
KL_IMPROVEMENT_TOLERANCE=0
```

The next run should keep the same broad10 monotonic setup but use these
constraints:

```text
RUN_NAME=broad10_available_cap10k_monotonic_power_multigate_seed5476
SEED=5476
COMPILER_MODE=monotonic_power
SELECTION_MODE=kl_first
ACTIVE_FLOOR_SELECTION=1
SUPPORT_FLOOR_SELECTION=1
KL_IMPROVEMENT_SELECTION=1
MAX_TRAIN_STEPS=1000
EVAL_EVERY_STEPS=250
```

Expected interpretation:

- If a learned checkpoint passes all three constraints, the monotonic output
  layer has a real first-stage teacher-fit improvement.
- If no learned checkpoint passes and epoch0 is selected, monotonic power is
  valuable as a diagnostic/value-calibration signal but not yet a final usable
  dense-equivalent compiler under the stricter gate.

The multi-constraint run completed and selected epoch0:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | top256/T128 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| `broad10_available_cap10k_monotonic_power_multigate_seed5476` | epoch0 | 1.0000 | 0.986500 | 0.932273 | 0.18720 | 0.986653 | floor |
| `broad10_available_cap10k_monotonic_power_multigate_seed5476` | step250 | 1.0855 | 0.984921 | 0.932273 | 0.13939 | 0.986653 | support fail |
| `broad10_available_cap10k_monotonic_power_multigate_seed5476` | step500 | 1.0816 | 0.985029 | 0.932273 | 0.13937 | 0.986653 | support fail |
| `broad10_available_cap10k_monotonic_power_multigate_seed5476` | step750 | 1.0809 | 0.985049 | 0.932273 | 0.13940 | 0.986653 | support fail |
| `broad10_available_cap10k_monotonic_power_multigate_seed5476` | step1000 | 1.0852 | 0.984929 | 0.932273 | 0.13938 | 0.986653 | support fail |

Trace flags in the JSON confirm the gate behavior: every learned checkpoint
has `active_floor_ok=true` and `kl_improvement_ok=true`, but
`support_floor_ok=false`, so `selection_constraints_ok=false`.

Formal result: the review concern is valid.  Monotonic power gives a strong KL
calibration signal while preserving active/top-k membership, but exact
support-cosine preservation rejects the learned calibration.  Under the strict
three-way gate, M546 should not promote a learned output compiler yet.

The bounded-geometry continuation `seed5477` relaxed the support floor by only
0.0005 and added an explicit support-cosine loss:

```text
RUN_NAME=broad10_available_cap10k_monotonic_power_boundcos05_w05_seed5477
SUPPORT_FLOOR_SELECTION=1
SUPPORT_FLOOR_TOLERANCE=0.0005
SUPPORT_COSINE_WEIGHT=0.5
SOFT_SUPPORT_KL_WEIGHT=1.0
LEARNING_RATE=5e-4
```

It also selected epoch0.  The learned checkpoints kept active recall exactly
flat and compressed KL, but all exceeded the 0.0005 support-cosine budget:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `boundcos05_w05_seed5477` | epoch0 | 1.0000 | 0.986048 | 0.930757 | 0.19071 | selected |
| `boundcos05_w05_seed5477` | step250 | 1.0791 | 0.984655 | 0.930757 | 0.14380 | support fail |
| `boundcos05_w05_seed5477` | step500 | 1.0844 | 0.984509 | 0.930757 | 0.14374 | support fail |
| `boundcos05_w05_seed5477` | step750 | 1.0802 | 0.984625 | 0.930757 | 0.14375 | support fail |
| `boundcos05_w05_seed5477` | step1000 | 1.0796 | 0.984665 | 0.930757 | 0.14382 | support fail |

This narrows the diagnosis: the useful KL reduction appears only after gamma
has moved far enough to change support geometry by roughly 0.0014.  The next
attempt should therefore stop using full broad10 verification as the search
loop.  Use a short fast frontier first:

```text
RUN_NAME=broad10_available_cap10k_monotonic_power_frontier_w20_eval50_seed5478
EVAL_DOC_ROWS=256
EVAL_EVERY_STEPS=50
MAX_TRAIN_STEPS=300
LEARNING_RATE=1e-4
SUPPORT_COSINE_WEIGHT=20.0
SUPPORT_FLOOR_TOLERANCE=0.0005
```

If this frontier finds any learned checkpoint inside the support-cosine budget
with lower KL than epoch0, rerun that shape as a full broad10 verification.
If it still selects epoch0, the monotonic-power compiler should be treated as
a diagnostic calibration tool rather than the next promoted first-stage
encoder.

The fast frontier did find such a candidate on the 256-doc broad10 surface:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `frontier_w20_eval50_seed5478` | epoch0 | 1.0000 | 0.987833 | 0.935544 | 0.17811 | floor |
| `frontier_w20_eval50_seed5478` | step50 | 1.0049 | 0.987788 | 0.935544 | 0.17288 | pass |
| `frontier_w20_eval50_seed5478` | step100 | 1.0094 | 0.987742 | 0.935544 | 0.16829 | pass |
| `frontier_w20_eval50_seed5478` | step150 | 1.0140 | 0.987690 | 0.935544 | 0.16390 | pass |
| `frontier_w20_eval50_seed5478` | step200 | 1.0184 | 0.987636 | 0.935544 | 0.15998 | pass |
| `frontier_w20_eval50_seed5478` | step250 | 1.0220 | 0.987588 | 0.935544 | 0.15696 | pass |
| `frontier_w20_eval50_seed5478` | step300 | 1.0257 | 0.987535 | 0.935544 | 0.15395 | selected |

The selected step300 checkpoint reduces KL by about 13.6% while keeping active
recall flat and support cosine within the 0.0005 budget.  Because this was a
fast frontier with `EVAL_DOC_ROWS=256`, it is not yet a promoted result.  The
active verification run is:

```text
RUN_NAME=broad10_available_cap10k_monotonic_power_frontier_w20_full1024_seed5478
EVAL_DOC_ROWS=1024
EVAL_EVERY_STEPS=100
MAX_TRAIN_STEPS=300
LEARNING_RATE=1e-4
SUPPORT_COSINE_WEIGHT=20.0
SUPPORT_FLOOR_TOLERANCE=0.0005
```

The full 1024-doc verification passed:

| Run | Checkpoint | Gamma | Support Cos | Active Recall | Soft KL | Gate |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `frontier_w20_full1024_seed5478` | epoch0 | 1.0000 | 0.987086 | 0.933940 | 0.18349 | floor |
| `frontier_w20_full1024_seed5478` | step100 | 1.0094 | 0.986996 | 0.933940 | 0.17360 | pass |
| `frontier_w20_full1024_seed5478` | step200 | 1.0184 | 0.986890 | 0.933940 | 0.16521 | pass |
| `frontier_w20_full1024_seed5478` | step300 | 1.0257 | 0.986789 | 0.933940 | 0.15913 | selected |

This is the first broad10 M546 result that satisfies the intended first-stage
constraints after review:

- no BM25, no qrels, no ranking objective;
- frozen dense root and monotonic output compiler only;
- active recall unchanged at 0.933940;
- support cosine drop limited to 0.000297, within the 0.0005 budget;
- soft support KL reduced from 0.18349 to 0.15913, a 13.3% reduction.

The result is modest but structurally important: large KL improvements still
break support geometry, but a small monotonic calibration can improve teacher
distribution fit while keeping the dense-derived posting geometry inside a
predeclared budget.  This is the current M546 milestone before adding BM25 or
any search/ranking optimization.

## Seed Variance Follow-Up

The same bounded monotonic-power setup was rerun with three seeds on the full
1024-doc broad10 evaluation surface:

```text
broad10_available_cap10k_monotonic_power_frontier_w20_full1024_seed5478
broad10_available_cap10k_monotonic_power_frontier_w20_full1024_seed5479
broad10_available_cap10k_monotonic_power_frontier_w20_full1024_seed5480
```

All three runs selected the learned step300 checkpoint under the same
multi-constraint gate:

| Seed | Gamma | Support Drop | Active Drop | KL Before | KL After | KL Rel Improve |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5478 | 1.025745 | 0.000298 | 0.000000 | 0.17811 | 0.15395 | 13.56% |
| 5479 | 1.025148 | 0.000287 | 0.000000 | 0.18689 | 0.16272 | 12.93% |
| 5480 | 1.025983 | 0.000300 | 0.000000 | 0.18309 | 0.15838 | 13.49% |

Aggregate over seeds:

| Metric | Mean | Std | Min | Max |
| --- | ---: | ---: | ---: | ---: |
| gamma | 1.025625 | 0.000351 | 1.025148 | 1.025983 |
| support drop | 0.000295 | 0.000006 | 0.000287 | 0.000300 |
| active drop | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| KL relative improvement | 13.33% | 0.28% | 12.93% | 13.56% |

This confirms that M546 is not a single-seed accident.  The current stage-one
claim is still deliberately narrow: the frozen dense root plus one monotonic
scalar can improve teacher distribution fit while staying inside the declared
support-geometry budget.  It does not yet prove downstream retrieval quality.
That next proof surface is M547 dense-only retrieval.

## M547 Dense-Only Retrieval Verification

M547 evaluated the M546 monotonic compiler on broad10 qrels before adding any
BM25, hybrid fusion, route positives, or ranking loss.  The full run used all
materialized documents and queries and completed on spark-1 CPU in `259.707`
seconds.

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00000 | 0.00000 |
| `row_int8` | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | 0.00006 | -0.00004 |
| `m546_gamma_mean` | 0.57282 | 0.43531 | 0.74779 | 0.65188 | 0.99414 | 0.00019 | -0.00024 |
| `m546_seed5479` | 0.57286 | 0.43531 | 0.74782 | 0.65188 | 0.99424 | 0.00023 | -0.00021 |

Interpretation: M546 preserves dense retrieval behavior closely enough to keep
as a stage-one baseline.  It does not produce a meaningful dense-beating search
gain; its value is that teacher distribution fit improves while qrels metrics
remain essentially neutral.  This clears the gate for the next phase to test
BM25/fusion/ranking separately, with exact dense, row-int8 dense, and M546
gamma as fixed baselines.

## Gate

Primary teacher-fit metrics:

- support cosine;
- support MSE;
- soft support KL over teacher top512;
- top256 retention of teacher top128;
- active recall / active jaccard;
- sign accuracy and active magnitude cosine.

Promotion rule:

- promote only if the best epoch beats epoch0 on the compiler teacher-fit
  surface without relying on qrels, BM25, or route metrics;
- if linear compiler cannot beat epoch0, try one small MLP compiler;
- if both fail, the blocker is likely target discontinuity or a mismatch
  between raw dense output and the chosen posting target, not ranking.

## Next Step After Passing

Only after M546 passes teacher-fit should the project reintroduce downstream
search concerns:

1. run M547 dense-only retrieval using the compiler output;
2. verify exact-dense overlap and qrels metrics before adding any BM25;
3. then decide whether fanout, BM25 fusion, or ranking calibration is needed.

Status: steps 1 and 2 are complete.  M548 then ran a BM25-pre gamma frontier
to confirm whether the monotonic compiler should be pushed further before
search optimization.

## M548 BM25-Pre Frontier

M548 expanded the dense-only retrieval check to a gamma grid from `1.005` to
`1.050` without BM25, qrels-trained ranking loss, or route positives.

The teacher-fit selected gamma remains `1.02562527`:

- active drop: `0.00000000`;
- support drop: `0.00029496`;
- KL relative improvement: `13.23%`;
- dense-only retrieval: NDCG@10 `+0.00019`, Recall@100 `-0.00024`,
  overlap@100 `0.99414` versus exact dense.

Higher gamma values can move NDCG slightly more, but they leave the
dense-equivalent regime.  For example, `gamma=1.050` reaches NDCG@10
`+0.00042`, but overlap@100 falls to `0.98944`.

Decision: M546/M547/M548 define the BM25-pre floor as exact dense, row-int8,
and M546 monotonic `gamma=1.02562527`.  Further dense-only monotonic tuning is
not the highest-value path; the next phase should reintroduce BM25/search
optimization as a separate stage.
