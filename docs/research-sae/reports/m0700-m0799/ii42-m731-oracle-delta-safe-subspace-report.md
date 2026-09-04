# M731 Oracle Delta Safe-Subspace Audit

Status: `weak_aggregate_safe_subspace_only`

M731 audits whether the positive M653G per-query ridge deltas have a
global low-rank safe subspace, and whether that subspace is predictable
from query features.  It does not train a deployable compiler and does
not use BM25, reranker, learned gates, or qrels-driven objectives.

## Split Counts

```json
{
  "dev": 71,
  "test": 54,
  "train": 275
}
```

## Explained Delta Variance

```json
{
  "r1": 0.1570002191073817,
  "r128": 0.8999654141622087,
  "r16": 0.557768560816305,
  "r2": 0.28141387007133456,
  "r256": 0.9956648419730091,
  "r32": 0.6604222605006906,
  "r4": 0.39432308784199493,
  "r64": 0.7754202621656278,
  "r8": 0.46768831257016014
}
```

## Heldout Gate Deltas

| Source | Gate | dO@100 | dO@256 | dCUB | dR@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `pca_oracle_r128` | `passed` | 0.000320 | 0.001250 | 0.000000 | 0.000000 |
| `pca_oracle_r16` | `failed` | -0.000400 | 0.000250 | 0.000000 | 0.000000 |
| `pca_oracle_r32` | `failed` | -0.000080 | 0.000469 | 0.000000 | 0.000000 |
| `pca_oracle_r4` | `failed` | -0.001120 | -0.000469 | 0.000000 | -0.001600 |
| `pca_oracle_r64` | `passed` | 0.000080 | 0.000906 | 0.000000 | 0.000000 |
| `pca_oracle_r8` | `failed` | -0.000960 | 0.000125 | 0.000000 | -0.001600 |
| `pred_r128_lambda0.1` | `failed` | -0.001120 | -0.000500 | 0.000000 | 0.000000 |
| `pred_r128_lambda1` | `failed` | -0.001120 | -0.000500 | 0.000000 | 0.000000 |
| `pred_r128_lambda10` | `failed` | -0.001200 | -0.000625 | 0.000000 | 0.000000 |
| `pred_r128_lambda100` | `failed` | -0.000960 | -0.000500 | 0.000000 | 0.000000 |
| `pred_r16_lambda0.1` | `failed` | -0.001200 | -0.000250 | 0.000000 | 0.000000 |
| `pred_r16_lambda1` | `failed` | -0.001280 | -0.000250 | 0.000000 | 0.000000 |
| `pred_r16_lambda10` | `failed` | -0.001280 | -0.000375 | 0.000000 | 0.000000 |
| `pred_r16_lambda100` | `failed` | -0.001120 | -0.000375 | 0.000000 | 0.000000 |
| `pred_r32_lambda0.1` | `failed` | -0.000400 | -0.000406 | 0.000000 | 0.000000 |
| `pred_r32_lambda1` | `failed` | -0.000400 | -0.000375 | 0.000000 | 0.000000 |
| `pred_r32_lambda10` | `failed` | -0.000560 | -0.000344 | 0.000000 | 0.000000 |
| `pred_r32_lambda100` | `failed` | -0.000960 | -0.000656 | 0.000000 | 0.000000 |
| `pred_r4_lambda0.1` | `failed` | -0.000960 | -0.000500 | 0.000000 | -0.001600 |
| `pred_r4_lambda1` | `failed` | -0.001040 | -0.000531 | 0.000000 | -0.001600 |
| `pred_r4_lambda10` | `failed` | -0.000800 | -0.000625 | 0.000000 | -0.001600 |
| `pred_r4_lambda100` | `failed` | -0.000880 | -0.000625 | 0.000000 | -0.001600 |
| `pred_r64_lambda0.1` | `failed` | -0.000880 | -0.000531 | 0.000000 | 0.000000 |
| `pred_r64_lambda1` | `failed` | -0.000880 | -0.000531 | 0.000000 | 0.000000 |
| `pred_r64_lambda10` | `failed` | -0.000800 | -0.000563 | 0.000000 | 0.000000 |
| `pred_r64_lambda100` | `failed` | -0.000800 | -0.000656 | 0.000000 | 0.000000 |
| `pred_r8_lambda0.1` | `failed` | -0.001120 | -0.000281 | 0.000000 | 0.000000 |
| `pred_r8_lambda1` | `failed` | -0.001120 | -0.000281 | 0.000000 | 0.000000 |
| `pred_r8_lambda10` | `failed` | -0.001120 | -0.000344 | 0.000000 | 0.000000 |
| `pred_r8_lambda100` | `failed` | -0.000960 | -0.000500 | 0.000000 | 0.000000 |
| `ridge_oracle` | `passed` | 0.003600 | 0.003344 | 0.000000 | 0.000276 |

## Dev Gate Deltas

