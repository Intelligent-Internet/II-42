# M733 Retrieval-Conditioned Posting Compiler Report

Status: `m733a_canary_failed_do_not_expand`

M733 tests whether a retrieval-conditioned compiler can admit query-local
unified-posting deltas while keeping the `P1.3 / M549U native signed-dot`
document posting geometry frozen.  The output remains native posting behavior:
atom delta/admission/gate, not a plain reranker score.

The first executable probe is M733A:

- source-aware atom interface derived from the M697 `support_atoms384` line;
- deterministic fixed small atom delta;
- one global query-level admission gate;
- no dataset-specific tuning;
- no BM25 alpha search;
- qrels used only to build/report oracle-derived labels, not runtime features.

Artifacts:

- Evidence design:
  `docs/research-sae/reports/m0700-m0799/ii42-m733-evidence-synthesis-and-design-report.md`
- Teacher readiness:
  `docs/research-sae/reports/m0700-m0799/ii42-m733a-teacher-surface-readiness-report.md`
- Canary JSON:
  `runs/m733a_source_aware_fixed_delta_canary_v1/m733a_summary.json`
- Canary report:
  `runs/m733a_source_aware_fixed_delta_canary_v1/m733a_report.md`

## Canary Surface

Datasets:

`fiqa,arguana,scidocs,cqadupstack`

M733A completed in `73.754` seconds.

## Gate Fit

| Split | Rows | Positives | AUC | Precision | Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| train | 330 | 46 | 0.820269 | n/a | n/a |
| holdout | 70 | 13 | 0.919028 | 0.800000 | 0.615385 |

The selector has measurable signal.  The failure is therefore not simply that
retrieval-conditioned features are unlearnable.  The deployment failure comes
from the admitted fixed delta not satisfying the hard native gates.

## Full Canary Matrix

| Source | Queries | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | O@100 | O@256 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 400 | 0.897022 | 0.542388 | 0.620497 | 0.679765 | 0.975815 | 0.468200 | 0.493857 | 0.000000 |
| m674 | 400 | 0.898212 | 0.542483 | 0.620497 | 0.679765 | 0.975815 | 0.474775 | 0.453701 | 1.000000 |
| m733a_fixed_all | 400 | 0.895254 | 0.543744 | 0.621114 | 0.681826 | 0.975315 | 0.465875 | 0.493926 | 1.000000 |
| m733a_fixed_gate | 400 | 0.896522 | 0.542518 | 0.620197 | 0.680195 | 0.974815 | 0.467875 | 0.493887 | 0.115000 |

## Full Delta vs Baseline

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m674 | +0.001190 | +0.000094 | +0.000000 | +0.000000 | +0.000000 | +0.006575 | -0.040156 | 1.000000 |
| m733a_fixed_all | -0.001768 | +0.001356 | +0.000617 | +0.002061 | -0.000500 | -0.002325 | +0.000068 | 1.000000 |
| m733a_fixed_gate | -0.000500 | +0.000130 | -0.000301 | +0.000431 | -0.001000 | -0.000325 | +0.000029 | 0.115000 |

## Holdout Matrix

| Source | Queries | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | O@100 | O@256 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 70 | 0.902108 | 0.567625 | 0.637714 | 0.673592 | 0.976905 | 0.457714 | 0.487109 | 0.000000 |
| m674 | 70 | 0.899251 | 0.567596 | 0.637714 | 0.673592 | 0.976905 | 0.464143 | 0.450725 | 1.000000 |
| m733a_fixed_all | 70 | 0.896394 | 0.567895 | 0.635338 | 0.672812 | 0.976905 | 0.454857 | 0.487779 | 1.000000 |
| m733a_fixed_gate | 70 | 0.899251 | 0.567767 | 0.637763 | 0.673116 | 0.976905 | 0.457286 | 0.487221 | 0.142857 |

