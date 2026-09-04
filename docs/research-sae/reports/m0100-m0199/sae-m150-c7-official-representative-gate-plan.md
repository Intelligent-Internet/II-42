# SAE M150 C7 Official Representative Gate Plan

Date: 2026-05-31

## Summary

C6 shows that the representation is strong on the M130 continuity full-corpus
surface, but the learned BM25+SAE fusion scale over-weights BM25. C7 then
found stable fixed profiles that improve the C6 result without retraining:

| Profile | Definition | Role |
| --- | --- | --- |
| `sae` | standalone SAE ranking | MRR/top-rank semantic baseline |
| `rank_boost` | `SAE + 0.05 * BM25(top100)` | balanced MAP/ranking profile |
| `ndcg_boost` | `SAE + 0.10 * BM25(top100)` | NDCG control |
| `recall_boost` | `SAE + 0.20 * BM25(top20)` | Recall@100 control |
| `recall20_boost` | `SAE + 0.35 * BM25(top10)` | Recall@20 control |

The next step is not another Stage-C training run. The next step is an
official representative full-corpus gate for these fixed C7 profiles.

## Why This Gate Exists

The continuity-surface result is not enough. The official BEIR gate previously
exposed a larger distribution gap, especially on QA-style and large-corpus
datasets. C7 fixed profiles must therefore be re-scored on official full-corpus
ranking artifacts before promotion.

Existing local official artifacts under
`results/sae/official-beir-mainline-recall-current/` are useful only as a
collector smoke test. They are from the older
`bm25sae-pplx16384k96-stagec-v1` checkpoint with `doc64/query80`, not the C6
checkpoint. They must not be used as the C6/C7 promotion gate.

## Representative Datasets

Run full-corpus official `all-test` gates in this order:

| Dataset | Reason |
| --- | --- |
| `fiqa` | small/medium financial search; fast regression signal |
| `scidocs` | known ranking/semantic weakness |
| `cqadupstack` | large QA-style distribution gap |
| `trec-covid` | biomedical long-doc/semantic coverage stress |
| `msmarco` | largest short-query engineering stress surface |

If `msmarco` becomes too expensive, run it as a query-sampled canary first, but
do not treat the sampled result as the final official quality gate.

## Runner

```text
scripts/run_m150_c7_official_representative_gate_spark.sh
```

Default run:

```text
bm25sae-m150-a1-c7-official-gate-v1
```

Default checkpoint:

```text
/home/huoju/leask/runs/bm25sae-m150-a1-c6-official-dense-miss-v1/bm25sae_stageb_best.pt
```

Default retrieval settings:

| Setting | Value |
| --- | ---: |
| `doc_active_k` | `96` |
| `query_active_k` | `96` |
| `max_df_ratio` | `0.12` |
| split | `all-test` |

The runner reuses the already materialized PPLX document embeddings under
`/home/huoju/leask/runs/m150-beir-full-pplx/<dataset>/documents.jsonl`, embeds
official test queries, runs the full-corpus evaluator, and writes
`m110_full_corpus_rankings.jsonl` for each dataset.

## Collector

```text
scripts/research_sae_m150_collect_c7_official_profiles.py
```

The collector re-scores existing official ranking JSONL artifacts with the C7
fixed profiles and emits:

```text
m150_c7_official_profiles.json
m150_c7_official_profiles.md
```

It does not retrain and does not sample the corpus.

## Acceptance Gate

C7 can be promoted only if the fixed profiles provide a clear full-corpus
improvement over both:

- the C6 learned BM25+SAE score fusion
- the prior M150/M130 official representative result

Promotion requires all of:

1. `rank_boost` or `recall_boost` improves the official representative macro
   Recall@100 or MAP@100 versus learned fusion.
2. No representative dataset shows a large MRR/NDCG collapse versus standalone
   SAE.
3. The profile narrows the BM25+dense gap on `fiqa`, `scidocs`, and
   `cqadupstack`.
4. Physical cost stays within the current C6/C7 `doc96/query96/max_df=0.12`
   envelope.

If fixed profiles do not transfer to official gates, the next model work should
target official semantic coverage directly. Do not keep tuning fusion weights
on the M130 continuity surface.
