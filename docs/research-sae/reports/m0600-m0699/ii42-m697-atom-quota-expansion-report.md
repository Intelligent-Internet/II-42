# M697 Atom Quota Expansion Audit

## Status

M697 is complete and produces a clear positive interface signal.

M696 showed that most M695 missing atoms were not missing because source
documents were absent. They were missing because the atom candidate budget
truncated them after the source documents were already available. M697 tests
that diagnosis by increasing only the source-specific atom quota.

## Setup

- Script: `scripts/audit_m697_atom_quota_expansion.py`
- Full summary:
  `runs/m697_atom_quota_expansion_v1/m697_summary.json`
- Full generated report:
  `runs/m697_atom_quota_expansion_v1/m697_report.md`
- Surface: native shared15 PostgreSQL path.
- Target rows: M691 `rank_safe_recall` rows.
- Source docs: qrels-free corpus support/composite sources from M695.
- No training in this step.

## Result

Full shared15 target atom visibility:

| Mode | Rows | Atom visibility | Full-hit rows | Visible atoms | Target atoms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `support_atoms48` | 29 | 0.263393 | 0 | 118 | 448 |
| `support_atoms96` | 29 | 0.381696 | 3 | 171 | 448 |
| `support_atoms192` | 29 | 0.627232 | 11 | 281 | 448 |
| `support_atoms384` | 29 | 0.883929 | 16 | 396 | 448 |
| `support_only384` | 29 | 0.883929 | 16 | 396 | 448 |
| `composite_atoms192` | 29 | 0.631696 | 11 | 283 | 448 |
| `oracle_teacher48` | 29 | 0.937500 | 22 | 420 | 448 |

The practical result is large:

- baseline q-free support source: `0.263393`;
- expanded q-free support atoms: `0.883929`;
- oracle teacher source: `0.937500`.

The q-free support source recovers most of the oracle visibility once the atom
candidate budget is no longer too narrow.

## Interpretation

This is the strongest positive signal after M691.

The blocker is not general training depth and not mostly qrels-free document
source recall. The blocker is the atom admission interface:

1. qrels-free support source docs are usually present;
2. target atoms are present in those docs;
3. the old `48` atom budget truncates them;
4. `384` source-specific atom candidates recovers `396 / 448` target atoms.

This creates a justified next training stage. We now have a candidate interface
with enough target visibility for a learned compiler/admission model to matter.

## Decision

Promote M697 as the next implementation target, not as a deployable model.

Do not return to traditional SAE reconstruction.

Do not launch more training over the old M692/M695 `48` atom interface.

Proceed to train/evaluate a source-aware generated-posting compiler using
expanded qrels-free support atom candidates, with strict dense/P1 head guards.

## Next Step

M698 should be a native shared15 training/evaluation pass:

1. Use `support_atoms384` or an equivalent source-specific atom candidate
   interface.
2. Train only a global atom admission/scoring layer over qrels-free features.
3. Keep doc posting/index geometry frozen.
4. Keep top95/CUB/NDCG/MRR as hard reporting gates.
5. Accept only if native shared15 shows positive Recall/MAP without material
   head metric regression.
