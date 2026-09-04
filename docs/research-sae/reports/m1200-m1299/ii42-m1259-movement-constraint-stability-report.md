# M1259 Movement Constraint Stability Audit

## Question

M1258 found a qrels-free native document-movement constraint that improves the
macro frontier over both:

- true `reserve0_s1`
- unconstrained `reserve1_s1`

M1259 checks whether the best M1258 policy is stable enough to promote to
broader/native validation, or whether it should remain a diagnostic/source
signal.

Policy under test:

- start from `reserve0_s1`
- add the single low-tail reserve atom only when:
  - `top10_new == 0`
  - `top100_new <= 2`

This is a bounded movement-source test, not another arbitrary guard sweep.

## Runs

Smoke:

- `runs/m1259_movement_constraint_stability_smoke_v1/`
- datasets: `cqadupstack`, `scidocs`, `webis-touche2020`

Full `shared15`:

- JSON:
  `runs/m1259_movement_constraint_stability_v1/m1259_movement_constraint_stability.json`
- Markdown:
  `runs/m1259_movement_constraint_stability_v1/m1259_movement_constraint_stability.md`
- query count: `1342`

## Full Shared15 Macro

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `reserve0_s1` | 7.975 | 0 | +0.001399 | +0.003336 | +0.003626 | +0.003127 | +0.000115 | +0.030620 |
| `reserve1_s1` | 8.958 | 0 | +0.001562 | +0.003536 | +0.003268 | +0.002995 | +0.000092 | +0.031034 |
| `no_top10_or_large100_s1` | 8.835 | 0 | +0.001566 | +0.003491 | +0.003663 | +0.003016 | +0.000155 | +0.031815 |

Macro still favors the movement-constrained policy.

## Query-Level Pairwise Shape

| Left | Right | Wins | Losses | Ties | MeanScoreDelta |
| --- | --- | ---: | ---: | ---: | ---: |
| `constrained` | `reserve0` | 97 | 144 | 1101 | -0.004245 |
| `constrained` | `reserve1` | 38 | 17 | 1287 | -0.005525 |
| `reserve1` | `reserve0` | 111 | 185 | 1046 | -0.016605 |

This is the important correction to the M1258 macro result:

- the constraint reduces reserve1 damage count
- but it also blocks a small number of large positive reserve1 movements
- therefore average query-level score remains negative against reserve1

## Stability Findings

The constraint is useful but not mature enough for broader promotion.

Evidence:

- It improves macro score over both adjacent baselines.
- It reduces reserve1-vs-reserve0 loss count from `185` to `144`.
- It accepts `1177/1342` queries (`87.70%`).
- It still has row-level losses against reserve0 on datasets including
  `cqadupstack`, `dbpedia-entity`, `fiqa`, `nfcorpus`, `trec-covid`, and
  `webis-touche2020`.
- Against reserve1, the main losses are rejected movements that were actually
  helpful, especially in `scidocs`, `dbpedia-entity`, `nfcorpus`, and `msmarco`.

The worst rejected-help cases have exactly the movement patterns the constraint
was designed to block:

- `top10_new == 1`
- or `top100_new >= 3`

So large movement is not purely harmful.  It is high variance: sometimes damage,
sometimes high-value rescue.

## Decision

Do not promote M1258/M1259 directly to broader/native engineering validation.

Keep `no_top10_or_large100_s1` as:

- the best current macro-safe movement-aware source-construction candidate
- evidence that native document movement contains useful qrels-free structure
- a design input for the next objective/source construction step

Do not treat it as:

- a row-safe default
- a final gate
- a reason to continue threshold/grid tweaking

## Next Step

The next branch should not be another movement-threshold variant.

M1260 should convert the retained signals into a source/objective design:

1. keep `signed_sum_s1` as the query-time delta signal;
2. keep CUB-specific teacher/action-source structure as the target surface;
3. include movement-risk features as constraints, not hard post-hoc filters;
4. explicitly model or construct exceptions for high-value large movement;
5. require target/harm separability before native replay.

The key new hypothesis is:

> Native movement context is useful, but hard movement vetoes are too blunt.
> The next route must preserve high-value large movements while suppressing
> over-expansion damage.

## Artifacts

- Script: `scripts/audit_m1259_movement_constraint_stability.py`
- Smoke JSON:
  `runs/m1259_movement_constraint_stability_smoke_v1/m1259_movement_constraint_stability.json`
- Full JSON:
  `runs/m1259_movement_constraint_stability_v1/m1259_movement_constraint_stability.json`
