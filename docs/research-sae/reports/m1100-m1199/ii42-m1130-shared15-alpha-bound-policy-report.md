# ii42 M1130 Alpha-Bound Policy Audit

Verdict: `reject_feature_policy_keep_fixed_band`

## Train-Selected Policies

- Fixed alpha by train score: `fixed_alpha_0.75`
- Feature policy by train score: `lex_atom_corr_le_0.551908_0.60_0.75`

### Feature Policy

```json
{"name": "lex_atom_corr_le_0.551908_0.60_0.75", "low_alpha": 0.6, "high_alpha": 0.75, "feature": "lex_atom_corr", "op": "le", "threshold": 0.5519077091661011}
```

## Eval Macro

| Policy | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Alpha counts |
| --- | ---: | ---: | ---: | ---: | --- |
| `fixed_alpha_0.00` | 0.746410 | 0.742393 | 0.669888 | 0.563245 | 0.000000:403 |
| `fixed_alpha_0.25` | 0.766004 | 0.764444 | 0.696505 | 0.590268 | 0.250000:403 |
| `fixed_alpha_0.50` | 0.770982 | 0.777186 | 0.699337 | 0.604800 | 0.500000:403 |
| `fixed_alpha_0.60` | 0.772398 | 0.779772 | 0.699965 | 0.606185 | 0.600000:403 |
| `fixed_alpha_0.75` | 0.771468 | 0.763056 | 0.689540 | 0.600224 | 0.750000:403 |
| `lex_atom_corr_le_0.551908_0.60_0.75` | 0.771531 | 0.764168 | 0.689995 | 0.600557 | 0.600000:23, 0.750000:380 |
| `query_metric_oracle` | 0.780820 | 0.808073 | 0.734918 | 0.639374 | 0.000000:241, 0.250000:39, 0.300000:15, 0.500000:28, 0.600000:16, 0.750000:64 |

## Selected Policy Deltas

| Baseline | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 | Damaged datasets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `fixed_alpha_0.00` | +0.025122 | +0.021775 | +0.020107 | +0.037311 | 7 |
| `fixed_alpha_0.50` | +0.000550 | -0.013018 | -0.009342 | -0.004244 | 13 |
| `fixed_alpha_0.60` | -0.000867 | -0.015604 | -0.009971 | -0.005628 | 13 |

## Selected Damage Versus Fixed Alpha 0.60

| Dataset | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | +0.000000 | -0.005081 | -0.013491 | -0.005416 |
| `climate-fever` | +0.000000 | +0.002315 | -0.000312 | +0.006787 |
| `cqadupstack` | -0.009483 | -0.015966 | -0.017553 | -0.011565 |
| `dbpedia-entity` | -0.003813 | -0.052778 | -0.034048 | -0.027810 |
| `fiqa` | -0.022222 | -0.017460 | -0.004343 | -0.008893 |
| `hotpotqa` | +0.000000 | +0.000000 | -0.005383 | -0.010491 |
| `msmarco` | +0.000646 | +0.000000 | -0.011646 | +0.000822 |
| `nfcorpus` | +0.001550 | -0.014803 | -0.006280 | -0.001061 |
| `nq` | +0.000000 | -0.023333 | -0.020364 | -0.028275 |
| `quora` | +0.000000 | -0.016667 | -0.016220 | -0.018435 |
| `scidocs` | +0.000000 | -0.049497 | -0.018457 | -0.013131 |
| `scifact` | +0.000000 | -0.001905 | -0.001937 | -0.002138 |
| `trec-covid` | +0.004055 | -0.038889 | -0.002260 | +0.020172 |

## Interpretation

The selected global inference-time feature policy does not beat the fixed alpha 0.60 heldout operating point on all macro metrics. This means M1129 should currently be kept as a bounded fixed-alpha band result, not promoted into another selector-training loop.
