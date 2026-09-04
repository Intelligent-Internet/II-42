# M1167 Direct Witness Class Gate

M1167 checks whether M1166 direct-action witness classes are predictable from
query-time features.  This is a deployability gate for the next generated-atom
teacher: if the clean/no-exit classes cannot be separated, teacher generation
must keep a native replay guard instead of relying on a blind classifier.

## Inputs

- Witness rows: M1166, 138 M1163-selected direct protected-tail actions.
- Features: M1144 native tail features, M1144 class probabilities, M1145 action
  probabilities, and M1145 action-specific candidate boundary features.
- Validation: leave-one-dataset-out logistic regression.
- Output:
  `runs/m1167_direct_witness_class_gate_v1/direct_witness_class_gate.json`.

## Label Predictability

| Label | Positives | LODO AUC |
| --- | ---: | ---: |
| `clean_entry` | 26 | 0.758585 |
| `no_relevant_exit` | 75 | 0.450370 |
| `useful_direct` | 79 | 0.593864 |

The clean-entry signal is real but incomplete.  The no-relevant-exit signal is
not currently learnable from these features, so exit safety cannot be delegated
to this blind gate.

## Best Row-Floor-Clean Gate

| Policy | Accepted | Classes | Actions | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| `clean0.40_noexit0.50_useful0.10` | 47 | `{'no_relevant_boundary_change': 10, 'clean_relevant_entry': 17, 'relevant_swap': 10, 'rank_gain_no_top100_recall': 9, 'relevant_exit_only': 1}` | `{'protect5': 1, 'protect20': 46}` | +0.000000 | +0.001041 | +0.000737 | +0.000000 | +0.000000 |

This is clean and useful, but it is not enough to call a breakthrough.  It
recovers a safe slice of the direct-action gain, mostly Recall/MAP, while NDCG
and MRR remain flat.

## Interpretation

M1167 confirms a structural constraint:

- Clean relevant entry can be selected above chance, so M1166 is not noise.
- Relevant-exit risk is not separable by current query-time features.
- Therefore the next generated-atom teacher should use `clean_relevant_entry`
  as the first positive target and keep `relevant_swap` behind a native replay
  or explicit exit-loss guard.

## Decision

Proceed to a new direct-action generated-atom teacher only under these
constraints:

1. Build the first target set from M1166 `clean_relevant_entry` rows.
2. Treat `relevant_exit_only` and no-boundary-change rows as negatives.
3. Do not use `relevant_swap` as an unconstrained positive.
4. Re-evaluate every trained/generated target through native replay before
   promotion because the current no-exit proxy is weak.

This avoids the previous loop where selector tuning was asked to solve a
proposal/teacher shape problem.
