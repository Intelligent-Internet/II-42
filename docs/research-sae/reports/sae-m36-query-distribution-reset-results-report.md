# SAE M36 Query-Distribution Reset Results Report

Status: closed after audit, synthetic validation, and one real-query replay
training run.

## Summary

M36 tested whether the post-M35 blocker was mainly lack of real broad-query
training distribution.

The answer is mostly no for the current available data:

- M36.0 audit found that current eval has 50 TREC-like
  `broad_high_df + many_positive` queries, while M32 train has only 3.
- M36.2 validation showed the default M35/M35b synthetic sources should not be
  reused. M35/M35b were already trained and failed, and the TREC synthetic
  subsets still do not match the hard real-query distribution.
- M36.1 expanded real-query replay used the strongest available clean real
  source, `nfcorpus` train/dev, growing it from 300 queries to 2,914 queries
  and 121,960 qrel pairs. This lowered SAE postings but did not fix the
  no-collapse gate.

M36 therefore closes as negative evidence for:

```text
existing BEIR train/dev real-query expansion
+ default M35/M35b synthetic artifacts
  -> robust TREC-like broad-query recovery
```

The next valid move is not another small weighting pass. It is either a new
validated query generator that actually matches hard broad-query intent, or a
stronger query encoder / supervision design under the same fixed-doc gate.

## M36.0 Query Distribution Audit

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m36_query_distribution_audit.py \
  --output-dir results/sae/m36/query-distribution-audit
```

Output:

```text
results/sae/m36/query-distribution-audit/m36_query_distribution_audit.md
results/sae/m36/query-distribution-audit/m36_query_distribution_audit.json
```

Decision:

```text
needs_validated_synthetic_queries
```

Surface summary:

| Surface | Queries | Mean content DF | Mean qrels/query | Mean teacher-BM25 MAP delta | TREC-like buckets |
| --- | ---: | ---: | ---: | ---: | ---: |
| `current_eval` | 1,342 | 0.0454 | 29.6140 | 0.0926 | 50 |
| `m32_official_test_artifact` | 3,741 | 0.0360 | 15.8041 | n/a | 56 |
| `m32_train_teacher` | 2,167 | 0.0294 | 8.7480 | 0.0719 | 3 |

Hard dataset summary:

| Surface/Dataset | Queries | Mean content DF | Mean qrels/query | Mean teacher-BM25 MAP delta | Key bucket counts |
| --- | ---: | ---: | ---: | ---: | --- |
| `current_eval/trec-covid` | 50 | 0.1925 | 493.4600 | 0.2946 | `broad_high_df:49, many_positive:50, semantic_heavy:41` |
| `current_eval/msmarco` | 43 | 0.0230 | 95.3953 | 0.1459 | `many_positive:14, semantic_heavy:20` |
| `current_eval/dbpedia-entity` | 100 | 0.0182 | 34.5800 | 0.1238 | `semantic_heavy:44, many_positive:7` |
| `m32_train_teacher/msmarco` | 300 | 0.0121 | 1.0567 | 0.1905 | `semantic_heavy:97, broad_high_df:9` |
| `m32_train_teacher/dbpedia-entity` | 67 | 0.0106 | 20.9701 | 0.0943 | `semantic_heavy:25, many_positive:1` |

Interpretation:

- M32 train has semantic-heavy queries, but they are mostly one-positive or
  low-qrel-count ranking problems.
- The hard `trec-covid` class is different: broad high-DF query terms, hundreds
  of positives per query, and large teacher-vs-BM25 neighborhood advantage.
- Ordinary M36.1 replay on the existing M32 train mix should not be expected to
  solve this by reweighting alone.

## M36.2 Synthetic Validation

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m36_synthetic_query_validation.py \
  --output-dir results/sae/m36/synthetic-query-validation
```

Output:

```text
results/sae/m36/synthetic-query-validation/m36_synthetic_query_validation.md
results/sae/m36/synthetic-query-validation/m36_synthetic_query_validation.json
```

Decision:

```text
existing_synthetic_sources_insufficient
```

Target distribution:

| Target | Queries | Mean token count | Mean content DF | Long share | Duplicate-like rate | Top term share |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `current_eval/trec-covid` TREC-like | 49 | 11.7143 | 0.1959 | 0.8163 | 0.0204 | 0.1037 |

Key synthetic validation result:

| Artifact | Dataset | Decision | Mean content DF | Long share | Duplicate-like rate | Failures |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `m35_q100` | `m35-trec-covid-neighborhood` | failed | 0.3511 | 0.8800 | 0.8000 | `content_df_mismatch, low_diversity` |
| `m35b_diverse_q100` | `m35-trec-covid-neighborhood` | failed | 0.3065 | 0.9700 | 0.0600 | `content_df_mismatch` |

Some non-TREC synthetic sub-artifacts pass coarse query-shape checks, but they
were already included in the failed M35 training runs. They are therefore not
new evidence and should not be retrained unchanged.

## M36.1 Real Query Replay

Additional clean real source:

```text
nfcorpus train/dev:
queries = 2,914
documents = 3,633
qrel pairs = 121,960
leakage with current/test = false
```

