# M1186 Confidence-Gated Added Atoms

## Objective

M1185 found a wider Top10 signal: dense/root centroid predictions recovered
more M1183 tail-added target mass than base atom cooccurrence, but with high
harm unsafe movement. M1186 tests whether a simple confidence gate can keep the
positive signal while reducing harm enough to justify larger training.

This is still an audit. It does not modify the native index.

## Gate

M1186 sweeps:

- feature source: `root_centroid_pos`, `p1_centroid_pos`;
- threshold quantiles: `0.00, 0.25, 0.50, 0.70, 0.80, 0.90, 0.95`;
- max atoms: `1,2,3,5,10`.

Acceptance is intentionally tied to M1184/M1185 baselines:

- positive recovered share > `0.080` (beats M1184 Top10);
- harm unsafe < `0.063` (below M1184 Top10);
- positive unsafe <= `0.058` (not worse than M1184 Top10).

## Result

- Output root: `runs/m1186_confidence_gated_added_atoms_v1`
- Records: `42510`
- Passing gate count: `0`

| Method | Q | Max atoms | Pos recovered | Pos unsafe | Harm unsafe | Score | Pass |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `root_centroid_pos` | 0.25 | 10 | 0.115 | 0.068 | 0.094 | 0.051062 | False |
| `p1_centroid_pos` | 0.25 | 10 | 0.114 | 0.063 | 0.098 | 0.049854 | False |
| `root_centroid_pos` | 0.00 | 10 | 0.115 | 0.072 | 0.112 | 0.041160 | False |
| `p1_centroid_pos` | 0.00 | 10 | 0.114 | 0.066 | 0.124 | 0.035679 | False |
| `root_centroid_pos` | 0.50 | 10 | 0.087 | 0.064 | 0.082 | 0.029517 | False |
| `p1_centroid_pos` | 0.50 | 10 | 0.087 | 0.064 | 0.094 | 0.023381 | False |
| `root_centroid_pos` | 0.70 | 10 | 0.070 | 0.059 | 0.080 | 0.015190 | False |

The tradeoff is clear:

- low thresholds retain positive recovery but harm unsafe remains too high;
- high thresholds reduce some positive unsafe but lose the recovered mass before
  harm unsafe reaches the required floor.

## Interpretation

M1186 closes this branch as a deployable first-stage compiler path:

1. M1183: target is compact after tail atoms are known.
2. M1184: base atom cooccurrence cannot predict it safely.
3. M1185: dense/root features recover more target mass, but not safely.
4. M1186: confidence gating cannot make that signal safe.

The failure is not just training depth. These audits are directly testing
whether the supervision is separable before training. The separability is not
strong enough for a blind query-only added-atom compiler.

## Decision

Do not scale M1183/M1185 into a larger blind first-stage training run.

The useful signal appears to need candidate/retrieval context. The next
structural route should be retrieval-conditioned generated-posting policy:

- keep P1/native unified posting as the substrate;
- allow query-time candidate context or native boundary features;
- output a constrained posting delta/policy rather than a free reranker;
- prove safety through native replay before training scale-up.

## Artifacts

- `scripts/audit_m1186_confidence_gated_added_atoms.py`
- `runs/m1186_confidence_gated_added_atoms_v1/summary.md`
- `runs/m1186_confidence_gated_added_atoms_v1/confidence_gated_added_atoms.json`
