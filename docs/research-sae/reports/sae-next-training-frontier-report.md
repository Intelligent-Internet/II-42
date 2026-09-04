# SAE Next Training Frontier Report

## Scope

This run executes the five post-M22 training directions without continuing the exhausted focused-objective loss sweep. The gate is query-time quality/cost Pareto, not offline training time.

Reference checkpoint:

| Reference | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Active8 Recall@100 | Candidates | SAE Posts |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `beironly-baseline` | `bm25_student_atoms_w0p25` | 0.7867 | 0.7899 | 0.6699 | 0.6282 | 0.7731 | 670.3 | 767.3 |
| `focus6` | `bm25_student_atoms_w0p25` | 0.7865 | 0.7931 | 0.6734 | 0.6299 | 0.7761 | 768.1 | 809.6 |

## Text-To-Atoms Training Directions

| Run | Decision | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Delta NDCG vs focus6 | Delta MAP vs focus6 | Active8 Recall@100 | Candidates | SAE Posts |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `teacher-upgrade-dense-qrel` | `cost-only` | 0.7842 | 0.7877 | 0.6689 | 0.6220 | -0.0045 | -0.0079 | 0.7346 | 624.9 | 527.7 |
| `native-active8-soft` | `cost-only` | 0.7863 | 0.7858 | 0.6680 | 0.6226 | -0.0054 | -0.0073 | 0.7550 | 564.6 | 450.8 |
| `domain-mix-heldout-distill` | `cost-only` | 0.7856 | 0.7899 | 0.6711 | 0.6261 | -0.0023 | -0.0039 | 0.7655 | 521.8 | 405.9 |

## Latent Vocabulary Teacher Sweep

| Teacher | Latents | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Active8 Recall@100 | Candidates | SAE Posts | Payload MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `shared_sae_12288_64` | 12288 | 0.8491 | 0.8450 | 0.7516 | 0.7274 | 0.8373 | 500.6 | 493.2 | 6.97 |
| `shared_sae_16384_64` | 16384 | 0.8455 | 0.8462 | 0.7530 | 0.7273 | 0.8374 | 491.1 | 481.3 | 7.00 |

## Evidence-Atom Frontier

M14-M17 is the existing source-blind evidence-atom frontier. It covers retrieval-aware atom reliability, active physical candidate selection, compact doc-row rerank, and real-workload readiness.

```json
{
  "m14_frontier": {
    "baseline_label": "h16_base_full",
    "passed": false,
    "selected": {
      "label": "h16_base_full",
      "metric_map_at_100": 0.7148297413980472,
      "metric_mrr_at_20": 0.8416095322057175,
      "metric_ndcg_at_10": 0.7506091129939232,
      "metric_recall_at_100": 0.8365124994501344,
      "metric_recall_at_20": 0.7261342674259814,
      "perf_head_size": 16.0,
      "perf_mean_candidate_docs": 743.5798101566209,
      "perf_mean_candidate_postings": 1048.4624543584875,
      "perf_mean_ms": 4.6503532976424315,
      "perf_mean_query_atoms": 79.38164973896535,
      "perf_mean_rerank_doc_terms": 109857.38770953963,
      "perf_p95_candidate_docs": 836.5333333333333,
      "perf_p95_ms": 6.071019469527528,
      "perf_p95_rerank_doc_terms": 124189.13333333333
    },
    "target_mrr": 0.8376095322057175,
    "target_recall": 0.8331124994501344
  },
  "m15_gate": {
    "candidate": {
      "label": "h16_doc128",
      "metric_map_at_100": 0.7135703359034006,
      "metric_mrr_at_20": 0.8413858191098793,
      "metric_ndcg_at_10": 0.7491518415727797,
      "metric_recall_at_100": 0.8353746606065791,
      "metric_recall_at_20": 0.7236738295244488,
      "perf_head_size": 16.0,
      "perf_mean_candidate_docs": 743.5798101566209,
      "perf_mean_candidate_postings": 1048.4624543584875,
      "perf_mean_ms": 4.439269919382105,
      "perf_mean_query_atoms": 79.38164973896535,
      "perf_mean_rerank_doc_terms": 84820.79498686916,
      "perf_p95_candidate_docs": 836.5333333333333,
      "perf_p95_ms": 5.721336269440751,
      "perf_p95_rerank_doc_terms": 95803.73333333334
    },
    "mrr_delta": -0.0002237130958382405,
    "passed": true,
    "recall_drop": 0.0011378388435553388,
    "rerank_term_reduction": 0.22790085623432654
  },
  "m17_sql_gate": {
    "compact_rerank_passed": true,
    "draft_status": "blocked; keep read-only experimental functions",
    "ready_for_sql_model_draft": false,
    "real_workload_ready": false,
    "selectivity_frontier_passed": false
  }
}
```

## Direction Verdicts

| Direction | Evidence | Verdict |
| --- | --- | --- |
| Retrieval-aware atom training | M14 reliability rows and `teacher-upgrade-dense-qrel` | No promotable student checkpoint; useful negative evidence for 8192 student loss tuning. |
| Native-active8-aware objective | `native-active8-soft` plus active8 C counters | Reduces candidates/postings but loses ranking; do not continue soft budget loss on the 8192 student. |
| Teacher signal upgrade | Dense/qrel teacher variant and larger teacher sweep | Loss variant fails, but larger teachers are a strong representation signal. |
| Latent vocabulary / atom allocation | 12288 and 16384 shared teachers | Strongest positive result; 16384 has best NDCG/MRR with slightly lower active8 cost than 12288. |
| BEIR + commons proxy split | Held-out BEIR unlabeled distill and M16 readiness gate | Held-out distill does not promote; no real commons qrels are available, so commons remains cost/proxy only. |

- Promoted text-to-atoms candidates: `none`.
- Best text-to-atoms ranking signal: `domain-mix-heldout-distill`.
- Best latent-vocabulary teacher signal: `shared_sae_16384_64`.
- Mean text candidate active8 candidates: `570.5`.
- Mean text candidate active8 SAE postings: `461.5`.
- Recommendation: stop tuning the 8192 text-to-atoms loss family. Start the next training pass by distilling a student from the `shared_sae_16384_64` teacher, then rerun the same full15/active8 promotion gate.
- Caveat: the 12288/16384 rows are teacher-side BM25+SAE signals, not dense-removal student checkpoints yet.

## Artifacts

- Stable report root: `/Volumes/Betty/Tmp/ii42_sae_reports/next-training-frontier`
- Tracked report: `sae-next-training-frontier-report.md`
