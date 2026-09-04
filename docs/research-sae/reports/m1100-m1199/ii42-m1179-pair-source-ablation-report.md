# M1179 Pair-Source Ablation

M1179 reruns the M1178 shallow LODO baseline with different pair sources to
localize whether M1178 failed because of source mixing or because the shallow
feature/scorer shape is insufficient.

Artifacts:

- Script: `scripts/audit_m1179_pair_source_ablation.py`
- JSON: `runs/m1179_pair_source_ablation_v1/pair_source_ablation.json`
- Summary: `runs/m1179_pair_source_ablation_v1/summary.md`

## Result

| Variant | Pair count | Clean | dCUB | dRecall | dMAP | dNDCG | dMRR |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| all_pairs | 44,184 | false | +0.000000 | -0.000036 | -0.001153 | -0.007266 | +0.010757 |
| balanced_sources | 37,034 | false | +0.000000 | -0.000024 | -0.000790 | -0.007357 | +0.010757 |
| harm_only | 25,667 | false | +0.000000 | -0.007282 | -0.081345 | -0.095011 | -0.111232 |
| rank_only | 18,517 | false | +0.000000 | +0.000004 | -0.097349 | -0.115405 | -0.089245 |

## Interpretation

The failure is not caused only by mixing harm penalties with rank-teacher
positives.  `rank_only` is also strongly negative.  Therefore the shallow
pairwise scorer cannot preserve the protected direct top-rank geometry.

Decision:

- Stop scaling linear/scikit-style pairwise rerankers for this branch.
- Keep M1177 supervision as training evidence.
- Next viable branch must train a compiler/output-shape objective that changes
  how query/doc posting scores are produced, with direct/P1 floors baked into
  the loss.
