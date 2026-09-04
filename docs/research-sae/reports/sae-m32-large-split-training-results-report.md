# SAE M32 Large-Split Training Results Report

Status: M32 first training pass completed; larger clean train/dev data improves
aggregate quality, but does not close the robustness blocker.

## Summary

M32 tested the hypothesis from M31: the final-ranking objective may have
failed because the previous prepared full15 artifact was too small and too
close to evaluation. M32 therefore built a clean official-BEIR train split,
materialized Snowflake/SAE teacher atoms, and trained the M31 final-ranking
query-side model with separate train and eval roots.

The result is useful but not a product pass:

- Full15 aggregate improves over both BM25 and the fixed-doc teacher reference.
- Physical cost is acceptable: student SAE postings are lower than teacher
  postings on the current regression surface.
- The no-collapse gate still fails on `trec-covid`, `msmarco`, and
  `dbpedia-entity`.
- `trec-covid` remains the blocker. M32 larger train data did not improve it
  over M31; in the best M32 run, it remains far below the fixed-doc teacher.

Conclusion: M32 supports the final-ranking training direction for aggregate
quality, but it also shows that general BEIR train/dev expansion is not enough.
The next useful move is not another global weight sweep. It is targeted
hard-distribution supervision, especially biomedical/claim-heavy query
behavior, or a stronger query encoder trained under the same final-ranking
gate.

## Data And Artifact State

M32.0 split artifacts:

| Artifact | Datasets | Documents | Queries | Qrel pairs |
| --- | ---: | ---: | ---: | ---: |
| `m32-train-20k-q300` | 8 | 128,816 | 2,167 | 18,957 |
| `m32-test-official` | 15 | 257,490 | 3,741 | 59,123 |

M32.1 teacher materialization:

| Field | Value |
| --- | --- |
| Output root | `/Volumes/Betty/Tmp/ii42_sae_m32_teacher` |
| Train artifact | `m32-train-20k-q300` |
| Teacher run | `shared_sae_8192_64` |
| Materialized train size | 2.4 GB |
| Materialized train datasets | 8 / 8 |

Materialization times:

| Dataset | Docs | Queries | Seconds |
| --- | ---: | ---: | ---: |
| `dbpedia-entity` | 20,000 | 67 | 123.53 |
| `fever` | 20,000 | 300 | 182.94 |
| `fiqa` | 20,000 | 300 | 205.63 |
| `hotpotqa` | 20,000 | 300 | 121.80 |
| `msmarco` | 20,000 | 300 | 126.33 |
| `nfcorpus` | 3,633 | 300 | 83.97 |
| `quora` | 20,000 | 300 | 82.72 |
| `scifact` | 5,183 | 300 | 121.57 |

## Candidate Coverage

The train candidate surface used `candidate_k=100` and `max_candidates=220`.

| Metric | Value |
| --- | ---: |
| Datasets | 8 |
| Queries | 2,167 |
| Qrel coverage | 0.7764 |
| Teacher top20 coverage | 0.9961 |
| Teacher top100 coverage | 0.9949 |
| Candidate docs mean | 159.2 |

The lower qrel coverage is almost entirely from `nfcorpus`, where some queries
have many positive documents and the candidate cap truncates positives. This is
not hidden: `nfcorpus` qrel coverage is 0.7118, while every other train dataset
has 1.0000 qrel coverage.

For M32 ranking training, this is acceptable as a known limitation because the
teacher top-k coverage is high. If `nfcorpus` becomes a blocker, the candidate
cap should become per-dataset or qrel-aware. It is not the current blocker.

## Runs

Two train-root/eval-root runs were completed:

| Run | Output | Key change |
| --- | --- | --- |
| `m32-primary` | `results/sae/m32/primary-train20k-eval-current` | M31 objective on M32 train split |
| `m32-teacher-anchor` | `results/sae/m32/teacher-anchor-train20k-eval-current` | stronger teacher anchor, weaker qrel/BM25 rewrite |

Both runs trained on:

```text
/Volumes/Betty/Tmp/ii42_sae_m32_teacher/m32-train-20k-q300
```

Both runs evaluated on the previous full15 regression surface:

```text
/Volumes/Betty/Tmp/ii42_sae_beir15_shared
```

