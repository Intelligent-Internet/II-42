# M547 Stage 1 Dense-Only Retrieval Report

## Purpose

M547 verifies M546 before any BM25, hybrid fusion, qrels-trained ranking loss,
or route optimization.  The question is narrow:

```text
Does the bounded monotonic output compiler preserve dense retrieval behavior?
```

Training remains stage-one only.  M546 learned a corpus-only monotonic compiler
from dense-derived posting targets.  M547 only evaluates retrieval behavior.

## Runs

Smoke:

```text
outputs/m547/stage1_dense_only_retrieval/
  fiqa_m546_monotonic_dense_only_smoke/
```

Full broad10:

```text
outputs/m547/stage1_dense_only_retrieval/
  broad10_m546_monotonic_dense_only_full/
```

The full run used all materialized documents and queries for:

```text
ArguAna, CQADupstackGamingRetrieval, CQADupstackUnixRetrieval,
ClimateFEVERHardNegatives, FEVERHardNegatives, FiQA2018,
HotpotQAHardNegatives, SCIDOCS, TRECCOVID, Touche2020Retrieval.v3
```

Elapsed time was `259.707` seconds on spark-1 CPU inside the NGC PyTorch
container.  GPU was not used.

## Macro Result

| Source | Gamma | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@10 | O@100 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` |  | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 1.00000 | 0.00000 | 0.00000 |
| `dense_fp16` |  | 0.57261 | 0.43536 | 0.74803 | 0.65179 | 0.99993 | 0.99992 | -0.00002 | 0.00000 |
| `row_int8` |  | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99630 | 0.99660 | 0.00006 | -0.00004 |
| `m546_gamma_mean` | 1.02562527 | 0.57282 | 0.43531 | 0.74779 | 0.65188 | 0.99333 | 0.99414 | 0.00019 | -0.00024 |
| `m546_seed5478` | 1.02574456 | 0.57281 | 0.43529 | 0.74779 | 0.65188 | 0.99328 | 0.99412 | 0.00018 | -0.00024 |
| `m546_seed5479` | 1.02514839 | 0.57286 | 0.43531 | 0.74782 | 0.65188 | 0.99344 | 0.99424 | 0.00023 | -0.00021 |
| `m546_seed5480` | 1.02598300 | 0.57282 | 0.43530 | 0.74779 | 0.65189 | 0.99325 | 0.99409 | 0.00019 | -0.00024 |

## Interpretation

M547 confirms the M546 stage-one route is safe enough to keep:

- The monotonic compiler keeps broad10 retrieval essentially dense-equivalent.
- Macro NDCG@10 is neutral to slightly positive versus exact dense.
- Macro Recall@100 drops only `0.00021-0.00024` for the learned gamma range.
- Dense overlap@100 is about `0.9941-0.9942`, so the compiler does perturb
  ranking more than row-int8 compression but not enough to materially damage
  qrels metrics.
- Seed-to-seed gamma differences are negligible at retrieval level.

This is not evidence that M546 beats dense in a meaningful way.  It is evidence
that the first-stage compiler can improve teacher distribution fit without
breaking dense retrieval.  That was the required gate before considering BM25
or any search-specific objective.

## Decision

M546/M547 is now a stable stage-one baseline:

```text
exact dense
row_int8 dense
M546 monotonic gamma mean
```

The next phase can reintroduce search optimization, but it should keep these
three surfaces as fixed dense-only baselines.  Any BM25/fusion/ranking gain
must be compared against all three, not only against exact dense.

## M548 Frontier Follow-Up

M548 reran the dense-only broad10 evaluator with a wider gamma grid:

```text
1.005, 1.010, 1.015, 1.020, 1.025, 1.02562527, 1.030, 1.040, 1.050
```

The teacher-fit selected `gamma=1.02562527` still passes the retrieval gate:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `gamma_1p025625` | 0.57282 | 0.43531 | 0.74779 | 0.65188 | 0.99414 | +0.00019 | -0.00024 |

Larger gamma values are not promoted for the BM25-pre stage.  They can improve
NDCG by a few more ten-thousandths, but overlap@100 falls below the declared
`0.994` dense-equivalence floor from `gamma=1.030` onward.

M548 therefore confirms that the BM25-pre stage should stop at the bounded
M546 monotonic gamma and move the next search gains into a separate
BM25/search-aware phase.