## Holdout Delta vs Baseline

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 | Apply |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m674 | -0.002857 | -0.000029 | +0.000000 | +0.000000 | +0.000000 | +0.006429 | -0.036384 | 1.000000 |
| m733a_fixed_all | -0.005714 | +0.000270 | -0.002377 | -0.000780 | +0.000000 | -0.002857 | +0.000670 | 1.000000 |
| m733a_fixed_gate | -0.002857 | +0.000142 | +0.000049 | -0.000476 | +0.000000 | -0.000429 | +0.000112 | 0.142857 |

## Failure Classes

Holdout `m733a_fixed_gate`:

```json
{
  "accepted": 1,
  "o100_regression": 4,
  "o256_regression": 3,
  "retrieval_metric_negative": 2,
  "selector_abstained": 60
}
```

## Gate Decision

Do not expand M733A to shared15.

M733A fails the required canary gate:

- `Recall@100` regresses on holdout by `-0.002857`;
- `O@100` regresses by `-0.000429`;
- `MRR@20` regresses by `-0.000476`;
- `CUB` is flat on holdout but drops by `-0.001000` on full canary;
- the gate applies to only `14.2857%` of holdout queries;
- only one holdout query is classified as accepted after failure analysis.

The small positive MAP movement is not enough to override the hard gate
violations.  This is exactly the case the M733 objective rejects: a retrieval
metric improvement bought with head/overlap instability.

## Interpretation

M733A gives a useful negative result, not a route failure:

1. Retrieval-conditioned features can predict oracle-style labels better than
   chance (`holdout AUC 0.919028`).
2. The fixed source-aware delta family is still too blunt: applying it to all
   queries improves MAP/MRR but loses Recall/O@100/CUB, while gating mostly
   abstains and still leaves Recall/O@100 loss.
3. The next bottleneck is delta family and source-risk calibration, not just
   classifier capacity or training duration.

This result is consistent with the earlier M684/M685 warning: selecting safe
queries is not enough if the admitted delta basis is wrong.

## M733B Source-Risk Calibrated Delta

M733B was implemented as:

`scripts/train_m733b_source_risk_calibrated_delta_canary.py`

It reuses the M733A source-aware atom interface and global gate, then evaluates
scale variants and a rule-audited source-risk scale policy.

Artifacts:

- JSON:
  `runs/m733b_source_risk_delta_canary_v1/m733b_summary.json`
- Markdown:
  `runs/m733b_source_risk_delta_canary_v1/m733b_report.md`

M733B improves the shape relative to M733A but still does not clear the hard
gate:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m733b_s0005_gate | +0.000000 | +0.000192 | +0.000087 | +0.000000 | +0.000000 | -0.000286 | +0.000112 |
| m733b_risk_gate | +0.000000 | +0.000094 | -0.000062 | -0.000476 | +0.000000 | -0.000429 | +0.000112 |

Decision: do not expand M733B to shared15.  The best conservative scale keeps
Recall/MAP/NDCG/MRR/CUB flat or positive, but still spends O@100.

## M733C Quota Smoke

Before designing a full quota policy, two canary-only quota smokes were run
with the same M733B script:

- quota 2:
  `runs/m733c_quota2_source_risk_delta_canary_v1/m733c_quota2_summary.json`
- quota 4:
  `runs/m733c_quota4_source_risk_delta_canary_v1/m733c_quota4_summary.json`

Holdout trend:

| Quota | Source | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | m733b_s0005_gate | -0.002857 | +0.000004 | +0.000017 | +0.000000 | +0.000000 | +0.000000 | +0.000223 |
| 4 | m733b_s001_gate | +0.000000 | +0.000193 | +0.000087 | +0.000000 | +0.000000 | -0.000143 | -0.000167 |
| 8 | m733b_s0005_gate | +0.000000 | +0.000192 | +0.000087 | +0.000000 | +0.000000 | -0.000286 | +0.000112 |

Interpretation:

- Smaller quota reduces head/overlap risk, but quota 2 loses Recall.
- Quota 4/8 preserve Recall and improve MAP/NDCG, but still lose O@100 and
  sometimes O@256.
