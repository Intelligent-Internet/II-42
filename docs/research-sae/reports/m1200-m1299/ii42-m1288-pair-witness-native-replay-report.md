# M1288 Pair/Witness Native Replay

M1288 replays the M1287 pair/witness-selected atoms through the native P1
posting scorer.  This tests whether the strong pair/witness source signal
survives actual native ranking movement.

This is a smoke gate, not a deployable policy.  The pair/witness features still
use dense-boundary witness documents, so a passing result means the source is
valuable as a teacher/objective shape.  It does not yet mean query-time
qrels-free selection is solved.

## Smoke Setup

- Surface: `arguana,cqadupstack,fiqa,scidocs`
- Limit: 25 queries per dataset
- Heldout surface: `dev + test`
- Feature group: `pair`
- Budgets: `96,192,384`
- Risk penalties: `0,0.5,1`
- Scales: `0.02,0.05`
- Native path: PostgreSQL P1 native posting scorer
- Output:
  `runs/m1288_pair_witness_selector_native_limit25_v2/m1288_native.json`

## Smoke Eval Best Variants

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | Top100 | Top256 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_risk0.5_b384_s0.05` | 0.706667 | 0.520000 | 56 | 0 | 0.957368 | 0.955500 | 0.960547 | 0.981611 | 0.004167 |
| `pair_risk1_b384_s0.05` | 0.703333 | 0.520000 | 55 | 0 | 0.958947 | 0.956000 | 0.961328 | 0.972099 | 0.001107 |
| `pair_risk0.5_b192_s0.05` | 0.700000 | 0.520000 | 54 | 0 | 0.959211 | 0.957000 | 0.962109 | 0.981611 | 0.008333 |
| `pair_risk1_b192_s0.05` | 0.696667 | 0.520000 | 53 | 0 | 0.961053 | 0.958500 | 0.962891 | 0.972099 | 0.002214 |
| `pair_risk0_b192_s0.05` | 0.676667 | 0.520000 | 47 | 0 | 0.957368 | 0.957750 | 0.960254 | 0.983513 | 0.041146 |

The best heldout variant improves native pair success by `+0.186667` with no
pair regressions in this smoke surface.

## Smoke Dataset Heldout Check

| Dataset | Best Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| arguana | `pair_risk0.5_b384_s0.05` | 0.718750 | 0.489583 | 22 | 0 | 0.952227 |
| cqadupstack | `pair_risk0.5_b384_s0.05` | 0.666667 | 0.483333 | 11 | 0 | 0.956579 |
| fiqa | `pair_risk1_b384_s0.05` | 0.661765 | 0.529412 | 9 | 0 | 0.967251 |
| scidocs | `pair_risk1_b192_s0.05` | 0.763158 | 0.578947 | 14 | 0 | 0.962105 |

All four datasets show positive pair movement with no observed pair
regression on the heldout smoke split.

## Full Four-Dataset Replay

After the smoke gate passed, the same variant space was replayed on all 399
queries from the same four datasets.  No extra hyperparameter search was added.

Output:
`runs/m1288_pair_witness_selector_native_four_full_v1/m1288_native.json`

### Full Eval Best Variants

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | Top100 | Top256 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_risk1_b384_s0.05` | 0.713645 | 0.561041 | 170 | 0 | 0.953857 | 0.954247 | 0.961018 | 0.973253 | 0.000999 |
| `pair_risk0.5_b384_s0.05` | 0.711849 | 0.561041 | 168 | 0 | 0.953425 | 0.953425 | 0.960376 | 0.982106 | 0.005583 |
| `pair_risk0.5_b192_s0.05` | 0.708259 | 0.561041 | 164 | 0 | 0.956309 | 0.955822 | 0.961446 | 0.982106 | 0.011166 |
| `pair_risk1_b192_s0.05` | 0.707361 | 0.561041 | 163 | 0 | 0.959048 | 0.957055 | 0.962837 | 0.973253 | 0.001998 |
| `pair_risk0_b192_s0.05` | 0.692998 | 0.561041 | 147 | 0 | 0.955588 | 0.955137 | 0.960269 | 0.982671 | 0.044164 |

The best full heldout variant improves native pair success by `+0.152604` with
`170` fixed pairs and zero observed pair regressions.

### Full Dataset Heldout Check

| Dataset | Best Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| arguana | `pair_risk0.5_b384_s0.05` | 0.730263 | 0.565789 | 50 | 0 | 0.955526 |
| cqadupstack | `pair_risk1_b384_s0.05` | 0.746835 | 0.594937 | 36 | 0 | 0.952632 |
| fiqa | `pair_risk1_b384_s0.05` | 0.691228 | 0.571930 | 34 | 0 | 0.953912 |
| scidocs | `pair_risk1_b384_s0.05` | 0.694444 | 0.517361 | 51 | 0 | 0.953627 |

The full run keeps the smoke pattern: every dataset improves pair success and
none has observed pair regression in the heldout split.

## Shared15 Narrow Replay

The full four-dataset run justified a broader replay, but not a wider grid.
The shared15 pass therefore kept only the stable full-four subspace:

- training rows:
  `runs/m721_dense_boundary_training_rows_shared15_v1/m653_dense_boundary_training_rows.jsonl`
