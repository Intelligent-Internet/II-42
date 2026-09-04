# M1331 Source-Choice Observability

## Question

M1330 proved a strong full-`shared15` oracle capacity: query-level choice among
baseline, candidate-self, prefix-rank, and fill-rank sources can remove all
dataset-level harm while roughly doubling the best single-source score.

M1331 tests the deployable prerequisite:

> Can the M1330 strict oracle source family be predicted from query-time
> observable action/source statistics?

This is an observability audit only. It is not a native replay, not a selector
promotion, and not a new default policy.

## Inputs

- Oracle labels:
  `runs/m1330_rank_source_oracle_shared15_v1/m1325_rank_source_oracle.json`
- Source surface:
  M1329 low-reserve candidate plus rank-prefix/fill variants
- Feature view:
  query-time action deltas, action overlaps, selected source sizes, selected
  source score/delta statistics, and source overlaps
- Validation:
  leave-one-dataset-out over full `shared15`
- Script:
  `scripts/audit_m1331_source_choice_observability.py`

The script deliberately does not use qrels-derived labels, per-query metric
deltas, oracle winner actions, or CUB protected deltas as features.

## Full Shared15 Result

Query count: `1342`.

Feature count: `127`.

Oracle family distribution:

| Family | Queries |
| --- | ---: |
| `baseline` | 166 |
| `candidate_self` | 232 |
| `rank_fill` | 37 |
| `rank_prefix` | 907 |

LODO metrics:

| Model | Accuracy | BalancedAcc | MacroF1 | BaselineRecall | CandidateRecall | PrefixRecall | FillRecall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `constant_rank_prefix` | 0.6759 | 0.2500 | 0.2016 | 0.0000 | 0.0000 | 1.0000 | 0.0000 |
| `lodo_majority` | 0.6759 | 0.2500 | 0.2016 | 0.0000 | 0.0000 | 1.0000 | 0.0000 |
| `hgb_balanced` | 0.5738 | 0.2540 | 0.2480 | 0.1024 | 0.1121 | 0.8015 | 0.0000 |
| `logistic_balanced` | 0.3577 | 0.3095 | 0.2592 | 0.2590 | 0.2500 | 0.4046 | 0.3243 |

The balanced logistic model only improves MacroF1 from `0.2016` to `0.2592`.
That comes at the cost of collapsing top-line accuracy from `0.6759` to
`0.3577`, and class recalls remain too weak for a deployable source router.

Per-dataset MacroF1 is also low. The best logistic rows are still only:

| Dataset | Logistic MacroF1 |
| --- | ---: |
| `cqadupstack` | 0.3079 |
| `webis-touche2020` | 0.2721 |
| `climate-fever` | 0.2637 |

Several broad rows stay near chance or worse, including `scifact` at `0.0679`.

## Smoke Cross-Check

The hard-row smoke (`cqadupstack`, `scidocs`, `webis-touche2020`) showed the
same shape:

| Model | Accuracy | BalancedAcc | MacroF1 |
| --- | ---: | ---: | ---: |
| `constant_rank_prefix` | 0.4980 | 0.2500 | 0.1662 |
| `hgb_balanced` | 0.2771 | 0.2015 | 0.1917 |
| `logistic_balanced` | 0.3012 | 0.2609 | 0.2267 |

So the full result is not a full-surface artifact; the source-choice signal is
weak on the hard rows too.

## Interpretation

M1331 confirms the review diagnosis.

The useful source mixture exists. M1330 proved that cleanly. The current
problem is that the safe source choice is not sufficiently visible from the
available qrels-free query-time source/action statistics.

This means a normal next step like:

- train a source selector;
- add a threshold gate;
- add a bigger classifier over the same features;
- do another scalar source-fusion sweep;

is unlikely to produce a row-safe deployable policy. It would mostly repeat the
M1245/M1253 failure mode: oracle capacity exists, but current observability is
too weak.

## Decision

Do not train a vanilla source selector from the M1331 feature view.

Keep M1330/M1331 as a precise boundary:

- **Capacity is real**: source mixture can remove row harm under oracle choice.
- **Deployability is not solved**: current qrels-free features cannot recover
  that choice reliably.
- **Next route**: move the positive signal into source/objective construction,
  rather than putting a selector after generated sources.

## Next Valid Work

The next branch should build source choice into the generated-posting objective
or candidate construction itself:

1. Use M1330 strict source labels as a teacher only for source construction
   analysis, not as a deployed gate.
2. Distill source-family behavior into the source generator so that each source
   is less ambiguous before routing.
3. Re-run the same three-step gate before native replay:
   target/harm or source-family separability, hard-row smoke, then full
   `shared15`.
4. Reject any branch that only improves via a post-hoc selector over the same
   M1331 feature view.

## Artifacts

- Script:
  `scripts/audit_m1331_source_choice_observability.py`
- Smoke JSON:
  `runs/m1331_source_choice_observability_smoke_v1/m1331_source_choice_observability.json`
- Smoke Markdown:
  `runs/m1331_source_choice_observability_smoke_v1/m1331_source_choice_observability.md`
- Full JSON:
  `runs/m1331_source_choice_observability_v1/m1331_source_choice_observability.json`
- Full Markdown:
  `runs/m1331_source_choice_observability_v1/m1331_source_choice_observability.md`
