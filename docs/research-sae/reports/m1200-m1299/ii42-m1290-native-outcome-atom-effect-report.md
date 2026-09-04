# M1290 Native Outcome Atom Effect Audit

## Question

Recent reviews argued that the branch should stop ordinary selector/gate
tuning and move harm separation into the source or objective.  M1290 tests a
minimal version of that claim:

- take deployable raw candidate atoms only;
- apply each atom by itself through the native P1 scorer;
- label atoms by actual native boundary outcome;
- test whether query-time features can separate safe effective atoms.

This is not a training run.  It is an observability gate before training a
deployable selector.

## Setup

- Datasets: `arguana,cqadupstack,fiqa,scidocs`
- Limit: `25` queries per dataset
- Candidate source: raw `abs_vote` top 16 atoms
- Scale: `0.05`
- Outcome path: native PostgreSQL P1 scoring
- Feature groups: `raw`, `pseudo`, `raw_pseudo`
- Output:
  `runs/m1290_native_outcome_atom_effect_limit25_v1/m1290_atom_effect.json`

An atom is labelled `effective` only if it fixes at least one dense-boundary
pair, introduces no pair regression, and keeps protected top95 above the safe
floor.  It is labelled `harmful` if it regresses a pair or violates the
protected top95 floor.

## Outcome Summary

| Surface | Rows | EffectiveShare | HarmfulShare | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `all` | 1600 | 0.008750 | 0.102500 | 25 | 23 | 0.993434 |
| `train` | 960 | 0.009375 | 0.111458 | 20 | 7 | 0.992982 |
| `eval` | 640 | 0.007812 | 0.089063 | 5 | 16 | 0.994112 |
| `arguana` | 400 | 0.000000 | 0.080000 | 0 | 1 | 0.993684 |
| `cqadupstack` | 400 | 0.012500 | 0.057500 | 5 | 0 | 0.994447 |
| `fiqa` | 400 | 0.007500 | 0.107500 | 3 | 10 | 0.993895 |
| `scidocs` | 400 | 0.015000 | 0.165000 | 17 | 12 | 0.991711 |

The effective base rate is too low for this candidate source: only `0.78%` on
heldout eval.  Harm is more than `11x` more common than effective movement.

## Separability

| Label | Features | TrainAUC | EvalAUC | TrainPos | EvalPos |
| --- | --- | ---: | ---: | ---: | ---: |
| `is_effective` | `pseudo` | 1.000000 | 0.424882 | 0.009375 | 0.007812 |
| `is_effective` | `raw` | 1.000000 | 0.409764 | 0.009375 | 0.007812 |
| `is_effective` | `raw_pseudo` | 1.000000 | 0.311654 | 0.009375 | 0.007812 |
| `is_harmful` | `pseudo` | 0.999956 | 0.653396 | 0.111458 | 0.089063 |
| `is_harmful` | `raw` | 0.996521 | 0.618955 | 0.111458 | 0.089063 |
| `is_harmful` | `raw_pseudo` | 0.999989 | 0.648882 | 0.111458 | 0.089063 |

The model overfits train effective labels perfectly but fails on eval.  Harm is
moderately observable, but useful positive movement is not.

## Interpretation

M1290 supports the review diagnosis.

The problem is not that we need another threshold, deeper tree, or more
selector variants on the same feature view.  Generic raw atom candidates do
contain occasional useful atoms, but safe effective atoms are rare and not
separable by the current deployable raw/native-context features.

This also explains M1289: the student could imitate broad M1288 teacher
selection, especially after global atom priors were added, but it did not learn
the rank-effective low-harm subset.

## Decision

Stop this exact `raw abs_vote topN -> outcome classifier` branch.

Do not train a deployable selector from M1290 features.  The next branch should
change candidate/source construction, not classifier mechanics.

## Next Valid Step

Use M1225, M1251, and M1288 as constraints for the next source:

1. Keep the source action-aware or pair-aware, because aggregated atoms lose
   the target/harm boundary.
2. Generate candidates from native movement structure, not from raw atom
   magnitude alone.
3. Evaluate a new source with the same M1290 native-outcome audit before any
   student training.
4. Only train if effective labels become non-trivial and eval AUC is above the
   observability gate.

The current best hypothesis is therefore:

> Useful atoms exist, but safe selection must be made internal to
> action/pair/CUB-aware source construction.  Post-hoc qrels-free gates over
> generic atom candidates remain unsupported.
