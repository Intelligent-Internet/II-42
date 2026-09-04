# M513 Route Objective Grid Report

M513 tests whether the M512 route-fanout tradeoff can be improved by simple
objective balancing.  The script trains the base PPLX-LoRA model once, restores
the same trainable adapter/head state for each preset, and then applies a short
route-aware fine-tune.

Training remains first-stage only: no BM25 and no qrels training signal.

## Run

| Run | Host | Task | Route docs | Presets | Output |
| --- | --- | --- | ---: | --- | --- |
| `fiqa_route_objective_grid` | `spark-1` | `FiQA2018` | `4096/57638` | `baseline,preserve16,preserve16_neg48_e2` | `outputs/m513/fiqa_route_objective_grid/m513_fiqa_route_objective_grid.json` |

## Preset Matrix

| Preset | Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline` | `p128` | 0.46711 | 0.87266 | 0.57163 | 0.41646 | 0.95359 | 0.74261 |
| `baseline` | `p96` | 0.46711 | 0.87005 | 0.57247 | 0.41677 | 0.92484 | 0.68090 |
| `baseline` | `p64` | 0.46262 | 0.85294 | 0.57329 | 0.41208 | 0.85656 | 0.58523 |
| `preserve16` | `p128` | 0.46248 | 0.87318 | 0.57153 | 0.41172 | 0.92953 | 0.69367 |
| `preserve16` | `p96` | 0.45224 | 0.85313 | 0.57314 | 0.39904 | 0.88328 | 0.62432 |
| `preserve16` | `p64` | 0.43738 | 0.81403 | 0.55780 | 0.38227 | 0.79844 | 0.52319 |
| `preserve16_neg48_e2` | `p128` | 0.46269 | 0.84922 | 0.57205 | 0.41092 | 0.84172 | 0.66830 |
| `preserve16_neg48_e2` | `p96` | 0.45758 | 0.82824 | 0.57295 | 0.40520 | 0.78156 | 0.59632 |
| `preserve16_neg48_e2` | `p64` | 0.42322 | 0.75179 | 0.53490 | 0.37143 | 0.67594 | 0.49212 |

## Interpretation

The simple objective grid did not improve over M512.  The M512 baseline remains
the best point:

- p128 is the only tested setting that keeps candidate recall above 0.95;
- p128 also keeps NDCG@10 at 0.46711 and Touch below 0.80;
- p96 keeps quality but candidate recall remains below target;
- adding more positives, more negatives, or a second route epoch reduces fanout
  but damages candidate recall and NDCG.

This rejects the naive hypothesis that more dense positives or stronger
negative pressure will recover the p96/p64 route.  The model is learning to
narrow coordinate sharing, but it narrows the wrong neighbours as pressure
increases.

## Current Best

The current best route in this branch is still M512/M513 `baseline` p128:

| Metric | Value |
| --- | ---: |
| NDCG@10 | 0.46711 |
| Recall@100 | 0.87266 |
| MRR@20 | 0.57163 |
| MAP@100 | 0.41646 |
| Candidate R@100 | 0.95359 |
| Touch | 0.74261 |

This is a real improvement over M511 p128 Touch 0.91362, while preserving
quality.  It is not yet enough for a full promotion because the p96/p64 frontier
still loses candidate recall too quickly.

## Next Step

M514 should stop using only query-local sampled negatives.  The next useful
training signal should directly regularize coordinate load and sharing:

1. penalize high global coordinate document frequency;
2. encourage a target per-query touched-doc budget;
3. preserve dense teacher top-k with a separate recall-oriented positive loss;
4. evaluate p96 and p128 first, not p64.

If load-balancing does not improve p96 candidate recall, then the route should
move to query-adaptive prefix/fanout rather than further scalar loss sweeps.
