# M1332 Source-Objective Construction

## Question

M1331 showed that M1330 source choices are not sufficiently observable from
query-time statistics for a post-hoc selector. M1332 tests the next idea:

> Move M1330 source-family supervision into atom source construction itself.

The script learns atom priors from oracle-chosen source families on training
datasets, then constructs a single query-time source from the union of:

- `candidate_self`
- `rank_prefix`
- `rank_fill`

This is only a target/harm frontier audit. It does not run native retrieval and
does not promote a policy.

## Smoke Result

Datasets: `cqadupstack`, `scidocs`, `webis-touche2020`.

| Policy | TargetRecall | Precision | HarmPrecision | HarmRecall | PredCount | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `rank_prefix_top4` | 0.5580 | 0.7669 | 0.2331 | 0.1056 | 1.309 | 0.8059 |
| `rank_prefix_top8` | 0.5580 | 0.7669 | 0.2331 | 0.1056 | 1.309 | 0.8059 |
| `union_score_top4` | 0.8839 | 0.4648 | 0.5352 | 0.6333 | 3.422 | -0.0384 |
| `candidate_self_top4` | 0.8616 | 0.4531 | 0.5469 | 0.6472 | 3.422 | -0.1028 |
| `source_objective_consensus_top4` | 0.2210 | 0.5130 | 0.4870 | 0.1306 | 0.775 | -0.3054 |
| `candidate_self_top8` | 0.9911 | 0.3874 | 0.6126 | 0.9750 | 4.602 | -0.3341 |
| `source_objective_top4` | 0.2411 | 0.4000 | 0.6000 | 0.2250 | 1.084 | -0.6714 |

## Interpretation

The global source-objective atom prior fails the hard-row gate.

It does not improve over the simple source controls. The cleanest policy is
still `rank_prefix`, which has good precision but inadequate target recall.
`candidate_self` and `union_score` have much higher target recall, but their
harm precision is too high.

So the next useful problem is narrower:

> Can we safely admit the candidate tail outside `rank_prefix`?

That is different from a generic selector. It keeps `rank_prefix` as the clean
anchor and asks whether candidate-only tail atoms have local admission
structure.

## Decision

Do not run full `shared15` native replay for M1332.

Stop this global-prior source-objective construction branch. The hard-row
frontier is not strong enough.

## Next Valid Work

M1333 should audit candidate-tail admission:

1. Treat `rank_prefix` atoms as the high-precision anchor.
2. Label candidate-only tail atoms by whether they belong to the M1330 oracle
   target source or only to nonchosen sources.
3. Measure whether query-local tail features separate useful tail atoms from
   harmful tail atoms.
4. Only if separation exists, construct a `rank_prefix + admitted_tail` source
   and run hard-row native replay.

This keeps the lesson from M1332: the issue is not the source head; it is safe
candidate-tail expansion.

## Artifacts

- Script:
  `scripts/audit_m1332_source_objective_construction.py`
- Smoke JSON:
  `runs/m1332_source_objective_construction_smoke_v1/m1332_source_objective_construction.json`
- Smoke Markdown:
  `runs/m1332_source_objective_construction_smoke_v1/m1332_source_objective_construction.md`
