# ii42 M1131 Alpha-Policy Stop and Next Route

## Scope

This stage audited whether the M1129 shared15 listwise-only checkpoint should
be followed by another alpha/selector policy line.

Inputs:

- M1129 shared15 listwise050 control ranking export.
- Inference-time query features only:
  candidate count, lexical/atom correlation, top-k lexical/atom overlap,
  cross-rank positions, and lexical/atom score gaps.
- No dataset-specific policy, no qrels-derived features, no model retraining.

## Results

### M1130 global alpha-bound policy

The best non-degenerate train-selected feature policy was:

```text
lex_atom_corr <= 0.551908 ? alpha 0.75 : alpha 0.60
```

Heldout result:

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| fixed alpha 0.00 | 0.746410 | 0.742393 | 0.669888 | 0.563245 |
| fixed alpha 0.50 | 0.770982 | 0.777186 | 0.699337 | 0.604800 |
| fixed alpha 0.60 | 0.772398 | 0.779772 | 0.699965 | 0.606185 |
| fixed alpha 0.75 | 0.771468 | 0.763056 | 0.689540 | 0.600224 |
| feature policy | 0.771531 | 0.764168 | 0.689995 | 0.600557 |
| query metric oracle | 0.780820 | 0.808073 | 0.734918 | 0.639374 |

The feature policy is worse than fixed alpha 0.60 on all macro metrics:

| Comparison | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| feature policy - fixed 0.60 | -0.000867 | -0.015604 | -0.009971 | -0.005628 |

It also damages 13 datasets versus fixed alpha 0.60, so it is not a
promotion candidate.

### M1131 alpha-gate separability

Train split LODO:

| Transition | LODO AUC | LODO AP | Verdict |
| --- | ---: | ---: | --- |
| 0.50 -> 0.60 | 0.408579 | 0.880575 | fail |
| 0.60 -> 0.75 | 0.447200 | 0.868972 | fail |

Train -> heldout cross split:

| Transition | Cross AUC | Cross AP | Verdict |
| --- | ---: | ---: | --- |
| 0.50 -> 0.60 | 0.551216 | 0.502624 | fail |
| 0.60 -> 0.75 | 0.445921 | 0.380228 | fail |

The heldout-only LODO for 0.60 -> 0.75 looked superficially positive, but it
does not survive train -> heldout cross evaluation. That means it should be
treated as split-local structure, not as a stable deployable gate signal.

## Conclusion

M1129 listwise-only remains a real macro-positive route, but the current
alpha-policy branch should stop here.

Retain:

- The listwise-only training signal from M1128/M1129.
- Fixed alpha 0.50/0.60 as the bounded operating band.
- Query metric oracle as evidence of remaining headroom.

Reject for now:

- Train-selected fixed alpha 0.75.
- Simple global feature alpha gates.
- Another round of feature-threshold or logistic alpha selector tuning over
  the same feature table.

## Next Route

The remaining oracle gap is not explained by a simple alpha gate. The next
useful route should change the scoring representation, not just choose alpha:

1. Keep M1129 listwise-only atom pressure as the current best training signal.
2. Build a score-shape audit over positive boundary documents:
   determine whether failures come from atom score calibration, lexical score
   domination, or missing interaction terms.
3. Test a small global monotonic score transform over atom scores before
   addition, with fixed alpha 0.50/0.60, rather than query-level alpha
   switching.
4. Only if the transform improves heldout macro and reduces row damage should
   it be scaled back into a new training objective.

Stop condition for the next route:

- If a global score transform cannot beat fixed alpha 0.60 without increasing
  dataset damage, do not keep tuning score wrappers. Return to the training
  objective and make the listwise signal directly optimize the failing
  boundary score shape.
