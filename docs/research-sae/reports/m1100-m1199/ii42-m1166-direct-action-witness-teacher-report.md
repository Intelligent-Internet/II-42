# M1166 Direct-Action Witness Teacher Audit

M1166 reconstructs the 138 M1163-selected native protected-tail actions through
the native table path.  It extracts top100 entrants/exits and qrel witness
statistics so the next generated-atom teacher can be built from the direct
action rather than from the old M1155 admission-atom pool.

## Inputs

- Selected detector: M1163 `m1144_supported_by_m1145`,
  threshold44 = 0.60, threshold45 = 0.30.
- Native streams: base M1129 and tail M1137 through the M1142 table-backed
  evaluator.
- Output:
  `runs/m1166_direct_action_witness_teacher_v1/direct_action_witness_teacher.json`.

## Overall

| Selected | Reconstructed | protect20 | protect5 |
| ---: | ---: | ---: | ---: |
| 138 | 138 | 96 | 42 |

Per selected-query delta:

| dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| ---: | ---: | ---: | ---: | ---: |
| +0.000721 | +0.013801 | +0.013618 | +0.008751 | +0.000000 |

## Witness Classes

| Class | Count | Actions | Mean overlap@100 | Mean entrant rel | Mean exit rel | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `clean_relevant_entry` | 26 | `{'protect20': 25, 'protect5': 1}` | 0.820385 | 2.807692 | 0.000000 | -0.000211 | +0.064717 | +0.048494 | -0.000068 | +0.000000 |
| `relevant_swap` | 57 | `{'protect20': 28, 'protect5': 29}` | 0.785439 | 12.684211 | 9.105263 | +0.001843 | +0.012476 | +0.014851 | +0.021218 | +0.000000 |
| `rank_gain_no_top100_recall` | 16 | `{'protect20': 16}` | 0.810000 | 0.000000 | 0.000000 | +0.000000 | +0.000000 | +0.006589 | +0.000000 | +0.000000 |
| `no_relevant_boundary_change` | 33 | `{'protect20': 21, 'protect5': 12}` | 0.769394 | 0.000000 | 0.000000 | +0.000000 | +0.000000 | -0.001199 | +0.000000 | +0.000000 |
| `relevant_exit_only` | 6 | `{'protect20': 6}` | 0.766667 | 0.000000 | 2.833333 | +0.000000 | -0.081534 | -0.048991 | +0.000000 | +0.000000 |

## Interpretation

The direct-action teacher is structurally mixed.  It is not safe to treat all
M1163 selected actions as the same positive target.

- `clean_relevant_entry` is the cleanest generated-atom seed for Recall@100:
  it adds relevant top100 entrants without relevant exits.
- `relevant_swap` is the largest bucket and carries strong MAP/NDCG/Recall, but
  it replaces relevant documents too.  It requires explicit exit penalties or
  support constraints.
- `rank_gain_no_top100_recall` is useful for MAP-only objectives, not for
  Recall expansion.
- `no_relevant_boundary_change` and `relevant_exit_only` should be filtered or
  used as negative/guard examples.

## Decision

M1166 supports a new direct-action teacher, but only as a typed teacher:

1. Start with `clean_relevant_entry` as positive Recall expansion targets.
2. Add `relevant_swap` only with a no-relevant-exit/support-preservation
   penalty.
3. Keep rank-only gains separate from Recall training.
4. Treat `relevant_exit_only` and no-boundary-change rows as guard negatives.

The next small experiment should test whether these witness classes are
predictable from inference-time features.  If not, the new teacher must remain
guarded by explicit native witness/replay rather than a blind selector.
