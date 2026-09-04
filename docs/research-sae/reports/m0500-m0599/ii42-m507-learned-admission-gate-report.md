# II-42 M507 Learned Admission Gate Report

## Summary

M507 tested whether the PPLX-root posting route can move beyond hand-written
fanout thresholds by learning a qrels-free candidate admission/rerank gate from
dense-teacher scores.

Result: the route still has signal, but the low-cost learned-admission version
is not ready.  A full-feature gate can slightly beat both M506d hand-count and
fixed p768 on Broad10 sampled, but it needs p768 candidate generation and full
route features.  The runtime-valid support-only admission gate fails badly, and
distilling the full gate into support-only features does not recover it.

Decision: stop the current linear/support-only learned-admission subline.  Do
not keep tuning thresholds or linear gates.  The only remaining worthwhile
direction is an architecturally stronger low-cost scorer/admission model, or
accepting p768/full-feature cost as an engineering tradeoff.

## Experiments

### M507

- Train existing `rotation_residual` support head.
- Generate p768 candidate pool.
- Fit two qrels-free linear ridge gates to dense-teacher score shape:
  - `support_admit_hand_budget`: support-only features, runtime-valid but weak.
  - `full_admit_hand_budget`: full route features, diagnostic upper point.
- Heldout qrels are used only for reporting.

### M507b

- Distill the full-feature gate output into the support-only feature space.
- Goal: recover full-gate selection quality without requiring full route
  scoring over p768.

## Broad10 Sampled Matrix

Source file:
`outputs/m507/broad10_seed5070_sampled/m507_broad10_seed5070_sampled.json`

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch | Generated |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.57712 | 0.74939 | 0.64108 | 0.41715 | 1.00000 | 1.00000 | 1.00000 | 0.00000 |
| `exact_materialized_dense` | 0.57617 | 0.74934 | 0.63895 | 0.41537 | 0.99648 | 1.00000 | 1.00000 | 0.00000 |
| `m507_rotation_residual_full_admit_hand_budget` | 0.55028 | 0.72574 | 0.61628 | 0.38211 | 0.79480 | 0.90616 | 0.47537 | 0.64702 |
| `m507_rotation_residual_full_rerank_p768` | 0.55028 | 0.72574 | 0.61628 | 0.38211 | 0.79480 | 0.90616 | 0.64702 | 0.64702 |
| `m507_rotation_residual_fixed_p768` | 0.54942 | 0.72510 | 0.61559 | 0.38162 | 0.80072 | 0.90616 | 0.64702 | 0.00000 |
| `m507_rotation_residual_hand_count` | 0.54819 | 0.71139 | 0.61373 | 0.37631 | 0.76832 | 0.84620 | 0.47537 | 0.00000 |
| `m507_rotation_residual_fixed_p512` | 0.53281 | 0.71942 | 0.60835 | 0.37146 | 0.76292 | 0.84796 | 0.55225 | 0.00000 |
| `m507_rotation_residual_support_admit_hand_budget` | 0.51230 | 0.67456 | 0.59808 | 0.34873 | 0.52388 | 0.56684 | 0.47537 | 0.64702 |
| `m507_rotation_residual_fixed_p256` | 0.49832 | 0.65725 | 0.58434 | 0.35252 | 0.66436 | 0.71580 | 0.39452 | 0.00000 |
| `m507_structural_fixed_p256` | 0.44622 | 0.60224 | 0.53422 | 0.29914 | 0.56520 | 0.59536 | 0.41076 | 0.00000 |

## M507b Distillation Gate

Source file:
`outputs/m507/broad4_seed5071_distill/m507b_broad4_seed5071_distill.json`

| Source | NDCG@10 | Dense O@100 | Cand R@100 | Touch | Generated |
| --- | ---: | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | 0.49689 | 1.00000 | 1.00000 | 1.00000 | 0.00000 |
| `m507_rotation_residual_fixed_p768` | 0.48387 | 0.83730 | 0.93640 | 0.75733 | 0.00000 |
| `m507_rotation_residual_hand_count` | 0.47949 | 0.81820 | 0.89610 | 0.59339 | 0.00000 |
| `m507_rotation_residual_full_admit_hand_budget` | 0.47708 | 0.82260 | 0.93640 | 0.59339 | 0.75733 |
| `m507_rotation_residual_support_admit_hand_budget` | 0.46558 | 0.63030 | 0.67080 | 0.59339 | 0.75733 |
| `m507_rotation_residual_support_distill_admit_hand_budget` | 0.46558 | 0.63030 | 0.67080 | 0.59339 | 0.75733 |

Distillation did not improve the support-only gate.  It reproduced the weak
support admission behavior instead of the full gate behavior.

## Interpretation

The full gate result is important: dense-teacher score shape can improve the
ranking/admission surface over fixed p768 and M506d hand-count in the Broad10
sample.  That means the PPLX-root route still has usable ranking signal.

But this is not a clean runtime win:

- `full_admit_hand_budget` reports Touch `0.47537`, but it still needs a p768
  generated pool at `0.64702` and full route features to score that pool.
- `support_admit_hand_budget` is the low-cost version, but it collapses to
  NDCG@10 `0.51230`, with Dense O@100 `0.52388`.
- `support_distill_admit_hand_budget` does not recover the full gate.

So the current bottleneck is not "can dense supervision rank candidates"; it
can.  The bottleneck is "can low-cost posting/support features identify the
same candidates before full route scoring"; currently they cannot.

## Stop Rule

Stop the current subline:

- no more hand threshold sweeps;
- no more linear support-only gates;
- no more support-only distillation with the current feature set;
- no RL until a low-cost candidate/admission representation can preserve dense
  teacher candidates.

The route is still useful only under one of these conditions:

1. accept p768/full-feature scoring cost and use M507 full-gate rerank as an
   engineering option;
2. build a stronger low-cost scorer that sees richer sparse interactions than
   aggregate support dot/rank features;
3. move back to the encoder/posting-head architecture and train it to emit a
   candidate surface where p256/p512 already contains the full-gate winners.

## Recommended Next Step

If continuing this research line, the next experiment should not be another
posthoc gate.  It should be a model-side change:

- train the support head with an explicit objective to preserve full-gate
  winners under p256/p512;
- or train a small interaction scorer over sparse coordinate intersections,
  not just aggregate support dot products;
- or treat p768 as the required candidate-generation budget and optimize
  serving/runtime around that reality.

For now, M506d hand-count remains the default low-cost dynamic policy.  M507
full-gate is a diagnostic upper point, not the default serving path.