| Source | Gate | dO@100 | dO@256 | dCUB | dR@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `pca_oracle_r128` | `passed` | 0.000845 | 0.000935 | 0.000000 | 0.000000 |
| `pca_oracle_r16` | `failed` | -0.000141 | 0.000220 | 0.000000 | 0.000000 |
| `pca_oracle_r32` | `passed` | 0.000141 | 0.000385 | 0.000000 | 0.000000 |
| `pca_oracle_r4` | `failed` | -0.000704 | -0.000385 | 0.000000 | 0.000000 |
| `pca_oracle_r64` | `passed` | 0.000563 | 0.000605 | 0.000000 | 0.000000 |
| `pca_oracle_r8` | `failed` | -0.000845 | 0.000110 | 0.000000 | 0.000000 |
| `pred_r128_lambda0.1` | `failed` | -0.000704 | -0.000110 | 0.000000 | 0.000000 |
| `pred_r128_lambda1` | `failed` | -0.000704 | -0.000110 | 0.000000 | 0.000000 |
| `pred_r128_lambda10` | `failed` | -0.000845 | -0.000330 | 0.000000 | 0.000000 |
| `pred_r128_lambda100` | `failed` | -0.000563 | -0.000220 | 0.000000 | 0.000000 |
| `pred_r16_lambda0.1` | `failed` | -0.000845 | -0.000110 | 0.000000 | 0.000000 |
| `pred_r16_lambda1` | `failed` | -0.000986 | -0.000055 | 0.000000 | 0.000000 |
| `pred_r16_lambda10` | `failed` | -0.000986 | -0.000110 | 0.000000 | 0.000000 |
| `pred_r16_lambda100` | `failed` | -0.000845 | -0.000165 | 0.000000 | 0.000000 |
| `pred_r32_lambda0.1` | `failed` | 0.000141 | -0.000055 | 0.000000 | 0.000000 |
| `pred_r32_lambda1` | `passed` | 0.000141 | 0.000000 | 0.000000 | 0.000000 |
| `pred_r32_lambda10` | `passed` | 0.000000 | 0.000000 | 0.000000 | 0.000000 |
| `pred_r32_lambda100` | `failed` | -0.000282 | -0.000220 | 0.000000 | 0.000000 |
| `pred_r4_lambda0.1` | `failed` | -0.000986 | -0.000330 | 0.000000 | 0.000000 |
| `pred_r4_lambda1` | `failed` | -0.000986 | -0.000385 | 0.000000 | 0.000000 |
| `pred_r4_lambda10` | `failed` | -0.000563 | -0.000440 | 0.000000 | 0.000000 |
| `pred_r4_lambda100` | `failed` | -0.000423 | -0.000440 | 0.000000 | 0.000000 |
| `pred_r64_lambda0.1` | `failed` | -0.000563 | -0.000440 | 0.000000 | 0.000000 |
| `pred_r64_lambda1` | `failed` | -0.000563 | -0.000440 | 0.000000 | 0.000000 |
| `pred_r64_lambda10` | `failed` | -0.000563 | -0.000385 | 0.000000 | 0.000000 |
| `pred_r64_lambda100` | `failed` | -0.000282 | -0.000605 | 0.000000 | 0.000000 |
| `pred_r8_lambda0.1` | `failed` | -0.000845 | -0.000220 | 0.000000 | 0.000000 |
| `pred_r8_lambda1` | `failed` | -0.000845 | -0.000220 | 0.000000 | 0.000000 |
| `pred_r8_lambda10` | `failed` | -0.000845 | -0.000220 | 0.000000 | 0.000000 |
| `pred_r8_lambda100` | `failed` | -0.000845 | -0.000275 | 0.000000 | 0.000000 |
| `ridge_oracle` | `passed` | 0.003662 | 0.002476 | 0.000000 | 0.000486 |

## Test Gate Deltas

