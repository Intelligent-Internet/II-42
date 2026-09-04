# M1217 LODO Contrastive Teacher Replay

M1217 validates the M1216 contrastive atom teacher under leave-one-dataset-out
atom filtering.

M1216 estimated positive-vs-harm atom contrast on the same full surface used
for replay.  M1217 estimates the atom filter from train datasets and replays
oracle-winning deltas on the held-out dataset.

This is still a teacher audit, not a deployable policy, because held-out replay
uses oracle-winning action identity.

## Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `lodo_r1.5_top8_s1` | 0.201 | 0 | +0.000803 | +0.000247 | +0.000094 | +0.000000 | +0.000015 | 0.004960 |
| `lodo_r1.5_top3_s1` | 0.201 | 0 | +0.000803 | +0.000247 | +0.000094 | +0.000000 | +0.000015 | 0.004960 |
| `lodo_r2_top8_s1` | 0.112 | 0 | +0.000000 | +0.000089 | +0.000023 | +0.000000 | +0.000015 | 0.000327 |

The hard-row LODO replay keeps the safety signal, but with much smaller
magnitude than M1216 in-surface filtering.

## Full Shared15

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `lodo_r1.5_top8_s1` | 0.522 | 0 | +0.000279 | +0.000802 | +0.000691 | +0.000488 | +0.000263 | 0.006418 |
| `lodo_r1.5_top3_s1` | 0.474 | 0 | +0.000279 | +0.000768 | +0.000579 | +0.000486 | +0.000257 | 0.006082 |
| `lodo_r2_top3_s1` | 0.320 | 0 | +0.000283 | +0.000475 | +0.000349 | +0.000196 | +0.000050 | 0.003982 |
| `lodo_r2_top8_s1` | 0.329 | 0 | +0.000283 | +0.000469 | +0.000345 | +0.000196 | +0.000050 | 0.003955 |
| `lodo_r1.5_top8_s0.5` | 0.522 | 0 | +0.000097 | +0.000220 | +0.000322 | +0.000122 | +0.000046 | 0.002081 |
| `lodo_r1.5_top3_s0.5` | 0.474 | 0 | +0.000014 | +0.000201 | +0.000310 | +0.000107 | +0.000044 | 0.001550 |

The best LODO result is `lodo_r1.5_top8_s1`.

Compared with M1216 full in-surface filtering:

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M1216 `contrastive_r1.5_top8_s1` | +0.000249 | +0.001083 | +0.000945 | +0.000614 | +0.000207 | 0.007822 |
| M1217 `lodo_r1.5_top8_s1` | +0.000279 | +0.000802 | +0.000691 | +0.000488 | +0.000263 | 0.006418 |

LODO keeps most of the score and all macro metrics remain positive.

## Interpretation

This is the first recent branch that survives the sequence:

1. hard-row smoke
2. full shared15 replay
3. held-out atom-filter validation

The signal is still small and teacher-only, but it is structurally different
from the failed local selector route:

- M1214 failed because action values did not generalize.
- M1215 showed raw oracle atoms were too entangled with harm atoms.
- M1216 showed contrastive filtering makes oracle deltas safe.
- M1217 shows the contrastive filter is not just full-surface leakage.

## Decision

Keep the contrastive atom teacher branch alive.

Do not claim it beats M1191/M1210 as a deployable policy.  It does not.  Its
value is as a cleaner supervision target for a generated-posting model.

Stop conditions that were avoided:

- CUB did not go negative for the best LODO variant.
- NDCG/MRR did not regress.
- hard-row gains did not disappear on full shared15.

## Next Direction

Run M1218: train a small atom proposal model against the M1217 teacher.

Scope must stay narrow:

- train target: atoms selected by `lodo_r1.5_top8_s1`
- input: baseline query atom features and native query/context features
- output: small set of atom additions, not a rerank score
- eval: native hard-row smoke first, then full shared15 only if smoke passes

Acceptance:

- generated model replay must keep CUB non-negative
- it must improve Recall/MAP/NDCG/MRR over baseline on smoke
- if smoke overfits or harms CUB, stop before full

## Artifacts

- Script: `scripts/audit_m1217_lodo_contrastive_teacher_replay.py`
- Smoke JSON: `runs/m1217_lodo_contrastive_teacher_replay_smoke_v1/m1217_lodo_contrastive_teacher_replay.json`
- Smoke Markdown: `runs/m1217_lodo_contrastive_teacher_replay_smoke_v1/m1217_lodo_contrastive_teacher_replay.md`
- Full JSON: `runs/m1217_lodo_contrastive_teacher_replay_v1/m1217_lodo_contrastive_teacher_replay.json`
- Full Markdown: `runs/m1217_lodo_contrastive_teacher_replay_v1/m1217_lodo_contrastive_teacher_replay.md`
