# M1184 Added Query Atom Predictor Smoke

## Objective

M1183 found that useful tail-added query atom contribution is highly
concentrated at pair level. M1184 tests the next necessary condition:

Can those useful added atoms be predicted from the base query atom set under a
query-heldout split?

This is a cheap gate before any heavier posting-compiler training. It does not
modify the native index or claim ranking improvement.

## Method

The smoke test rebuilds pair-level tail-added atom contribution rows from the
M1177 supervision pairs:

- Base query atoms: `m1129`
- Tail query/doc atoms: `m1137`
- Pair sources:
  - `rank_teacher_positive`
  - `harm_penalty`
- Split: 5-fold query-heldout by `dataset::query_id`

Three predictors are compared:

1. `frequency`: global positive target atom prior.
2. `cooccurrence`: base-query-atom -> positive target atom cooccurrence.
3. `harm_penalized_cooccurrence`: cooccurrence minus harm-pair risk.

Metrics:

- `Hit`: predicted atoms intersect top target atoms.
- `Recovered`: share of positive expected contribution recovered.
- `Realized`: signed expected contribution from predicted atoms.
- `Unsafe`: fraction of rows where predicted movement is in the wrong
  expected direction.

## Main Run: Top3

- Output root: `runs/m1184_added_atom_predictor_smoke_v1`
- Config: `predict_k=3`, `target_k=3`, `harm_lambda=1.0`
- Records: `42510`
  - `rank_teacher_positive`: `18310`
  - `harm_penalty`: `24200`

| Method | Source | Hit | Recovered | Realized | Unsafe |
| --- | --- | ---: | ---: | ---: | ---: |
| `frequency` | `rank_teacher_positive` | 0.000 | 0.000 | 0.000000 | 0.000 |
| `frequency` | `harm_penalty` | 0.004 | 0.004 | 0.041398 | 0.000 |
| `cooccurrence` | `rank_teacher_positive` | 0.033 | 0.016 | 0.046664 | 0.004 |
| `cooccurrence` | `harm_penalty` | 0.049 | 0.047 | 0.057696 | 0.036 |
| `harm_penalized_cooccurrence` | `rank_teacher_positive` | 0.022 | 0.014 | 0.029449 | 0.004 |
| `harm_penalized_cooccurrence` | `harm_penalty` | 0.049 | 0.047 | 0.057696 | 0.036 |

Top3 cooccurrence has a real but too-small positive signal. It recovers only
1.6% of positive contribution. The harm-penalized version reduces positive
signal and does not reduce harm behavior.

## Width Checks

### Top5

- Output root: `runs/m1184_added_atom_predictor_smoke_k5_v1`

| Method | Source | Hit | Recovered | Realized | Unsafe |
| --- | --- | ---: | ---: | ---: | ---: |
| `cooccurrence` | `rank_teacher_positive` | 0.129 | 0.071 | 0.105760 | 0.035 |
| `cooccurrence` | `harm_penalty` | 0.058 | 0.057 | -0.043425 | 0.043 |
| `harm_penalized_cooccurrence` | `rank_teacher_positive` | 0.071 | 0.057 | 0.038313 | 0.025 |
| `harm_penalized_cooccurrence` | `harm_penalty` | 0.058 | 0.057 | -0.043425 | 0.043 |

### Top10

- Output root: `runs/m1184_added_atom_predictor_smoke_k10_v1`

| Method | Source | Hit | Recovered | Realized | Unsafe |
| --- | --- | ---: | ---: | ---: | ---: |
| `cooccurrence` | `rank_teacher_positive` | 0.145 | 0.080 | 0.099159 | 0.058 |
| `cooccurrence` | `harm_penalty` | 0.079 | 0.077 | -0.042917 | 0.063 |
| `harm_penalized_cooccurrence` | `rank_teacher_positive` | 0.085 | 0.066 | 0.032773 | 0.044 |
| `harm_penalized_cooccurrence` | `harm_penalty` | 0.079 | 0.077 | -0.042917 | 0.063 |

Increasing prediction width improves positive recovery but also increases
unsafe movement. The harm penalty is not a usable safety mechanism in this
simple feature space.

## Interpretation

M1184 is a useful negative gate:

- M1183 showed the target is compact once tail query atoms are known.
- M1184 shows the target is not recoverable well enough from base atom
  identity cooccurrence alone.
- A simple sparse cooccurrence compiler should not be scaled into a larger
  training run.
- The failure is not lack of `k`; wider predictions trade positive recovery for
  unsafe movement.

This does not invalidate the M1183 target. It says the input representation for
the compiler is too weak. The next attempt needs richer query context from the
dense root or text encoder state, not just old atom IDs.

## Next Step

M1185 should test whether the target atoms are predictable from dense-root
query features rather than base atom identity:

- Use the same query-heldout target rows.
- Build labels from top aligned `rank_teacher_positive` added atoms.
- Compare:
  - base atom cooccurrence baseline from M1184,
  - dense/root query vector nearest-centroid predictor,
  - small linear multilabel classifier if root features are available.
- Keep the same harm audit. Do not train a full posting compiler unless the
  predictor beats M1184 and does not increase unsafe harm movement.

Stop condition:

- If dense/root query features also fail to recover useful target atoms under
  query-heldout split, then M1137 tail-added atom targets are not directly
  learnable as a first-stage query compiler and the route should pivot back to
  retrieval-conditioned query-time policy.

## Artifacts

- `scripts/audit_m1184_added_atom_predictor_smoke.py`
- `runs/m1184_added_atom_predictor_smoke_v1/summary.md`
- `runs/m1184_added_atom_predictor_smoke_v1/added_atom_predictor_smoke.json`
- `runs/m1184_added_atom_predictor_smoke_k5_v1/summary.md`
- `runs/m1184_added_atom_predictor_smoke_k10_v1/summary.md`
