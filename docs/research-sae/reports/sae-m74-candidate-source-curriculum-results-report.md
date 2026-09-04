# SAE M74 Candidate Source Curriculum Results Report

Date: 2026-05-21

Status: candidate-source utility diagnostics are promoted. Hard-filter
`teacher_extra_positive` curriculum is rejected.

## Summary

M74 followed the M73 conclusion: teacher signal should be used to choose better
training examples and candidates, not to rewrite every query's ranking target.

The trainer now reports source-level qrel utility for:

- BM25 candidates;
- dense teacher candidates;
- SAE teacher candidates;
- dense/SAE teacher union;
- BM25 + teacher candidate union.

It also reports the same teacher utility by M73 query family. This gives a
direct answer to the next curriculum question:

```text
Does the teacher actually add qrel positives that BM25 misses?
```

## Implemented Pieces

The M70 trainer now supports:

```text
--diagnostics-only
--candidate-curriculum none
--candidate-curriculum teacher_extra_positive
--candidate-curriculum-min-extra-positives N
```

The output now includes:

```text
candidate_diagnostics.candidate_source_utility.global
candidate_diagnostics.candidate_source_utility.families
candidate_diagnostics.candidate_curriculum
```

`--diagnostics-only` writes a compact M74 report without training a random
model. This is important for medium and large corpus scans.

## Local Smoke

Scope: `scifact + nfcorpus`, 2,400 documents, 112 train queries, 43 eval
queries.

### Source Utility

| Source | Candidate Mean | Qrel Recall | Extra Recall Over BM25 | Queries With Extra Hit |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 60.00 | 0.2435 | 0.0000 | 0 |
| `dense_teacher` | 40.00 | 0.2620 | 0.0844 | 40 |
| `sae_teacher` | 40.00 | 0.2250 | 0.0597 | 31 |
| `teacher_union` | 62.04 | 0.2949 | 0.1077 | 43 |
| `candidate_union` | 104.67 | 0.3512 | 0.1077 | 43 |

Interpretation: the teacher candidates are not redundant. On this smoke split,
they recover 157 qrel positives that BM25 top-60 misses.

### Training Comparison

| Run | Train Examples | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | n/a | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| no curriculum | 112 | 0.5410 | 0.6040 | 0.4686 | 0.3537 |
| hard `teacher_extra_positive` | 43 | 0.5280 | 0.6039 | 0.4686 | 0.3532 |

Hard filtering is too lossy: it keeps only 43/112 examples and loses most of
the recall gain from the normal M70 coverage path.

## M39 Medium Diagnostics

Scope: 9 datasets, 146,353 documents, 1,750 train queries, 28,328 qrel
positives. This was run with diagnostics-only mode and no model training.

| Source | Candidate Mean | Qrel Hits | Qrel Recall | Extra Hits Over BM25 | Extra Recall | Queries With Extra Hit |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 128.00 | 10,165 | 0.3588 | 0 | 0.0000 | 0 |
| `dense_teacher` | 48.00 | 9,906 | 0.3497 | 3,845 | 0.1357 | 448 |
| `sae_teacher` | 48.00 | 9,903 | 0.3496 | 4,660 | 0.1645 | 383 |
| `teacher_union` | 77.29 | 12,744 | 0.4499 | 6,125 | 0.2162 | 474 |
| `candidate_union` | 183.86 | 16,290 | 0.5750 | 6,125 | 0.2162 | 474 |

Family view:

| Family | Queries | Qrel Total | Teacher Qrel Recall | Teacher Extra Recall | Queries With Teacher Extra Hit |
| --- | ---: | ---: | ---: | ---: | ---: |
| `all` | 1,750 | 28,328 | 0.4499 | 0.2162 | 474 |
| `broad_many_positive` | 258 | 23,660 | 0.4160 | 0.2427 | 238 |
| `lexical_heavy` | 18 | 265 | 0.0943 | 0.0264 | 4 |
| `long_query` | 598 | 15,728 | 0.5512 | 0.3362 | 203 |
| `semantic_heavy` | 931 | 22,731 | 0.4326 | 0.2139 | 381 |
| `short_keyword` | 334 | 8,850 | 0.2475 | 0.0598 | 127 |

Interpretation:

- Teacher candidates are clearly useful as candidate mining signal.
- Dense and SAE teacher complement BM25 and each other.
- The usefulness is concentrated: only 474/1,750 train queries have teacher
  extra qrel hits, but those hits account for a large amount of qrel coverage.
- Hard filtering to only those 474 queries would discard too much lexical and
  calibration surface.

## Decision

Promoted:

- candidate-source qrel utility diagnostics;
- diagnostics-only M74 scan mode;
- using teacher candidate utility as a curriculum signal;
- separating source utility by query family.

Rejected:

- hard `teacher_extra_positive` filtering as the next training route;
- another teacher residual gate sweep;
- using M73 family labels alone as promotion evidence.

## Next Direction

The next step should be source-utility-weighted curriculum, not hard filtering:

```text
M75: candidate-source utility weighted training
  - keep all train queries;
  - upweight candidates where dense/SAE teacher finds qrel positives that BM25
    misses;
  - preserve normal BM25/qrel ranking loss for lexical calibration;
  - report source utility before and after training;
  - reject if weighted training improves recall but damages MRR/NDCG/MAP.
```

This keeps the useful teacher evidence while avoiding M74's hard-filter data
collapse.
