# SAE Full15 Gap And Adaptive Gate Report

## Scope

This pass starts from the current full15 shared-teacher frontier:

```text
results/sae/text-atoms/full15-shared-dense-teacher-token-char/
```

It does not retrain the student. Instead, it rebuilds BM25, teacher SAE, and
student SAE rankings for the same 15 BEIR qrels-preserving datasets and
evaluates whether the remaining teacher/student gap can be closed by adaptive
student atom weights.

## Artifacts

Gap diagnostics:

```text
scripts/research_sae_gap_adaptive_gate.py
results/sae/text-atoms/full15-gap-adaptive-gate/
```

Learned gate follow-up:

```text
scripts/research_sae_learned_gate_from_diagnostics.py
results/sae/text-atoms/full15-learned-adaptive-gate/
```

The diagnostics payload stores per-query metrics and runtime-safe query
features. The learned-gate script reuses that payload and does not rerun
ranking.

## Dataset-Macro Source Means

These numbers are aligned with the existing full15 quality matrix.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.7838 | 0.7863 | 0.6675 | 0.6254 |
| `teacher` | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| `student_w0p25` | 0.8111 | 0.8137 | 0.7007 | 0.6689 |
| `student_w0p5` | 0.8240 | 0.8227 | 0.7111 | 0.6827 |
| `student_w0p75` | 0.8232 | 0.8231 | 0.7086 | 0.6778 |
| `student_w1` | 0.8211 | 0.8165 | 0.6967 | 0.6603 |
| `student_w1p5` | 0.8117 | 0.7718 | 0.6427 | 0.5983 |

## Oracle Signal

Per-query oracle weight selection has enough headroom to matter:

| Metric | Query-Macro Oracle | Query-Macro Teacher | Gap |
| --- | ---: | ---: | ---: |
| `recall@100` | 0.8615 | 0.8613 | -0.0002 |
| `mrr@20` | 0.8393 | 0.8281 | -0.0112 |
| `ndcg@10` | 0.7429 | 0.7437 | 0.0008 |
| `map@100` | 0.6968 | 0.7070 | 0.0102 |

This means the student rankings already contain many of the right documents,
but the best global weight cannot expose them for every query.

## Simple Gate Results

The leave-one-dataset-out threshold gate gives only small gains:

| Objective | Gate | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `recall@100` | `fixed` | 0.8213 | 0.8210 | 0.7104 | 0.6810 |
| `recall@100` | `threshold` | 0.8228 | 0.8223 | 0.7106 | 0.6790 |
| `mrr@20` | `fixed` | 0.8220 | 0.8196 | 0.7074 | 0.6783 |
| `mrr@20` | `threshold` | 0.8248 | 0.8233 | 0.7099 | 0.6810 |
| `ndcg@10` | `fixed` | 0.8240 | 0.8227 | 0.7111 | 0.6827 |
| `ndcg@10` | `threshold` | 0.8261 | 0.8237 | 0.7111 | 0.6812 |
| `map@100` | `fixed` | 0.8240 | 0.8227 | 0.7111 | 0.6827 |
| `map@100` | `threshold` | 0.8227 | 0.8219 | 0.7097 | 0.6808 |

The best threshold result improves Recall@100 to `0.8261`, but MAP falls
slightly. This is a useful diagnostic, not a product-quality gate.

## Learned Gate Results

A centroid gate and a kNN gate were tested on the same leave-one-dataset-out
splits. They do not beat the fixed global weight.

| Objective | Gate | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `recall@100` | `fixed` | 0.8213 | 0.8210 | 0.7104 | 0.6810 |
| `recall@100` | `centroid` | 0.8190 | 0.8180 | 0.7014 | 0.6665 |
| `recall@100` | `knn` | 0.8111 | 0.8139 | 0.7008 | 0.6691 |
| `mrr@20` | `fixed` | 0.8220 | 0.8196 | 0.7074 | 0.6783 |
| `mrr@20` | `centroid` | 0.8222 | 0.8180 | 0.7062 | 0.6758 |
| `mrr@20` | `knn` | 0.8111 | 0.8137 | 0.7007 | 0.6689 |
| `ndcg@10` | `fixed` | 0.8240 | 0.8227 | 0.7111 | 0.6827 |
| `ndcg@10` | `centroid` | 0.8218 | 0.8238 | 0.7071 | 0.6718 |
| `ndcg@10` | `knn` | 0.8113 | 0.8133 | 0.6998 | 0.6684 |
| `map@100` | `fixed` | 0.8240 | 0.8227 | 0.7111 | 0.6827 |
| `map@100` | `centroid` | 0.8196 | 0.8142 | 0.6960 | 0.6587 |
| `map@100` | `knn` | 0.8136 | 0.8144 | 0.6988 | 0.6636 |

The failure mode is instructive: the learned gates over-select low or high
weights when query diagnostics look locally similar but ranking consequences
differ by dataset. This suggests the current query-side features are not enough
to recover the oracle.

## Dataset Gap Shape

The largest Recall@100 gaps are:

| Dataset | Teacher R@100 | Best Student R@100 | Gap |
| --- | ---: | ---: | ---: |
| `cqadupstack` | 0.9348 | 0.8746 | 0.0602 |
| `dbpedia-entity` | 0.8926 | 0.8383 | 0.0543 |
| `fiqa` | 0.8771 | 0.8299 | 0.0471 |
| `trec-covid` | 0.2074 | 0.1709 | 0.0365 |
| `nfcorpus` | 0.3664 | 0.3316 | 0.0349 |

These are not primarily global score-weight problems. They are candidate
coverage and representation problems: the student needs better query/document
atom alignment for datasets where teacher semantics opens documents BM25 and
the current text encoder do not expose.

## Decision

Do not promote adaptive weighting as the next main route. Keep `student_w0p5`
as the balanced default and use the threshold gate only as a diagnostic.

The next real training effort should target representation and candidate
coverage:

1. Train a candidate-budget objective that directly rewards opening qrels or
   dense-teacher positives under a fixed atom budget.
2. Add query/document asymmetric capacity, because query atom prediction is the
   serving-time bottleneck for semantic recall.
3. Revisit product-domain arxiv/pubmed text through pseudo-query or
   dense-neighborhood supervision, not unweighted doc-only distillation.
