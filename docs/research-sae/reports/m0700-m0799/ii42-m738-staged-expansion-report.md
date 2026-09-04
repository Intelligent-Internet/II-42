# M738 Staged Expansion Report

M738 tested whether the M736/M737 productive atom signal can be made more
deployable by ranking atoms inside each query instead of selecting atoms with
independent global thresholds.

## Stage 1: Canary Result

The first M738 canary used expanded M736B single-atom replay rows on
`fiqa,arguana,scidocs,cqadupstack`.

The query-internal pairwise ranker found a positive holdout signal:

| Policy | Split | Rows | Productive | dMAP | dNDCG | dCUB | dO@100 | dO@256 | Gate |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pairwise_damage_penalty_top1_s0.005` | `holdout` | 29 | 0.068966 | +0.000398 | +0.000209 | +0.000000 | +0.001034 | +0.000000 | 1 |
| `pairwise_damage_penalty_top1_s0.005` | `full` | 200 | 0.120000 | +0.000500 | -0.000025 | -0.001981 | -0.000450 | -0.000117 | 0 |

The canary result is useful but not sufficient.  It shows that query-internal
ranking can sometimes pick productive atoms, but the same policy already fails
the full canary aggregate because it spends candidate upper bound and dense
overlap.

## Stage 2: Shared8 Atom Replay

To test whether the signal expands, the atom replay was rerun on:

`fiqa,arguana,scidocs,cqadupstack,nfcorpus,scifact,trec-covid,webis-touche2020`

with `limit_queries=30`.

Outputs:

- Rows: `runs/m738b_shared8_atom_replay_v1/m735a_rows.jsonl`
- Summary: `runs/m738b_shared8_atom_replay_v1/m735a_summary.json`
- Root report: `docs/research-sae/reports/m0700-m0799/ii42-m738b-shared8-atom-replay-report.md`

The shared8 replay produced:

| Surface | Split | Rows | Safe | Productive | Danger | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head_risk_atoms_s0.005` | `full` | 960 | 0.617708 | 0.092708 | 0.382292 | +0.000109 | -0.000389 | -0.000420 | -0.000438 | -0.000160 | -0.000115 | -0.000081 |
| `head_risk_atoms_s0.005` | `holdout` | 160 | 0.568750 | 0.131250 | 0.431250 | +0.002092 | +0.000192 | +0.001090 | -0.000112 | -0.000314 | +0.000063 | -0.000659 |
| `source_atoms_s0.005` | `full` | 1920 | 0.719792 | 0.073958 | 0.280208 | +0.000266 | -0.000255 | -0.000426 | -0.000085 | -0.000118 | -0.000266 | +0.000075 |
| `source_atoms_s0.005` | `holdout` | 320 | 0.728125 | 0.084375 | 0.271875 | +0.001051 | -0.000019 | +0.000418 | -0.000099 | -0.000066 | -0.000000 | -0.000183 |

This confirms the atom teacher still finds productive atoms on broader data,
but average row-level effects are not clean enough to deploy directly.

## Stage 3: Shared8 Separability Gate

M735B was trained on the shared8 rows.

Output:

- Summary: `runs/m738b_shared8_signal_v1/m735b_summary.json`
- Root report: `docs/research-sae/reports/m0700-m0799/ii42-m738b-shared8-signal-report.md`

The interface gate failed:

| Surface | Label | Train AUC | Holdout AUC | Holdout + | Pred + | TP | Precision | Recall |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `combined` | `productive_atom` | 0.760177 | 0.685378 | 48 | 70 | 14 | 0.200000 | 0.291667 |
| `source_atoms_s0.005` | `productive_atom` | 0.761645 | 0.711794 | 27 | 45 | 8 | 0.177778 | 0.296296 |
| `head_risk_atoms_s0.005` | `productive_atom` | 0.768865 | 0.610483 | 21 | 4 | 0 | 0.000000 | 0.000000 |

`safe_atom` and `danger_atom` remain moderately separable, but the productive
atom selector does not generalize.  No `productive_atom` selected model was
emitted.  M738 therefore cannot run the damage-penalty replay on shared8 from
this selector surface.

The attempted M738 replay failed for the correct reason:

```text
TypeError: 'NoneType' object is not subscriptable
```

The missing object is the unselected `productive_atom` model.  This should be
treated as a stop signal for the current expansion, not as an invitation to
force the canary selector onto broader data.

## Diagnosis

M738 is not a dead end, but the current teacher/interface is not broad enough.

The useful observation is that the blocker moved:

- M736 proved positive-enriched oracle bundles exist.
- M737 proved damage filters can remove some harm but lose productive holdout.
- M738 proved query-internal ranking can improve atom selection on a canary.
- Shared8 shows productive atom separability is unstable outside the canary.

The failure is no longer simply thresholding, bundle size, or damage filtering.
The bottleneck is productive atom ranking under broader row distributions.

Positive holdout atoms are also unevenly distributed across datasets.  In the
shared8 slice, `nfcorpus`, `trec-covid`, `scidocs`, and `webis-touche2020`
provide productive holdout signal, while several rows have little or no
holdout productive evidence.  A global selector trained on this surface can
easily learn dataset/surface artifacts rather than atom utility.

## Decision

Do not expand M738 to full shared15 yet.

Preserve the M738 canary as evidence that query-internal atom ranking is a
valid subproblem, but redesign the productive atom teacher before the next
larger run.

## Next Experiment

The next useful experiment should be M739:

1. Build a richer productive atom teacher from per-query positive movement,
   not only row-level `productive_atom`.
2. Add rank-position labels: whether an atom moves a qrels-positive document
   across top100/top256 boundaries, and whether it rescues dense-missed
   positives without losing dense support.
3. Train a query-internal listwise ranker with group-normalized labels, so
   each query only compares atoms from the same query and avoids dataset
   prevalence bias.
4. Keep the deployable policy fixed and small: top1 only, scale `0.005`,
   dense-equivalence hard gate.
5. Re-test on shared8 before full shared15.

Stop if M739 cannot improve productive atom hit@1/hit@2 on shared8 holdout
without reducing O@100/O@256 or CUB.
