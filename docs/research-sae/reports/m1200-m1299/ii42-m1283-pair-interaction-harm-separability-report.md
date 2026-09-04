# M1283 Pair-Interaction Harm Separability Audit

## Question

M721b/M722 kept the pair-interaction route alive: it improved dense
overlap@100, Recall@100, and CUB on the correct P1.3 native surface, while
slightly hurting NDCG@10, MAP@100, and MRR@20.

M1283 asks the next route-control question before any further training:

Can M721b safe-gain queries be separated from rank-harm/tradeoff queries using
query-time selected-source features?

This is an observability audit, not a native replay.

## Surface

Hard-row smoke:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Variant:

- `pair_hgb_rp1_b64_s0.02`

Inputs:

- M722 per-query metric deltas:
  `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1`
- P1.3 query atoms:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- Pair-boundary training rows:
  `runs/m721_dense_boundary_training_rows_shared15_v1/m653_dense_boundary_training_rows.jsonl`

## Result

Outcome buckets over 248 query rows:

| Bucket | Count | Mean dNDCG | Mean dMAP | Mean dRecall | TargetVis | NegShare |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `safe_gain` | 95 | +0.001726 | +0.000397 | +0.000405 | 0.8996 | 0.0038 |
| `tradeoff` | 22 | -0.005487 | -0.002302 | -0.001031 | 0.8863 | 0.0000 |
| `rank_harm` | 30 | -0.002344 | -0.003510 | +0.000000 | 0.9164 | 0.0000 |
| `neutral` | 101 | +0.000077 | +0.000159 | +0.000000 | 0.8925 | 0.0006 |

LODO separability:

| Feature group | Label | QueryCount | PositiveShare | AUC |
| --- | --- | ---: | ---: | ---: |
| observable | rank_harm | 248 | 0.2097 | 0.4026 |
| observable | safe_gain | 248 | 0.3831 | 0.5421 |
| observable | tradeoff_among_gain | 117 | 0.1880 | 0.4852 |
| diagnostic | rank_harm | 248 | 0.2097 | 0.4102 |
| diagnostic | safe_gain | 248 | 0.3831 | 0.5329 |
| diagnostic | tradeoff_among_gain | 117 | 0.1880 | 0.4923 |

Best individual observable feature AUCs are weak:

| Label | Best feature | Direction | AUC |
| --- | --- | ---: | ---: |
| rank_harm | `rank_min` | - | 0.5723 |
| safe_gain | `pair_pair_candidate_align_max` | + | 0.6020 |
| tradeoff_among_gain | `abs_impact_min` | + | 0.5919 |

## Interpretation

M1283 is a stop signal for gate/filter micro-tuning on the exact M721b
candidate.

The important observation is that even diagnostic teacher labels do not
separate harm.  `tradeoff` and `rank_harm` queries do not show higher selected
negative-only share.  In fact, their selected negative share is zero on this
smoke surface.  That means the M722 rank loss is not a simple "selected risky
atom" problem that can be fixed by a local veto.

The route remains informative because M721b/M722 showed real movement, but the
current selected-source feature view does not expose a deployable safe/harm
split.

## Decision

Do not run a full shared15 M1283 replay or classifier.

Do not continue with:

- more thresholds over M721b selected atoms;
- negative-only atom vetoes;
- shallow safe-gain/rank-harm query gates using the current feature family;
- longer training of the same pair-interaction candidate.

Next valid work must change the source/interface or compiler architecture.
Specifically, the next branch should not ask "which selected M721b atoms should
we veto?", but instead "can we construct a source whose atoms are intrinsically
rank-preserving before selection?"

## Artifacts

- Script:
  `scripts/audit_m1283_pair_interaction_harm_separability.py`
- JSON:
  `runs/m1283_pair_interaction_harm_separability_smoke_v1/m1283_pair_interaction_harm_separability.json`
- Markdown:
  `runs/m1283_pair_interaction_harm_separability_smoke_v1/m1283_pair_interaction_harm_separability.md`