- query atoms:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- schema: `ii42_shared15`
- datasets: all 15 shared15 datasets
- queries: 1335
- variants: `risk=0.5,1`, `budget=96,192,384`, `scale=0.05`
- output:
  `runs/m1288_pair_witness_selector_native_shared15_narrow_v1/m1288_native.json`

### Shared15 Eval Best Variants

| Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 | Top100 | Top256 | TargetRecall | NegOnly |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_risk0.5_b192_s0.05` | 0.509081 | 0.347723 | 613 | 0 | 0.951422 | 0.952399 | 0.962757 | 0.947726 | 0.011351 |
| `pair_risk1_b192_s0.05` | 0.506976 | 0.347723 | 605 | 0 | 0.953650 | 0.954254 | 0.964300 | 0.940243 | 0.001880 |
| `pair_risk0.5_b384_s0.05` | 0.498552 | 0.347723 | 573 | 0 | 0.946520 | 0.947036 | 0.960717 | 0.947726 | 0.005676 |
| `pair_risk1_b384_s0.05` | 0.491972 | 0.347723 | 548 | 0 | 0.947602 | 0.948569 | 0.961725 | 0.940243 | 0.000940 |
| `pair_risk0.5_b96_s0.05` | 0.488813 | 0.347723 | 536 | 0 | 0.957194 | 0.958831 | 0.967797 | 0.937240 | 0.018292 |
| `pair_risk1_b96_s0.05` | 0.487497 | 0.347723 | 531 | 0 | 0.958807 | 0.961351 | 0.969561 | 0.920483 | 0.002919 |

The best shared15 variant improves pair success by `+0.161358` with `613`
fixed pairs and zero observed pair regressions.

### Shared15 Dataset Check

| Dataset | Best Variant | PairSuccess | Baseline | Fixed | Regressed | Top95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| arguana | `pair_risk0.5_b192_s0.05` | 0.575658 | 0.437500 | 42 | 0 | 0.956579 |
| climate-fever | `pair_risk0.5_b192_s0.05` | 0.556777 | 0.421245 | 37 | 0 | 0.957895 |
| cqadupstack | `pair_risk0.5_b192_s0.05` | 0.514768 | 0.329114 | 44 | 0 | 0.950000 |
| dbpedia-entity | `pair_risk0.5_b192_s0.05` | 0.381132 | 0.181132 | 53 | 0 | 0.949173 |
| fever | `pair_risk1_b192_s0.05` | 0.509259 | 0.296296 | 69 | 0 | 0.963800 |
| fiqa | `pair_risk0.5_b192_s0.05` | 0.578947 | 0.400000 | 51 | 0 | 0.944808 |
| hotpotqa | `pair_risk0.5_b192_s0.05` | 0.506494 | 0.347403 | 49 | 0 | 0.960594 |
| msmarco | `pair_risk0.5_b192_s0.05` | 0.447761 | 0.268657 | 24 | 0 | 0.939348 |
| nfcorpus | `pair_risk0.5_b192_s0.05` | 0.542969 | 0.375000 | 43 | 0 | 0.943860 |
| nq | `pair_risk0.5_b192_s0.05` | 0.546429 | 0.417857 | 36 | 0 | 0.959398 |
| quora | `pair_risk0.5_b192_s0.05` | 0.462451 | 0.335968 | 32 | 0 | 0.954386 |
| scidocs | `pair_risk0.5_b192_s0.05` | 0.517361 | 0.336806 | 52 | 0 | 0.955903 |
| scifact | `pair_risk0.5_b192_s0.05` | 0.591667 | 0.427778 | 59 | 0 | 0.946667 |
| trec-covid | `pair_risk0.5_b192_s0.05` | 0.030303 | 0.000000 | 4 | 0 | 0.920743 |
| webis-touche2020 | `pair_risk0.5_b192_s0.05` | 0.660000 | 0.450000 | 21 | 0 | 0.939098 |

Shared15 preserves the same core pattern: every dataset improves pair success
and no heldout pair regression is observed.  The remaining concern is protected
head movement: top95 stays above 0.95 macro, but some rows drop into the
0.92-0.94 range.

## Interpretation

M1288 is the first recent result in this branch that satisfies the review
constraint on smoke, full four-dataset, and shared15 surfaces: it is not
another post-hoc gate over the old feature view.  It changes the source by
preserving pair/witness identity, and that source transfers into native ranking
movement.

This supports the current diagnosis:

- useful added atoms exist;
- pair/witness source structure exposes them more cleanly than aggregated atom
  votes;
- native replay can move ranking without immediate pair regression on a small
  and broad heldout surface;
- query-time deployability is still unsolved because the selected source uses
  dense-boundary witnesses.

## Next Step

Do not default this policy directly.  The next valuable step is M1289:

1. Build a deployable approximation of the pair/witness source.  This should
   be source construction or training objective work, not another threshold
   guard.
2. Reuse M1288 as the teacher/replay target: preserve pair success gains while
   adding a stricter protected-head constraint, especially for trec-covid,
   msmarco, webis-touche2020, and nfcorpus.
3. Evaluate through the same native shared15 path.  A deployable candidate must
   preserve most of the M1288 pair-success gain and improve row-level top95.
4. Stop if the deployable approximation loses the pair/witness advantage or
   reintroduces pair regressions.

The current result is a real positive signal and should replace the older
selector/gate micro-tuning loop as the next branch to develop.