Commands:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m32_build_split_artifacts.py \
  --output-root /Volumes/Betty/Tmp/ii42_sae_m36_splits \
  --datasets nfcorpus \
  --max-train-queries 3000 \
  --max-docs 20000 \
  --skip-test-artifacts

PYTHONPATH=scripts python3 scripts/research_sae_m32_materialize_teacher.py \
  --split-root /Volumes/Betty/Tmp/ii42_sae_m36_splits \
  --output-root /Volumes/Betty/Tmp/ii42_sae_m36_teacher \
  --artifacts m32-train-20k-q300 \
  --datasets nfcorpus \
  --device mps
```

Training root:

```text
/Volumes/Betty/Tmp/ii42_sae_m36_train_merged/m36-real-nfcorpus-expanded
```

Training output:

```text
results/sae/m36/real-nfcorpus-expanded-train20k-eval-current
```

Data scope:

| Metric | Value |
| --- | ---: |
| Train datasets | 8 |
| Train documents | 128,816 |
| Train queries | 4,781 |
| Train qrel pairs | 126,212 |
| Eval datasets | 15 |
| Eval queries | 1,342 |

## Quality Matrix

| Run | Best | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidate docs | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | `m31_fixed_w0p5` | 0.8694 | 0.8780 | 0.7734 | 0.7449 | 2964.8 | 2778.6 |
| M33 hard-family x1 | `m31_fixed_w0p5` | 0.8661 | 0.8749 | 0.7727 | 0.7446 | 2973.2 | 2848.6 |
| M35 q100 | `m31_fixed_w0p5` | 0.8702 | 0.8791 | 0.7744 | 0.7465 | 2965.3 | 2893.1 |
| M36 nfcorpus-expanded | `m31_fixed_w0p5` | 0.8671 | 0.8795 | 0.7731 | 0.7429 | 2923.6 | 2384.2 |

M36 lowers SAE postings substantially, but the quality/cost tradeoff is not
promotable because hard-dataset ranking remains outside the gate.

## Hard Dataset Table

### `trec-covid`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6226 | 0.4545 | 0.1435 | -0.2124 | -0.3094 |
| M33 hard-family x1 | 0.6391 | 0.4691 | 0.1458 | -0.1960 | -0.2948 |
| M35 q100 | 0.6325 | 0.4558 | 0.1435 | -0.2026 | -0.3081 |
| M36 nfcorpus-expanded | 0.6429 | 0.4516 | 0.1419 | -0.1922 | -0.3123 |

M36 gives the best TREC NDCG among these runs, but MAP and Recall regress. It
does not solve the neighborhood coverage problem.

### `msmarco`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6320 | 0.8765 | 0.7894 | -0.0929 | -0.0587 |
| M33 hard-family x1 | 0.6336 | 0.8832 | 0.7923 | -0.0912 | -0.0520 |
| M35 q100 | 0.6375 | 0.8877 | 0.7937 | -0.0873 | -0.0475 |
| M36 nfcorpus-expanded | 0.6395 | 0.8784 | 0.7900 | -0.0853 | -0.0568 |

NDCG improves slightly, but MAP and recall regress versus M35.

### `dbpedia-entity`

| Run | NDCG@10 | MAP@100 | Recall@100 | Delta NDCG vs teacher | Delta MAP vs teacher |
| --- | ---: | ---: | ---: | ---: | ---: |
| M32 teacher-anchor | 0.6942 | 0.7542 | 0.8523 | 0.0085 | -0.0283 |
| M33 hard-family x1 | 0.6861 | 0.7505 | 0.8511 | 0.0004 | -0.0321 |
| M35 q100 | 0.6918 | 0.7579 | 0.8561 | 0.0062 | -0.0246 |
| M36 nfcorpus-expanded | 0.6834 | 0.7438 | 0.8558 | -0.0023 | -0.0387 |

M36 worsens `dbpedia-entity` collapse.

## Decision

M36 is closed as failed gate.

Do not promote:

- M36 nfcorpus-expanded real-query replay;
- default M35/M35b synthetic sources;
- another pass that only changes weights on the same data.

Keep:

- M36 audit runner;
- M36 synthetic validation runner;
- expanded nfcorpus artifact as a negative/control training source;
- evidence that cost can be lowered, but not at sufficient robust quality.

## Next Direction

The remaining blocker is no longer "we did not include enough BEIR train
queries." The stronger interpretation is:

```text
current query-side encoder and supervision do not learn broad many-positive
semantic-neighborhood intent from available BEIR train/dev sources.
```

Next valid options:

1. Design a new synthetic query generator whose validation target is
   `broad_high_df + many_positive + semantic_heavy`, not title/sentence
   questionization.
2. Add external real supervised corpora with many-positive broad queries, if
   available, and run the M36 audit before training.
3. Reopen query encoder capacity, but only after locking the same audit,
   no-leakage, and no-collapse gate.

The most direct M37 candidate is a stronger query-side model trained on the
same fixed teacher doc atoms, with explicit neighborhood distribution loss and
hard-bucket model selection. It should not depend on dataset ID or test qrels,
and it should not claim dense removal until it passes the existing gate.
