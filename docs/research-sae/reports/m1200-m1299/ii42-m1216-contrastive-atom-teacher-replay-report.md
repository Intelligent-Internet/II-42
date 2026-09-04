# M1216 Contrastive Atom Teacher Replay

M1216 follows the M1215 negative structure audit.  Instead of using raw
oracle-winning atoms as targets, it filters atom deltas by positive-vs-harm
contrast and replays only the filtered atoms.

This is a teacher feasibility audit, not a deployable model.  It still uses
oracle-winning actions to choose which query delta to replay.  The question is
whether a stronger teacher can create a safe generated-posting target.

## Method

Source action family is M1213:

- high: `bm25_top_docs3_s0.10`
- mid: `bm25_top_docs3_s0.075`
- low: `fused_tail_docs3_s0.10`

For each query/action:

1. Build query atom delta relative to baseline.
2. Label oracle-winning non-baseline deltas as positive.
3. Label negative protected-score deltas as harm.
4. Keep atoms whose positive absolute delta exceeds harm absolute delta by a
   contrastive ratio.
5. Replay filtered atoms from the oracle-winning action.

Tested filters:

- ratio: `1.5`, `2`, `3`
- top atoms per query: `3`, `8`
- replay scale: `0.5`, `1.0`

## Hard-Row Smoke

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

Best smoke variants:

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `contrastive_r1.5_top8_s1` | 1.100 | 0 | +0.002053 | +0.001503 | +0.001298 | +0.000100 | +0.000000 | 0.017569 |
| `contrastive_r1.5_top3_s1` | 0.924 | 0 | +0.002053 | +0.001208 | +0.001531 | +0.000100 | +0.000000 | 0.017152 |
| `contrastive_r2_top8_s1` | 0.731 | 0 | +0.001249 | +0.000791 | +0.001296 | +0.000000 | +0.000000 | 0.011211 |

The smoke passed the safety gate: no negative macro metrics for the best
variants.

## Full Shared15

| Variant | Selected | NegMetrics | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `contrastive_r1.5_top8_s1` | 0.747 | 0 | +0.000249 | +0.001083 | +0.000945 | +0.000614 | +0.000207 | 0.007822 |
| `contrastive_r1.5_top3_s1` | 0.606 | 0 | +0.000244 | +0.001000 | +0.000840 | +0.000630 | +0.000215 | 0.007376 |
| `contrastive_r2_top8_s1` | 0.484 | 0 | +0.000228 | +0.000799 | +0.000754 | +0.000395 | +0.000172 | 0.006008 |
| `contrastive_r2_top3_s1` | 0.448 | 0 | +0.000228 | +0.000663 | +0.000631 | +0.000261 | +0.000186 | 0.005099 |
| `contrastive_r3_top8_s1` | 0.291 | 0 | +0.000244 | +0.000563 | +0.000386 | +0.000223 | +0.000038 | 0.004163 |
| `contrastive_r3_top3_s1` | 0.284 | 0 | +0.000244 | +0.000561 | +0.000386 | +0.000223 | +0.000038 | 0.004158 |

All scale-1 full variants are positive on all macro metrics.  The best full
variant is `contrastive_r1.5_top8_s1`.

## Interpretation

This is a real teacher signal, but not yet a deployable retrieval improvement.

What changed compared with M1215:

- M1215 showed raw oracle atoms are entangled with harm atoms.
- M1216 subtracts harm-heavy atoms before replay.
- The filtered target remains positive on hard rows and full shared15.

The important result is not the magnitude.  The magnitude is smaller than
M1191/M1210.  The important result is that contrastive filtering converts a
messy oracle target into a safe replay target.

## Decision

- Keep M1216 as the current generated-posting teacher direction.
- Do not promote it as a production policy.
- Do not restart local selector tuning.
- Next step must test whether the contrastive teacher survives held-out atom
  filtering.

## Next Direction

Run M1217: leave-one-dataset-out contrastive teacher replay.

M1216 used the full surface to estimate positive-vs-harm atom contrast.  M1217
must estimate the contrastive atom filter from train datasets and replay on the
held-out dataset.

Acceptance:

- best LODO contrastive replay keeps dCUB non-negative
- no material NDCG/MRR regression
- retains at least a meaningful fraction of M1216 full gains

If LODO fails, M1216 is only an in-surface teacher artifact.  If LODO passes,
the next step can train a small atom proposal model against this filtered
teacher.

## Artifacts

- Script: `scripts/audit_m1216_contrastive_atom_teacher_replay.py`
- Smoke JSON: `runs/m1216_contrastive_atom_teacher_replay_smoke_v1/m1216_contrastive_atom_teacher_replay.json`
- Smoke Markdown: `runs/m1216_contrastive_atom_teacher_replay_smoke_v1/m1216_contrastive_atom_teacher_replay.md`
- Full JSON: `runs/m1216_contrastive_atom_teacher_replay_v1/m1216_contrastive_atom_teacher_replay.json`
- Full Markdown: `runs/m1216_contrastive_atom_teacher_replay_v1/m1216_contrastive_atom_teacher_replay.md`
