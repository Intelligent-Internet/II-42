# SAE M73 Query-Family Diagnostics Results Report

Date: 2026-05-21

Status: query-family diagnostics implemented. Family-aware residual was tested
locally and on Spark; it is not promoted.

## Summary

M73 followed the M72 conclusion: teacher signal should not be a global residual
weight. The next hypothesis was that teacher residual should only affect query
families where semantic coverage is likely useful.

Implemented query families:

- `all`
- `short_keyword`
- `long_query`
- `broad_many_positive`
- `lexical_heavy`
- `semantic_heavy`

The family labels are intentionally diagnostic, not product routing. They use
query length, BM25 concentration, BM25 positive rate, and qrel count where
available. This makes them suitable for research and curriculum analysis, not a
runtime API contract.

## Implemented Pieces

The M70 trainer now writes family-level metrics in every result JSON and
markdown report:

```text
metrics.families.<family>.bm25
metrics.families.<family>.student
metrics.families.<family>.query_count
```

The trainer also supports:

```text
teacher_residual_gate=semantic_or_broad
```

This gate applies residual target pressure only to low-BM25-concentration
queries or broad/many-positive supervised queries.

## Local Smoke

Two-dataset smoke: `scifact + nfcorpus`, 2,400 docs, 112 train queries, 43 eval
queries.

Best local M73 structure:

```text
teacher_residual_weight = 0.05
teacher_residual_gate = semantic_or_broad
semantic_teacher_weight = 0.10
```

Local aggregate:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| `student` | 0.5311 | 0.6085 | 0.4724 | 0.3544 |

Local family deltas versus BM25:

| Family | Queries | Recall Delta | MRR Delta | NDCG Delta | MAP Delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `broad_many_positive` | 4 | +0.0182 | +0.0000 | +0.0000 | +0.0039 |
| `semantic_heavy` | 28 | +0.0151 | +0.0052 | +0.0030 | +0.0018 |
| `short_keyword` | 20 | +0.0113 | +0.0073 | +0.0041 | +0.0028 |
| `long_query` | 8 | +0.0000 | +0.0000 | +0.0000 | +0.0000 |

Local interpretation:

- The family-aware gate is cleaner than `low_bm25_or_broad`.
- It improves the exact families it was meant to target.
- It does not perturb long natural-language queries.

## Medium Spark Diagnostics

M39 medium setup: 9 datasets, 92,816 docs, 1,650 train queries, 679 eval
queries, 31,904 qrel positives in candidate pools.

Runs:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Residual mass |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 baseline | 0.6661 | 0.6685 | 0.5858 | 0.4949 | n/a |
| coverage rerun with family metrics | 0.6647 | 0.6687 | 0.5858 | 0.4949 | 0.0000 |
| residual `low_bm25` | 0.6650 | 0.6692 | 0.5857 | 0.4949 | 0.5900 |
| residual `semantic_or_broad` | 0.6646 | 0.6692 | 0.5859 | 0.4949 | 0.4770 |

The family-metric rerun is not a promotion benchmark; it is a diagnostic rerun
with the updated script. The important evidence is the within-run family
pattern.

Medium family deltas versus BM25 for `semantic_or_broad`:

| Family | Queries | Recall Delta | NDCG Delta | MAP Delta |
| --- | ---: | ---: | ---: | ---: |
| `broad_many_positive` | 111 | -0.0002 | +0.0000 | -0.0002 |
| `semantic_heavy` | 399 | -0.0012 | +0.0000 | +0.0000 |
| `short_keyword` | 182 | -0.0054 | +0.0005 | +0.0002 |
| `long_query` | 211 | +0.0000 | -0.0000 | -0.0000 |
| `lexical_heavy` | 4 | +0.0000 | +0.0000 | +0.0000 |

Medium interpretation:

- The local family gains do not survive the 9-dataset medium run.
- `semantic_or_broad` improves NDCG/MAP surface slightly but loses Recall@100.
- Residual target pressure still trades coverage for ranking polish.
- The family taxonomy is useful diagnostically, but not sufficient as a
  training gate.

## Decision

M73 closes as a diagnostics milestone, not a promoted training structure.

Promoted:

- query-family metrics in the trainer;
- `semantic_or_broad` as a research gate for future tests;
- family-level reporting as required evidence for future teacher/curriculum
  work.

Rejected:

- using the current family labels directly as a residual target gate;
- further single-stage residual gate sweeps;
- broad/many-positive qrel count as the only signal for teacher routing.

## Next Direction

The next step should be data/curriculum-first:

```text
M74: teacher candidate curriculum diagnostics
  - separate qrel-positive hits by candidate source: BM25, dense teacher,
    SAE teacher, random
  - identify queries where dense teacher adds qrel positives BM25 misses
  - train only on examples where dense teacher has proven candidate utility
  - keep final listwise target qrel-first with no teacher residual by default
```

The key change is to use embedding teacher to select better training examples
and hard candidates, not to modify the ranking target for all queries.