- Therefore query-level quota/scale is not enough.  The next knob cannot be
  just "smaller delta"; it must be atom-level head-risk selection.

## Current Decision

Do not run large/deep compiler training yet, and do not expand to shared15.

M733A/B/C-smoke collectively show:

1. retrieval-conditioned selector features are learnable;
2. source-aware atoms can move ranking metrics in the right direction;
3. global scale/quota cannot eliminate O@100/O@256 loss;
4. the next bottleneck is atom-level source/head-risk filtering, not query-level
   classifier capacity or training depth.

## Next Step

The next controlled probe should be `M733C proper: atom-level head-risk
selection`, still canary-only:

- keep frozen P1.3/M549U doc geometry;
- keep the same source-aware atom candidates;
- train or audit an atom-level accept/drop policy before quota selection;
- features must be inference-time only:
  - same/opposite sign against query atoms;
  - boundary atom collision;
  - P1/BM25 visibility channel;
  - source channel and fanout/IDF-like statistics;
  - contribution to protected head atoms;
- output remains query-local posting delta over the accepted atom subset;
- preserve the same hard gates before any shared15 expansion.

Stop this line if atom-level filtering cannot keep O@100/O@256/CUB flat while
retaining the quota-4/8 MAP/NDCG gains.

## M733C Atom-Level Head-Risk Selection

M733C was implemented as:

`scripts/train_m733c_atom_head_risk_selection_canary.py`

It reuses the M733B native evaluator and selector, but replaces the atom
selection stage with an inference-time head-risk filter before quota selection.
The filter rejects boundary atoms and unsupported/opposite/negative-risk atom
candidates.  Output remains query-local unified posting delta.

Artifacts:

- JSON:
  `runs/m733c_atom_head_risk_selection_canary_v1/m733c_summary.json`
- Markdown:
  `runs/m733c_atom_head_risk_selection_canary_v1/m733c_report.md`

Policy counters:

```json
{
  "accepted_atoms": 297602,
  "candidate_atoms": 307202,
  "queries": 800,
  "reject_boundary_atom": 9600,
  "selected_atoms": 3200
}
```

M733C strict filter therefore does reject real atoms.  The remaining failure is
not that the policy is a no-op.

Holdout deltas:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m733b_s0005_gate | +0.000000 | -0.000003 | +0.000041 | -0.000752 | +0.000000 | -0.000714 | +0.000391 |
| m733b_risk_gate | +0.000000 | -0.000020 | +0.000041 | -0.000752 | +0.000000 | -0.000714 | +0.000391 |

Decision: do not expand M733C to shared15.

Interpretation:

- Boundary-atom rejection alone does not fix O@100.
- The remaining head loss comes from non-boundary atoms that still alter dense
  top100 membership.
- Query-level gate AUC remains high, but accepted rows still include O@100 and
  retrieval-metric regressions.
- This rules out another simple hand-written atom rule as the next mainline.

## Updated Next Step

Before any deeper compiler or shared15 expansion, run a focused
`M733D-audit accepted-delta failure analysis`:

- collect per-query accepted/rejected delta rows for M733B and M733C;
- record selected atom IDs, source features, risk parts, scale, gate score, and
  per-query metric/dense deltas;
- classify O@100 loss rows by atom/source signature;
- compare accepted positive rows versus O@100-regression rows;
- decide whether there is a learnable inference-time atom risk target.

Only if this audit finds a separable atom/source signature should the next
model be trained.  If O@100 loss cannot be predicted without post-hoc dense or
qrels information, the M733 compiler line should stop at the current canary
surface rather than moving to shared15.

## M733D Accepted-Delta Failure Audit

M733D was implemented as:

`scripts/audit_m733d_accepted_delta_failure.py`

It replays M733B and M733C on the canary surface and records per-query selected
atoms, atom/source features, risk scores, gate scores, metric deltas, and
dense-overlap deltas.  It does not train a new compiler.

