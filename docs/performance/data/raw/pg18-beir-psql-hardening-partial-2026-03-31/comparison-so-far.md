# Comparison So Far

This partial rerun compares the current branch results against
`docs/performance/data/diagnostics/pg18-beir-extension-matrix-current-2026-03-31.json`.

Complete datasets: `12`
Partial datasets: `['fever']`

| Dataset | ids old | ids new | ids delta | text old | text new | text delta |
|---|---:|---:|---:|---:|---:|---:|
| `arguana` | 1141.17 | 1253.85 | +9.87% | 1094.53 | 946.03 | -13.57% |
| `climate-fever` | 39.66 | 37.21 | -6.19% | 38.27 | 42.09 | +9.97% |
| `cqadupstack` | 321.27 | 319.55 | -0.54% | 233.90 | 249.41 | +6.63% |
| `dbpedia-entity` | 88.66 | 77.89 | -12.15% | 66.33 | 75.10 | +13.23% |
| `fiqa` | 984.26 | 963.28 | -2.13% | 900.86 | 909.30 | +0.94% |
| `nfcorpus` | 3382.96 | 3278.06 | -3.10% | 3234.63 | 3299.25 | +2.00% |
| `nq` | 115.96 | 107.09 | -7.65% | 116.98 | 115.66 | -1.13% |
| `quora` | 448.25 | 453.43 | +1.16% | 445.23 | 416.57 | -6.44% |
| `scidocs` | 1373.32 | 1253.64 | -8.71% | 1432.67 | 1226.70 | -14.38% |
| `scifact` | 2203.32 | 2182.63 | -0.94% | 2181.21 | 2108.46 | -3.34% |
| `trec-covid` | 160.74 | 155.22 | -3.43% | 133.04 | 115.59 | -13.12% |
| `webis-touche2020` | 75.82 | 72.20 | -4.77% | 69.74 | 68.61 | -1.62% |

- `psql_bm25s_ids` median QPS delta: `-3.27%`
- `psql_bm25s_text[]` median QPS delta: `-1.37%`
