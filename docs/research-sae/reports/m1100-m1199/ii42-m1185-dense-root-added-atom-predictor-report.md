# M1185 Dense/Root Added Atom Predictor

## Objective

M1184 showed that M1183 tail-added query atom targets are not recoverable from
base atom identity/cooccurrence alone. M1185 tests whether richer first-stage
query-only inputs can predict the same targets:

- dense/root query vector;
- P1 query posting vector;
- dense/root + P1 concatenation.

This is still only a predictability gate. It does not modify the native index
and does not claim ranking improvement.

## Method

The target is the same M1177/M1183/M1184 supervision:

- `rank_teacher_positive`: atoms whose tail-added query contribution moves
  positive pairs in the teacher direction;
- `harm_penalty`: atoms whose contribution would move harm pairs in the wrong
  direction.

Rows are split by query-heldout 5-fold assignment. The model sees only query
features, so every pair sharing a query receives the same predicted atom set.
This matches a deployable first-stage query compiler more closely than a
pair-specific oracle.

Predictors:

- `frequency`: global target atom prior;
- `centroid`: nearest weighted target-atom centroid in query feature space;
- `ridge`: multi-output ridge from query feature to target atom score.

Feature modes:

- `root`;
- `p1`;
- `root_p1`.

## Main Result

### Top5

- Output root: `runs/m1185_dense_root_added_atom_predictor_v1`
- Config: `predict_k=5`, `target_k=5`
- Records: `42510`
- Query count: `116`

Best method: `root_ridge_pos`

| Method | Pos recovered | Pos realized | Pos unsafe | Harm realized | Harm unsafe |
| --- | ---: | ---: | ---: | ---: | ---: |
| `root_ridge_pos` | 0.063 | 0.059574 | 0.024 | -0.043022 | 0.060 |
| `root_p1_ridge_pos` | 0.066 | 0.059093 | 0.043 | -0.043411 | 0.060 |
| `p1_ridge_pos` | 0.056 | 0.035188 | 0.042 | -0.043411 | 0.060 |

Top5 does not beat M1184's base-atom cooccurrence recovery (`0.071`). It is
more conservative for positive unsafe movement in the best root-only ridge
case, but harm unsafe is still worse than desired.

### Top3

- Output root: `runs/m1185_dense_root_added_atom_predictor_k3_v1`
- Best family: centroid

| Method | Pos recovered | Pos realized | Pos unsafe | Harm realized | Harm unsafe |
| --- | ---: | ---: | ---: | ---: | ---: |
| `root_centroid_pos` | 0.060 | 0.081500 | 0.053 | -0.002302 | 0.070 |

Top3 is not competitive with M1183's compact target. It confirms that dense/root
features are not enough if prediction width is kept very small.

### Top10

- Output root: `runs/m1185_dense_root_added_atom_predictor_k10_v1`
- Best family: centroid

| Method | Pos recovered | Pos realized | Pos unsafe | Harm realized | Harm unsafe |
| --- | ---: | ---: | ---: | ---: | ---: |
| `root_centroid_pos` | 0.115 | 0.135250 | 0.072 | -0.043797 | 0.112 |
| `p1_centroid_pos` | 0.114 | 0.144193 | 0.066 | -0.045458 | 0.124 |
| `root_ridge_pos` | 0.084 | 0.065609 | 0.069 | -0.041554 | 0.099 |

Top10 is the first positive signal after M1183: root centroid recovery reaches
11.5%, which is above M1184 Top10 cooccurrence recovery (`0.080`). But the same
move increases harm unsafe to 11.2%. This is not deployable and should not be
scaled directly.

## Interpretation

M1185 changes the diagnosis:

1. The M1183 target is not purely unlearnable. Dense/root query features recover
   more target mass than base atom cooccurrence at wider prediction width.
2. The useful signal is not safe by default. The same centroid movement also
   predicts harmful atoms too often.
3. This means the next bottleneck is safety/confidence selection, not model
   capacity or a larger blind training run.

The result supports one more small experiment:

- Do not train a full posting compiler yet.
- First test whether root-centroid predictions have a confidence margin that
  separates useful target recovery from harm unsafe movement.

## Next Step

M1186 should be a confidence-gated replay audit over M1185 predictions:

- Use `root_centroid_pos` and `p1_centroid_pos` Top10 scores.
- Sweep confidence thresholds and max atom count.
- Measure retained positive recovery, positive unsafe, harm unsafe.
- Stop if no threshold can keep meaningful positive recovery while reducing
  harm unsafe below the M1184 Top10 baseline.

Acceptance for scaling:

- positive recovered share remains above M1184 Top10 (`0.080`);
- harm unsafe falls below M1184 Top10 (`0.063`);
- positive unsafe does not increase over M1184 Top10 (`0.058`).

If M1186 fails, the dense/root tail-added atom target should not be expanded as
a blind first-stage query compiler. The route should pivot to
retrieval-conditioned policy, where candidate context can supply the missing
safety signal.

## Artifacts

- `scripts/audit_m1185_dense_root_added_atom_predictor.py`
- `runs/m1185_dense_root_added_atom_predictor_v1/summary.md`
- `runs/m1185_dense_root_added_atom_predictor_k3_v1/summary.md`
- `runs/m1185_dense_root_added_atom_predictor_k10_v1/summary.md`