## Full15 Aggregate Quality

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| Teacher fixed-doc | 0.8447 | 0.8399 | 0.7490 | 0.7247 |
| M31 unfrozen best | 0.8648 | 0.8721 | 0.7653 | 0.7359 |
| M32 primary best fixed | 0.8685 | 0.8752 | 0.7710 | 0.7430 |
| M32 primary calibrated | 0.8732 | 0.9117 | 0.7988 | 0.7706 |
| M32 teacher-anchor best fixed | 0.8694 | 0.8780 | 0.7734 | 0.7449 |
| M32 teacher-anchor calibrated | 0.8740 | 0.9176 | 0.8003 | 0.7734 |

Aggregate improved. This confirms that final `BM25 + SAE` ranking supervision
has real signal when trained on a larger official train/dev surface.

However, aggregate quality is not enough. The gate requires no dataset
collapse versus teacher.

## Robustness Gate

Best fixed source for both M32 runs is `m31_fixed_w0p5`. The relevant hard
dataset deltas versus teacher:

| Run | Dataset | Metric | Delta vs teacher |
| --- | --- | --- | ---: |
| M32 primary | `dbpedia-entity` | MAP@100 | -0.0304 |
| M32 primary | `msmarco` | NDCG@10 | -0.0903 |
| M32 primary | `msmarco` | MAP@100 | -0.0544 |
| M32 primary | `trec-covid` | NDCG@10 | -0.2205 |
| M32 primary | `trec-covid` | MAP@100 | -0.3097 |
| M32 teacher-anchor | `dbpedia-entity` | MAP@100 | -0.0283 |
| M32 teacher-anchor | `msmarco` | NDCG@10 | -0.0929 |
| M32 teacher-anchor | `msmarco` | MAP@100 | -0.0587 |
| M32 teacher-anchor | `trec-covid` | NDCG@10 | -0.2124 |
| M32 teacher-anchor | `trec-covid` | MAP@100 | -0.3094 |

`trec-covid` detail:

| Source | Recall@100 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: |
| BM25 | 0.1384 | 0.6645 | 0.4693 |
| Teacher fixed-doc | 0.2081 | 0.8351 | 0.7639 |
| M31 unfrozen best | 0.1419 | 0.6539 | 0.4648 |
| M32 primary best fixed | 0.1433 | 0.6146 | 0.4542 |
| M32 teacher-anchor best fixed | 0.1435 | 0.6226 | 0.4545 |

M32 does not improve the hard `trec-covid` blocker. It slightly improves
aggregate and some datasets, but `trec-covid` remains close to BM25 and far
from the teacher. This is strong evidence that the current text-to-atoms query
encoder is not learning the teacher semantic behavior for biomedical/claim
style queries from the available train/dev data.

## Physical Cost

| Source | Candidate docs | BM25 postings | SAE postings |
| --- | ---: | ---: | ---: |
| Teacher fixed-doc | 2,986.2 | 10,185.0 | 4,522.8 |
| M32 primary | 2,964.5 | 10,185.0 | 2,775.3 |
| M32 teacher-anchor | 2,964.8 | 10,185.0 | 2,778.6 |

Physical cost is not the reason for failure. The M32 student paths use fewer
SAE postings than the fixed-doc teacher while preserving similar candidate-doc
volume. The blocker is ranking robustness on specific query families.

## Interpretation

M32 answers the immediate question:

```text
Was M31 blocked mainly because the training artifact was too small?
```

Answer:

```text
Partially for aggregate quality, no for product robustness.
```

The larger clean split made the model stronger on average, but it did not
teach the query encoder to recover teacher-quality semantic behavior on the
hard holdout family. Since `trec-covid` has no official train/dev split in the
current BEIR cache, this is expected: M32 improved what it could see, but not
the most important missing distribution.

## Decision

M32 is not promoted.

Do not proceed to doc-side training or SQL/API productization from this model.

Recommended next training phase:

1. Build a targeted biomedical/claim-heavy proxy training track, clearly
   separated from official `trec-covid` test queries.
2. Preserve the final-ranking objective and fixed teacher doc atoms.
3. Add teacher-neighborhood distillation for hard query families without using
   held-out test qrels as train labels.
4. Keep M32 train/dev as the general-data backbone, but add hard-family
   supervision as a separate arm.
5. Only if `trec-covid`, `msmarco`, and `dbpedia-entity` collapse improves
   should M32 proceed to official-test materialization and doc-side training.

This keeps the product target unchanged:

```text
text -> atoms -> unified sparse evidence engine
```

but the model-side route needs hard-distribution training, not another global
weight sweep.
