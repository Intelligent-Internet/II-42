# SAE Phase 5 Archived Explorations

Date: 2026-05-12

This file archives Phase 5 exploration items that have enough evidence for the
current planning cycle. Archived does not mean permanently closed. It means the
next active work should not spend more cycles on the same question until new
data or a new implementation path changes the tradeoff.

## Archived As Done For Now

| Topic | Evidence | Current decision |
| --- | --- | --- |
| Raw score contract | `sae-phase5-gap-exploration-report.md` | Do not use raw weighted score as the first native contract. |
| Fixed saturation contract | `sae-phase5-gap-exploration-report.md` | Promote to the first native scorer candidate. |
| BM25+dense+SAE tri-hybrid | `sae-phase5-gap-exploration-report.md` | SAE adds value even when dense is present; do not claim dense can be universally removed yet. |
| Query active-dim budget | `sae-phase5-gap-exploration-report.md` | Keep top-16/top-32 as the next native candidate budgets. |
| Document active-dim budget | `sae-phase5-gap-exploration-report.md` | Keep top-48 as the first compression candidate. |
| Naive unigram+bigram BM25 | `sae-phase5-gap-exploration-report.md` | Do not continue this as a mainline learned-sparse replacement. |
| Phase 5 physical counters | `sae-phase5-gap-exploration-report.md` | Candidate fanout and resident doc-vector decode cost remain the main systems problems. |
| SPLADE v2-distil baseline | `sae-splade-baseline-report.md` | Keep as an offline reference baseline; do not move this checkpoint into SQL/native now. |

## Still Active

| Topic | Reason |
| --- | --- |
| Stronger SPLADE variants | `naver/splade_v2_distil` is now tested. Reopen only for stronger checkpoints, larger active budgets, or real workload evidence. |
| BGE-M3 sparse baseline | Existing local BGE-M3 artifacts exist, but `FlagEmbedding` is not installed in this environment. Keep as a follow-up baseline. |
| Real arxiv/pubmed/commons qrels | Needed before claiming dense replacement or production relevance for the target RAG workload. |

## SPLADE Baseline Result

SPLADE was explored at the offline matrix layer before SQL sidecar work. The
database-facing representation remains the same generic sparse impact shape:

```text
source_id = splade
source_dim_id = token_id
weight = learned sparse lexical/expansion impact
```

The first SPLADE pass compared:

```text
BM25
BM25+SAE
SPLADE
BM25+SPLADE
BM25+SAE+SPLADE
```

The normalized five-dataset mean did not justify SQL integration:

| Source | Recall@100 | MRR@20 | NDCG@10 |
| --- | ---: | ---: | ---: |
| `bm25_sae` | 0.7947 | 0.6835 | 0.6036 |
| `splade` | 0.7519 | 0.6324 | 0.5559 |
| `bm25_splade` | 0.7445 | 0.6402 | 0.5570 |
| `bm25_sae_splade` | 0.7926 | 0.6827 | 0.6087 |

The tri-source path improves NDCG slightly, but not enough to justify adding a
new SQL/native source for this checkpoint.

The report also records why this does not invalidate SPLADE as a direction:
SAE has corpus-adapted Snowflake semantic geometry, SPLADE was capped at
`doc_active_dims = 128`, fusion weights were not swept, and v2-distil is only a
practical public baseline.
