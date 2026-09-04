# M1157 Atom Feature Safety Policy

## Objective

M1155 showed a strong oracle signal: selecting one marginal admitted atom per
query can improve all macro metrics with clean per-dataset floors.  M1156
showed that rank/proxy/native-query features alone were not enough to deploy
that signal safely, especially because some policies still damaged MRR or
row-level MAP/NDCG.

M1157 tests the next missing piece: reconnect each marginal atom label to its
M1148/M1151 proposal atom support/impact feature vector, then run a
leave-one-dataset-out policy audit.

## Inputs

- Marginal native labels:
  `runs/m1155_marginal_atom_admission_v1/marginal_atom_replay.json`
- Proposal atom features rebuilt from the M1151/M1148 builder:
  `runs/m1157_atom_feature_safety_policy_v1/proposal_feature_rows.json`
- Query proxy score shape:
  `runs/m1151_admission_proxy_recovery_v1/admission_proxy.json`
- Native query boundary features:
  `runs/m1154_native_boundary_risk_policy_v1/native_boundary_features.json`

The rebuilt proposal feature cache has 308,703 rows and is kept under `runs/`
as an experiment cache, not as a source artifact.

## Method

For each M1155 atom replay row, M1157 joins:

- atom rank, scale, admission score
- M1148/M1151 atom proposal features
- optional query proxy-score features
- optional native boundary features

It trains LODO logistic policies for safe-gain, safe, and rank-safe labels.
Each policy can choose at most one atom/scale action per query, otherwise it
falls back to `base`.

Policy families:

- `proposal_atom`: atom context + proposal feature vector
- `proposal_proxy`: `proposal_atom` + proxy score-shape features
- `proposal_native`: `proposal_atom` + native query boundary features
- `proposal_proxy_native`: all features

## Results

Dataset surface:

- query_count: 149
- atom_count: 2,370
- safe_rate: 0.780591
- safe_gain_rate: 0.146414
- rank_safe_rate: 0.965401

Oracle remains the upper bound:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `oracle_single` | +0.001118 | +0.019178 | +0.005896 | +0.008171 | +0.005705 | clean |

Best row-floor-clean non-oracle policy:

| Policy | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Row floor |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `proposal_proxy_guarded_g0.50_s0.50_r0.70` | +0.000051 | +0.000551 | +0.000370 | +0.001220 | +0.003356 | clean |

This policy accepts 74 non-base actions:

```text
a1_s0.01:5, a1_s0.05:6, a2_s0.01:2, a2_s0.05:4,
a3_s0.01:11, a3_s0.05:18, a4_s0.01:8, a4_s0.05:11,
a5_s0.05:5, a6_s0.01:1, a6_s0.05:2, a7_s0.05:1,
base:75
```

The larger-gain `proposal_proxy_native` policies are not row-floor clean:

| Policy | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | Key failure |
| --- | ---: | ---: | ---: | ---: | --- |
| `proposal_proxy_native_guarded_g0.30_s0.50_r0.70` | +0.006863 | +0.000105 | +0.000865 | +0.003356 | dbpedia/fiqa/webis row regressions |

## Interpretation

M1157 is a real positive update over M1156:

1. A non-oracle row-floor-clean policy now exists.
2. The best clean policy improves all five macro metrics.
3. MRR is no longer the main blocker for the best clean policy.
4. Proposal atom features are useful; M1156 did not have enough atom-specific
   information.

The gain is still much smaller than the oracle:

- clean deployed policy Recall gain: +0.000551
- oracle Recall gain: +0.019178
- clean deployed policy MAP gain: +0.000370
- oracle MAP gain: +0.005896

So the current classifier recovers a safe but small slice of the available
single-atom opportunity.

The failed high-gain policies show the remaining bottleneck.  Adding native
query boundary features increases macro Recall but lets in row regressions.
That means the missing variable is not another query-level gate.  The risk is
candidate-slot local: an atom can be globally plausible for a query but still
move the wrong boundary document for a specific row.

## Conclusion

Keep M1157 as a meaningful improvement and as the new conservative atom-policy
baseline.  It proves that atom-specific proposal features can convert part of
the M1155 oracle into a deployable, row-floor-clean policy.

Do not keep pressing the same query-level gate family for the next step.  To
capture more of the oracle gain, the next useful line should add candidate-slot
or pair-local risk features, following the earlier M650 lesson: predict whether
an admitted atom can safely replace a specific top100/topK boundary slot, then
replay only those safe local movements.
