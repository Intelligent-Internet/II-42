# M1174 Tail Doc-Teacher Movement Audit

M1174 demotes the full M1137 `tail_full` surface from a runtime route to a
doc-level teacher/reference surface.  The question is whether `tail_full`
contains usable training signal after M1172/M1173 showed that raw tail routing
is not row-floor safe.

Artifacts:

- Script: `scripts/audit_m1174_tail_doc_teacher_movement.py`
- JSON: `runs/m1174_tail_doc_teacher_movement_v1/tail_doc_teacher_movement.json`
- Run summary: `runs/m1174_tail_doc_teacher_movement_v1/summary.md`

## Overall Result

Compared with protected direct action on the same 138 selected queries:

| View | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| direct_protect | +0.000721 | +0.013801 | +0.013618 | +0.008751 | +0.000000 |
| tail_full | +0.000733 | +0.013691 | +0.018345 | +0.016553 | +0.022145 |
| tail_minus_direct | +0.000011 | -0.000110 | +0.004727 | +0.007801 | +0.022145 |

Doc movement:

- Teacher classes: `tail_rank_teacher=51`, `tail_harm=67`, `tail_neutral=20`.
- Relevant top100 entrants from direct to tail: `11`.
- Relevant top100 exits from direct to tail: `11`.
- Pair-target boundary examples: `24`.
- Relevant rank-gain movements: `1792`.
- Relevant rank-loss movements: `592`.

## Interpretation

This is not a clean Recall boundary teacher.  There are as many relevant exits
as relevant entrants, and no `tail_clean_teacher` rows survived the strict
classification.  The top100 boundary signal is too sparse and too entangled
with harm to train a reliable expansion objective directly from `tail_full`.

The real signal is ranking movement.  `tail_rank_teacher` has no Recall/CUB
movement but improves the rank metrics strongly:

| Class | Count | Rel rank gains | Rel rank losses | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| tail_rank_teacher | 51 | 466 | 187 | +0.000000 | +0.033015 | +0.072248 | +0.091433 |
| tail_harm | 67 | 1276 | 374 | -0.000226 | -0.015394 | -0.038927 | -0.023987 |

The harm rows also contain rank gains, so the next objective cannot simply
imitate all tail preferences.  It needs explicit harm penalties and row-level
constraints.

## Dataset Notes

- `arguana` and `dbpedia-entity` are clean positive ranking surfaces.
- `trec-covid` has strong rank gains and some boundary movement, but harm is
  mixed with useful rows.
- `msmarco` and `nfcorpus` are the main unsafe surfaces; tail improves some MAP
  but hurts NDCG/MRR or Recall.

## Decision

Stop treating `tail_full` as a runtime router or a Recall expansion teacher.
Keep it as a typed ranking teacher/reference:

1. Positive labels: `tail_rank_teacher` rank improvements.
2. Negative labels: `tail_harm` rank losses, relevant exits, and row-level
   NDCG/MRR regressions.
3. Boundary labels: keep only as a tiny auxiliary term; do not make them the
   primary objective.

The next worthwhile branch is a conservative rank-distillation objective with
hard dense/direct safety floors, not another raw tail surface gate.
