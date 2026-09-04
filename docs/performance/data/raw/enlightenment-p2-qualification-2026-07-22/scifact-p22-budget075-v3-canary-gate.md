# P2 Runtime Variant Canary

- Native route: `ii42_search`
- Passed: `false`
- Maximum metric drop: `0.001000`
- Required resource improvement: `20.0%`

| Method | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB | p50 ms | p95 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Baseline | 0.709902 | 0.670223 | 0.952667 | 0.679660 | 0.990000 | 51.749 | 62.845 |
| Variant | 0.704454 | 0.665971 | 0.947667 | 0.676055 | 0.990000 | 49.405 | 60.748 |
| Delta | -0.005448 | -0.004252 | -0.005000 | -0.003605 | +0.000000 | - | - |

| Resource | Improvement | Gate |
| --- | ---: | --- |
| Index bytes | 9.789% | `false` |
| Latency p50 | 4.529% | - |
| Latency p95 | 3.337% | `false` |
