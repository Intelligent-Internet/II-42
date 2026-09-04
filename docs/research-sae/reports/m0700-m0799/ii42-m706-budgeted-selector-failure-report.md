# M706 Budgeted Selector Failure Attribution

M706 diagnoses why M705 did not recover the M704 budgeted set-selection oracle.

It does not run another native ranking sweep. It compares the learned selector
against the oracle target/risk structure on the same expanded candidate
interface.

## Context

M704 proved the route is alive:

- expanded interface: `source384`, `doc_atom_head=48`, `cap1536`
- best strict oracle: `safe_fill_delta_target_b384_s0.02`
- pair success: `0.541176 -> 0.617647`
- fixed/regressed: `234/0`
- top95 overlap: `0.967023`

M705 tested a simple trainable selector/regressor. It did not pass eval gate:

- best eval variant: `learned_predicted_b192_s0.01`
- pair success: `0.561041 -> 0.563734`
- fixed/regressed: `12/9`
- top95 overlap: `0.959625`

M706 asks whether the learned selector fails because target atoms are not
available, because signs are wrong, or because the selector does not rank target
atoms into the budget.

## Run

```bash
python3 scripts/audit_m706_budgeted_selector_failure.py \
    --datasets arguana,cqadupstack,fiqa,scidocs \
    --max-atom-train-rows 0 \
    --budgets 24,48,96,192,384 \
    --output-root runs/m706_budgeted_selector_failure_v1
```

Output:

- `runs/m706_budgeted_selector_failure_v1/m706_summary.json`
- `runs/m706_budgeted_selector_failure_v1/m706_report.md`

## Eval Attribution

| Budget | Target visibility | Learned target recall | Oracle target recall | Learned negative-only | Oracle overlap |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 24 | 0.982671 | 0.153701 | 0.635901 | 0.194635 | 0.266553 |
| 48 | 0.982671 | 0.241853 | 0.935958 | 0.163670 | 0.466182 |
| 96 | 0.982671 | 0.365982 | 0.982671 | 0.124715 | 0.721176 |
| 192 | 0.982671 | 0.518742 | 0.982671 | 0.091039 | 0.845427 |
| 384 | 0.982671 | 0.701639 | 0.982671 | 0.062429 | 0.851830 |

Sign agreement is not the blocker:

- candidate sign match: `1.0`
- predicted sign match: `1.0`

Candidate visibility is also not the blocker:

- target visibility on eval: `0.982671`

The blocker is target recovery inside the learned budget:

- at budget 384, learned target recall is `0.701639`
- oracle target recall is `0.982671`
- gap is `0.281032`

## Interpretation

M705 is not failing because the expanded interface lacks the required atoms.
The atoms are mostly present.

M705 is not failing because impact signs are systematically wrong. For selected
target atoms, both candidate and predicted signs match the target direction.

M705 fails because the global pointwise selector does not rank enough target
atoms into the budget. It also selects non-trivial negative-only atoms,
especially at smaller budgets.

This explains the M705 native result:

- safer small variants preserve top95 but do not move boundary pairs;
- larger variants recover more targets but still miss too many and introduce
  regressions;
- the model has weak train/all movement but no reliable eval gate.

## Decision

Do not scale M705 directly.

The next version should not be "more steps" on the same pointwise classifier.
The repair should be architectural/objective-level:

1. Query-conditioned selector: include query-level context and per-query
   normalization so target atoms are ranked relative to that query's candidate
   set.
2. Setwise selector: optimize coverage/diversity of target-like atoms under a
   fixed budget, instead of independently scoring atoms.
3. Pair-impact supervision: train on whether selecting an atom fixes or
   regresses dense-boundary pairs, not only whether the atom appears in the
   target set.
4. Hard negative handling: explicitly separate target atoms from negative-only
   atoms that look similar under current pointwise features.

M704 remains the positive upper-bound signal. M706 narrows the missing
component: recover the oracle's target coverage in a support-safe, trainable
way.

