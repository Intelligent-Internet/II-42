# ii42 M336 TREC-COVID Admission Diagnostic Report

## Summary

M335 broad-8 originally reported `trec-covid` heldout candidate upper bound
as `0.0000`. M336 tested whether that was a real candidate/admission failure
or an evaluator artifact.

The result is clear: `trec-covid` candidates are healthy. The original M335
collapse was caused by a multi-dataset evaluation bug: raw query IDs were used
as global qrel keys, so datasets with overlapping query IDs could overwrite
each other in `qrels_by_query`.

## Inputs

- Dataset: `trec-covid`
- Dataset root:
  `/home/huoju/leask/runs/ii42-m180a-stagea-gate-latest-full-v1/all-test`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m334-interaction-candidate-cache-v1/trec-covid_m320_nfcorpus_scifact_seed1050_admission_prior005_d96_q80_ml256_ck160_bm25_1.5_0.75_unified_0p25_0p5_teacher_none_0_df_0p25.pt`
- Diagnostic JSON:
  `/home/huoju/leask/runs/ii42-m336-trec-covid-admission-diagnostic-v1/trec_covid_admission_diagnostic.json`
- Local diagnostic copy:
  `/tmp/trec_covid_admission_diagnostic.json`
- Variant: `df_le_0p25`
- Candidate K: `160`
- Unified candidate scales: `0.25`, `0.5`
- Seed / split seed: `1050` / `1050`

## Diagnostic Results

### Qrel Normalization

| Surface | Queries | Positive queries | Qrel docs | Missing qrel docs |
| --- | ---: | ---: | ---: | ---: |
| Raw qrels | 50 | 50 | 24,673 | 24,673 |
| Normalized qrels | 50 | 50 | 24,673 | 0 |

The raw qrel doc IDs do not directly match the corpus IDs, but the existing
normalization path fixes that completely. This is expected behavior and not
the failure source.

### Candidate Admission

| Split | Queries | BM25 hit rate | Atom hit rate | Unified hit rate | Candidate hit rate | Cache score hit rate | Candidate misses |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| All | 50 | 1.0000 | 0.9000 | 1.0000 | 1.0000 | 1.0000 | 0 |
| Train | 35 | 1.0000 | 0.9143 | 1.0000 | 1.0000 | 1.0000 | 0 |
| Heldout | 15 | 1.0000 | 0.8667 | 1.0000 | 1.0000 | 1.0000 | 0 |

Heldout admission is not collapsed: every heldout query has at least one qrel
positive in BM25 topK / unified candidate union / compact cache score maps.

## Root Cause

`research_sae_m322_candidate_pool_scorer.py` built a global
`qrels_by_query` map with raw query IDs:

```python
qrels_by_query.update(dataset_info['qrels'])
```

That is unsafe in broad multi-dataset evaluation because datasets can share
query IDs such as `2`, `10`, `11`, etc. When later datasets overwrite earlier
entries, examples from `trec-covid` can be evaluated against qrels from another
dataset. That makes candidate upper bound appear as zero even though the cache
contains correct positives.

## Fix

The evaluator now namespaces evaluation qrel keys as:

```text
<dataset>::<query_id>
```

This preserves raw query IDs inside examples and diagnostics while preventing
cross-dataset qrel collisions in:

- model-selection metrics;
- learned scorer evaluation;
- candidate upper-bound evaluation;
- query diagnostics.

## Corrected Broad-8 Rerun

The namespace fix has been applied in:

- `scripts/research_sae_m322_candidate_pool_scorer.py`

The corrected broad-8 rerun completed on spark-1:

- output JSON:
  `/home/huoju/leask/runs/ii42-m335-interaction-feature-broad8-v1/m335_interaction_feature_broad8_seed1050_split1050_qidfix.json`
- local copy:
  `/tmp/m335_interaction_feature_broad8_seed1050_split1050_qidfix.json`

### Heldout Aggregate

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.5104 | 0.2804 | 0.2748 | 0.2374 |
| unified scale 0.5 | 0.5485 | 0.2981 | 0.2871 | 0.2490 |
| atom BM25 | 0.2039 | 0.0597 | 0.0600 | 0.0476 |
| M335 scorer, qidfix | 0.8000 | 0.4370 | 0.4238 | 0.3700 |
| candidate upper bound, qidfix | 0.8754 | 1.0000 | 0.9137 | 0.8754 |

The corrected aggregate is slightly better than the original pre-fix result:
`+0.0013` Recall@100, `+0.0030` MRR@20, `+0.0021` NDCG@10, and `+0.0007`
MAP@100.

### Corrected TREC-COVID Heldout

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.0665 | n/a | 0.4485 | n/a |
| M335 scorer, qidfix | 0.0808 | 0.7917 | 0.5029 | 0.0453 |
| candidate upper bound, qidfix | 0.1188 | 1.0000 | 1.0000 | 0.1188 |

The original zero was an evaluator artifact. The corrected result shows a
different issue: top-rank quality is good once a positive is admitted, but
Recall@100 stays low because `trec-covid` has many qrel-positive documents per
query and the current candidate topK admits only a small fraction of them.

## M337 Candidate-K Sweep

M337 reused the existing score maps without rebuilding embeddings and varied
only candidate admission depth. This tests whether the current score surface has
usable recall that is being cut off by the top160 admission policy.

| Dataset | K | Candidate hit | Candidate qrel recall | Cache qrel recall |
| --- | ---: | ---: | ---: | ---: |
| `cqadupstack` | 320 | 0.6536 | 0.4825 | 0.4825 |
| `cqadupstack` | 640 | 0.6536 | 0.4825 | 0.4825 |
| `cqadupstack` | 1000 | 0.6536 | 0.4825 | 0.4825 |
| `fiqa` | 320 | 0.8093 | 0.6345 | 0.9979 |
| `fiqa` | 640 | 0.8711 | 0.6858 | 0.9979 |
| `fiqa` | 1000 | 0.8763 | 0.7125 | 0.9979 |
| `nfcorpus` | 320 | 0.8454 | 0.2679 | 0.5431 |
| `nfcorpus` | 640 | 0.8763 | 0.3208 | 0.5431 |
| `nfcorpus` | 1000 | 0.8763 | 0.3704 | 0.5431 |
| `scidocs` | 320 | 0.8567 | 0.4767 | 0.9804 |
| `scidocs` | 640 | 0.9033 | 0.5382 | 0.9804 |
| `scidocs` | 1000 | 0.9267 | 0.5713 | 0.9804 |
| `trec-covid` | 320 | 1.0000 | 0.1543 | 0.9931 |
| `trec-covid` | 640 | 1.0000 | 0.2123 | 0.9931 |
| `trec-covid` | 1000 | 1.0000 | 0.2517 | 0.9931 |
| `webis-touche2020` | 320 | 1.0000 | 0.6620 | 1.0000 |
| `webis-touche2020` | 640 | 1.0000 | 0.7561 | 1.0000 |
| `webis-touche2020` | 1000 | 1.0000 | 0.7944 | 1.0000 |

The important pattern is that `fiqa`, `scidocs`, `trec-covid`, and
`webis-touche2020` have high cache-level qrel recall, but topK admission
throws much of it away. `cqadupstack` is different: cache qrel recall is already
the ceiling, so it needs a deeper source surface, not just a larger candidate K.

## Implication

M335 should not be classified as a failed route. The reported `trec-covid`
collapse was an evaluator bug, and the corrected broad-8 scorer remains strong.

The next blocker is not generic scorer training. It is admission: the score maps
often contain the relevant documents, but the current fixed top160 candidate
policy discards too many positives on multi-positive datasets. M338 should
therefore test a learned admission policy over the existing score-map surface
before any expensive encoder retraining.
