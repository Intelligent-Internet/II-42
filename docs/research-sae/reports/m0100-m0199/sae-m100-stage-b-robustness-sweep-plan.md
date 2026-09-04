# SAE M100 Stage-B Robustness Sweep Plan

Date: 2026-05-22

## Summary

M99 is a large candidate-surface ranking improvement, so the next optimization
should not immediately add model capacity. M100 checks whether M99 is robust
enough to become the Stage-B mainline.

The focus is controlled robustness:

- repeat the same architecture with another seed;
- test a more conservative residual capacity;
- test whether broad-generated gains depend on explicit train reweighting.

All variants keep the same runtime contract: BM25/SAE-only features at query
time, dense only as a training/control teacher.

## Variants

| Variant | Purpose |
| --- | --- |
| `baseline_seed123` | Check seed sensitivity. |
| `conservative_residual` | Lower residual capacity and stronger regularization. |
| `no_broad_weight` | Remove broad-family training upweighting. |

## Gate

M100 passes if at least one non-identical variant stays close to M99:

- eval NDCG@10 remains clearly above M98;
- holdout remains above M98;
- broad-generated family remains above BM25+dense or at least above M98;
- no variant requires runtime dense features.

If only the exact M99 seed/config works, M99 is useful but not robust enough for
mainline promotion. The next step would be safer model selection or more data,
not a bigger residual model.