Artifacts:

- Query rows:
  `runs/m733d_accepted_delta_failure_audit_v1/m733d_query_rows.jsonl`
- JSON:
  `runs/m733d_accepted_delta_failure_audit_v1/m733d_summary.json`
- Markdown:
  `runs/m733d_accepted_delta_failure_audit_v1/m733d_report.md`

Policy summary:

| Policy | Rows | Applied | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 | dO@256 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| m733b_source_atoms | 400 | 46 | +0.000500 | +0.000128 | -0.000357 | +0.000437 | -0.000500 | -0.000100 | -0.000049 |
| m733c_head_risk_atoms | 400 | 92 | -0.000500 | +0.000128 | +0.000087 | +0.000954 | +0.000000 | -0.000225 | -0.000010 |

Separability evidence:

| Policy | Applied | O@100 loss rows | Safe accepted rows | Best O@100 AUC | Best accepted AUC |
| --- | ---: | ---: | ---: | ---: | ---: |
| m733b_source_atoms | 46 | 12 | 12 | 0.639706 | 0.580882 |
| m733c_head_risk_atoms | 92 | 22 | 30 | 0.609091 | 0.572043 |

The strongest inference-time atom/source features are weak.  They are not close
to the level required to train a safe selector:

- M733B best O@100-loss signal is `boundary_count` with AUC `0.639706`.
- M733C best O@100-loss signal is `p1_visible_mean` with AUC `0.609091`.
- Accepted-row signals are also weak: best AUC is only `0.580882` for M733B and
  `0.572043` for M733C.

Decision:

Do not train a deeper compiler from the current M733A/B/C surface.  Do not
expand to shared15.

This triggers the M733 stop condition for the current interface: safe movement
is still not predictable well enough from the available inference-time
atom/source features.  The route should either redesign the teacher/interface
or stop this compiler branch at canary evidence.

## Final Current Recommendation

Do not continue with another scale/quota/filter tweak on the same source-aware
atom interface.

The next valuable work is not `M733E` as a deeper model.  It would need one of
these redesigns first:

1. a richer inference-time interaction surface that can expose why a delta
   changes dense top100 membership;
2. a new teacher that directly supplies safe atom-level labels without post-hoc
   dense/qrels gates at inference;
3. a product-path fallback that treats P1.3/M549U as frozen and does not try to
   learn query-local deltas from this candidate interface.

Until one of those is available, M733 should remain a documented negative
canary branch rather than being promoted to shared15 or deep training.

## Per-Dataset Canary Matrix

The machine-readable JSON artifacts contain the full per-dataset rows.  The
tables below summarize the principal gate source from each stage on the full
canary surface.

### M733A: `m733a_fixed_gate`

| Dataset | Queries | R@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | O@10 | O@50 | O@100 | O@256 | Apply | Failure classes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| fiqa | 100 | 0.944833 | 0.665874 | 0.715102 | 0.765977 | 0.993333 | 0.495000 | 0.455200 | 0.472800 | 0.518516 | 0.000000 | `{"selector_abstained":100}` |
| arguana | 100 | 1.000000 | 0.496414 | 0.600937 | 0.495997 | 1.000000 | 0.747000 | 0.562600 | 0.477800 | 0.458359 | 0.000000 | `{"selector_abstained":100}` |
| scidocs | 100 | 0.697500 | 0.324203 | 0.416001 | 0.633483 | 0.911000 | 0.484000 | 0.413000 | 0.413700 | 0.447969 | 0.460000 | `{"accepted":8,"o100_regression":17,"o256_regression":14,"retrieval_metric_negative":7,"selector_abstained":54}` |
| cqadupstack | 100 | 0.943755 | 0.683583 | 0.748746 | 0.825324 | 0.994928 | 0.516000 | 0.507000 | 0.507200 | 0.550703 | 0.000000 | `{"selector_abstained":100}` |
| macro | 400 | 0.896522 | 0.542518 | 0.620197 | 0.680195 | 0.974815 | 0.560500 | 0.484450 | 0.467875 | 0.493887 | 0.115000 | `{"accepted":8,"o100_regression":17,"o256_regression":14,"retrieval_metric_negative":7,"selector_abstained":354}` |