| Source | Gate | dO@100 | dO@256 | dCUB | dR@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `pca_oracle_r128` | `failed` | -0.000370 | 0.001664 | 0.000000 | 0.000000 |
| `pca_oracle_r16` | `failed` | -0.000741 | 0.000289 | 0.000000 | 0.000000 |
| `pca_oracle_r32` | `failed` | -0.000370 | 0.000579 | 0.000000 | 0.000000 |
| `pca_oracle_r4` | `failed` | -0.001667 | -0.000579 | 0.000000 | -0.003704 |
| `pca_oracle_r64` | `failed` | -0.000556 | 0.001302 | 0.000000 | 0.000000 |
| `pca_oracle_r8` | `failed` | -0.001111 | 0.000145 | 0.000000 | -0.003704 |
| `pred_r128_lambda0.1` | `failed` | -0.001667 | -0.001013 | 0.000000 | 0.000000 |
| `pred_r128_lambda1` | `failed` | -0.001667 | -0.001013 | 0.000000 | 0.000000 |
| `pred_r128_lambda10` | `failed` | -0.001667 | -0.001013 | 0.000000 | 0.000000 |
| `pred_r128_lambda100` | `failed` | -0.001481 | -0.000868 | 0.000000 | 0.000000 |
| `pred_r16_lambda0.1` | `failed` | -0.001667 | -0.000434 | 0.000000 | 0.000000 |
| `pred_r16_lambda1` | `failed` | -0.001667 | -0.000506 | 0.000000 | 0.000000 |
| `pred_r16_lambda10` | `failed` | -0.001667 | -0.000723 | 0.000000 | 0.000000 |
| `pred_r16_lambda100` | `failed` | -0.001481 | -0.000651 | 0.000000 | 0.000000 |
| `pred_r32_lambda0.1` | `failed` | -0.001111 | -0.000868 | 0.000000 | 0.000000 |
| `pred_r32_lambda1` | `failed` | -0.001111 | -0.000868 | 0.000000 | 0.000000 |
| `pred_r32_lambda10` | `failed` | -0.001296 | -0.000796 | 0.000000 | 0.000000 |
| `pred_r32_lambda100` | `failed` | -0.001852 | -0.001230 | 0.000000 | 0.000000 |
| `pred_r4_lambda0.1` | `failed` | -0.000926 | -0.000723 | 0.000000 | -0.003704 |
| `pred_r4_lambda1` | `failed` | -0.001111 | -0.000723 | 0.000000 | -0.003704 |
| `pred_r4_lambda10` | `failed` | -0.001111 | -0.000868 | 0.000000 | -0.003704 |
| `pred_r4_lambda100` | `failed` | -0.001481 | -0.000868 | 0.000000 | -0.003704 |
| `pred_r64_lambda0.1` | `failed` | -0.001296 | -0.000651 | 0.000000 | 0.000000 |
| `pred_r64_lambda1` | `failed` | -0.001296 | -0.000651 | 0.000000 | 0.000000 |
| `pred_r64_lambda10` | `failed` | -0.001111 | -0.000796 | 0.000000 | 0.000000 |
| `pred_r64_lambda100` | `failed` | -0.001481 | -0.000723 | 0.000000 | 0.000000 |
| `pred_r8_lambda0.1` | `failed` | -0.001481 | -0.000362 | 0.000000 | 0.000000 |
| `pred_r8_lambda1` | `failed` | -0.001481 | -0.000362 | 0.000000 | 0.000000 |
| `pred_r8_lambda10` | `failed` | -0.001481 | -0.000506 | 0.000000 | 0.000000 |
| `pred_r8_lambda100` | `failed` | -0.001111 | -0.000796 | 0.000000 | 0.000000 |
| `ridge_oracle` | `passed` | 0.003519 | 0.004485 | 0.000000 | 0.000000 |

## Decision

```json
{
  "heldout_oracle_gate": {
    "checks": {
      "cub_safe": true,
      "dense_overlap_100_safe": true,
      "dense_overlap_256_safe": true,
      "recall_safe": true
    },
    "deltas": {
      "accuracy": 0.0,
      "candidate_upper_bound": 0.0,
      "dense_overlap_at_10": -0.0007999999999999119,
      "dense_overlap_at_100": 0.0036000000000000476,
      "dense_overlap_at_256": 0.0033437499999999787,
      "dense_overlap_at_50": 0.0044800000000001505,
      "hit_rate_at_10": 0.0,
      "hit_rate_at_100": 0.0,
      "hit_rate_at_1000": 0.0,
      "hit_rate_at_20": 0.0,
      "map_at_10": -0.0013784211191107953,
      "map_at_100": -0.0008258628335647833,
      "map_at_1000": -0.0008785899803566144,
      "map_at_20": -0.0009717503126058791,
      "mrr_at_10": -0.0040000000000000036,
      "mrr_at_100": -0.004035294117647026,
      "mrr_at_1000": -0.004030798833681515,
      "mrr_at_20": -0.0040000000000000036,
      "ndcg_at_10": -0.0017649286520214913,
      "ndcg_at_100": -0.0009713128932881121,
      "ndcg_at_1000": -0.0010611289799996593,
      "ndcg_at_20": -0.0010925255317161264,
      "precision_at_10": -0.0008000000000000229,
      "precision_at_100": 7.999999999999674e-05,
      "precision_at_1000": 0.0,
      "precision_at_20": 0.0,
      "recall_at_10": -0.0016000000000000458,
      "recall_at_100": 0.00027586206896546006,
      "recall_at_1000": 0.0,
      "recall_at_20": 0.0
    },
    "failed_checks": [],
    "status": "passed"
  },
  "passed_pca_source": null,
  "passed_pca_source_all_eval_splits": null,
  "passed_pca_source_heldout_macro": "pca_oracle_r64",
  "passed_predictor_source": null,
  "passed_predictor_source_all_eval_splits": null,
  "passed_predictor_source_heldout_macro": null,
  "status": "weak_aggregate_safe_subspace_only"
}
```

## Conclusion

A low-rank oracle reconstruction passed only on the combined heldout
macro, not on both dev and test splits.  This is a weak positive
signal about compressibility, but it is not robust enough to promote
another query-side compiler.  The next step should inspect why test
queries still lose O@100 before any broader training.