### M733B: `m733b_s0005_gate`

| Dataset | Queries | R@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | O@10 | O@50 | O@100 | O@256 | Apply | Failure classes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| fiqa | 100 | 0.944833 | 0.665874 | 0.715102 | 0.765977 | 0.993333 | 0.495000 | 0.455200 | 0.472800 | 0.518516 | 0.000000 | `{"selector_abstained":100}` |
| arguana | 100 | 1.000000 | 0.496414 | 0.600937 | 0.495997 | 1.000000 | 0.747000 | 0.562600 | 0.477800 | 0.458359 | 0.000000 | `{"selector_abstained":100}` |
| scidocs | 100 | 0.701500 | 0.324641 | 0.416114 | 0.633842 | 0.913000 | 0.486000 | 0.412400 | 0.414700 | 0.447656 | 0.460000 | `{"accepted":12,"cub_regression":1,"o100_regression":12,"o256_regression":15,"retrieval_metric_negative":6,"selector_abstained":54}` |
| cqadupstack | 100 | 0.943755 | 0.683583 | 0.748746 | 0.825324 | 0.994928 | 0.516000 | 0.507000 | 0.507200 | 0.550703 | 0.000000 | `{"selector_abstained":100}` |
| macro | 400 | 0.897522 | 0.542628 | 0.620225 | 0.680285 | 0.975315 | 0.561000 | 0.484300 | 0.468125 | 0.493809 | 0.115000 | `{"accepted":12,"cub_regression":1,"o100_regression":12,"o256_regression":15,"retrieval_metric_negative":6,"selector_abstained":354}` |

### M733C: `m733b_risk_gate`

| Dataset | Queries | R@100 | MAP@100 | NDCG@10 | MRR@20 | CUB | O@10 | O@50 | O@100 | O@256 | Apply | Failure classes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| fiqa | 100 | 0.944833 | 0.665874 | 0.715102 | 0.765977 | 0.993333 | 0.495000 | 0.455000 | 0.472800 | 0.518398 | 0.030000 | `{"accepted":1,"o100_regression":1,"o256_regression":1,"selector_abstained":97}` |
| arguana | 100 | 1.000000 | 0.496414 | 0.600937 | 0.495997 | 1.000000 | 0.747000 | 0.562600 | 0.477800 | 0.458359 | 0.000000 | `{"selector_abstained":100}` |
| scidocs | 100 | 0.697500 | 0.324381 | 0.416712 | 0.636411 | 0.915000 | 0.482000 | 0.414400 | 0.414300 | 0.447734 | 0.770000 | `{"accepted":23,"cub_regression":1,"o100_regression":18,"o256_regression":20,"retrieval_metric_negative":15,"selector_abstained":23}` |
| cqadupstack | 100 | 0.943755 | 0.683394 | 0.749583 | 0.824491 | 0.994928 | 0.517000 | 0.506400 | 0.507000 | 0.550898 | 0.120000 | `{"accepted":6,"o100_regression":3,"o256_regression":2,"retrieval_metric_negative":1,"selector_abstained":88}` |
| macro | 400 | 0.896522 | 0.542516 | 0.620584 | 0.680719 | 0.975815 | 0.560250 | 0.484600 | 0.467975 | 0.493848 | 0.230000 | `{"accepted":30,"cub_regression":1,"o100_regression":22,"o256_regression":23,"retrieval_metric_negative":16,"selector_abstained":308}` |

This confirms the route is not broadly deployable: the selector mostly abstains
outside `scidocs`, and when it does apply it still produces O@100/O@256 failure
classes.  Treating this as a shared15 candidate would be overclaiming.
