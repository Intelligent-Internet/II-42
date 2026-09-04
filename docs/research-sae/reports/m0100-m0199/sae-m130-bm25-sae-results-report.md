# SAE M130 bm25sae Results Report

Date: 2026-05-24

## Status

M130 is the current completed candidate baseline. Stage A, Stage B, and
Stage C have completed on the full PPLX all-data surface. Stage C passes the
stricter dense and BM25+dense gate, but the default `k96/k96` sparse profile
has too much fanout. The current production-shaped evaluation profile is
therefore `doc_active_k=64` and `query_active_k=80`.

## Data Surface

| Surface | Rows |
| --- | ---: |
| Full document index | 993,336 |
| Representation queries | 205,059 |
| Qrel-backed train queries | 5,620 |
| Held-out eval queries | 886 |

Held-out eval artifacts:

```text
/home/huoju/leask/runs/m130-pplx-all-data-eval-corpus
```

Stage-B full-corpus artifacts:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stageb-v2
/home/huoju/leask/runs/bm25sae-pplx16384k96-stageb-v2-full-corpus-eval
```

Stage-C full-corpus artifacts:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1-full-corpus-eval
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1-full-corpus-eval-doc64-q80
```

## Stage A

Run:

```text
bm25sae-pplx16384k96-stagea-v2
```

Checkpoint:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagea-v2/bm25sae_stagea_best.pt
```

Summary:

| Metric | Value |
| --- | ---: |
| Examples processed | 3,595,185 |
| Document examples | 2,980,008 |
| Query examples | 615,177 |
| Best step | 5,500 |
| Best loss | 0.023576 |
| Best neighbor overlap | 0.858594 |

## Stage B Candidate-Surface Result

Run:

```text
bm25sae-pplx16384k96-stageb-v2
```

The candidate-surface eval is strong but is not the promotion gate:

| Row | hit@20 | MRR@20 |
| --- | ---: | ---: |
| BM25 | 0.6061 | 0.3951 |
| Dense | 0.6817 | 0.4677 |
| SAE model | 0.8623 | 0.6247 |
| BM25+SAE model | 0.8578 | 0.6203 |

## Stage B Full-Corpus Gate

Held-out full-corpus eval:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stageb-v2-full-corpus-eval/m110_full_corpus_index_eval.json
```

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3155 | 0.2851 | 0.2115 | 0.1393 |
| SAE | 0.3185 | 0.3308 | 0.2341 | 0.1505 |
| BM25+SAE RRF | 0.3266 | 0.3374 | 0.2289 | 0.1390 |
| BM25+SAE score fusion | 0.3281 | 0.3448 | 0.2351 | 0.1495 |

Interpretation:

- BM25+SAE score fusion beats BM25+dense score fusion on Recall@100, MRR@20,
  NDCG@10, and MAP@100.
- BM25+SAE score fusion beats dense-only on Recall@100, MRR@20, and NDCG@10.
- BM25+SAE score fusion is still slightly behind dense-only on MAP@100
  (`0.1495` vs `0.1504`).
- SAE alone has the best MAP@100 among the Stage-B rows (`0.1505`), but the
  promoted product path is still BM25+SAE because the goal is one unified
  lexical/semantic evidence surface.

## Physical Cost

Stage-B full-corpus diagnostics:

| Row | Postings touched/query | Accumulator entries/query | Elapsed/query |
| --- | ---: | ---: | ---: |
| BM25 | 1,829,152 | 475,879 | n/a |
| SAE | 1,752,814 | 652,513 | 1.065 s |

Interpretation:

- Stage-B quality is strong enough to pass the canonical M130 gate.
- SAE query fanout is still too high for a production-shaped query path.
- Stage C should therefore optimize MAP and fanout together, not just ranking
  quality.

## Miss Taxonomy

Stage-B miss taxonomy:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stageb-v2-full-corpus-eval/bm25sae_miss_taxonomy.json
```

| Category | Count |
| --- | ---: |
| Covered by BM25+SAE | 3,638 |
| BM25+dense relevant hits | 3,351 |
| Dense relevant hits | 3,449 |
| Candidate hit but score low | 1,252 |
| Dense hit but SAE candidate missed | 359 |
| Dense-only candidate missed | 320 |
| Not retrieved by controls | 17,906 |

Interpretation:

- BM25+SAE is recovering more relevant hits than BM25+dense in this held-out
  surface.
- The actionable remaining gap is split between score ordering and dense-only
  semantic misses.
- Stage C should refresh candidate rows from actual Stage-B full-corpus
  behavior and continue training without using eval qrels as train labels.

## Stage C

Stage C completed:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1
```

Stage C was configured to:

- generate train split full-corpus Stage-B rankings against
  `/home/huoju/leask/runs/m130-pplx-all-data-corpus`;
- build `candidate_k=120` candidate rows from actual BM25+SAE, SAE, dense,
  BM25+dense, and BM25 rankings;
- continue from `bm25sae-pplx16384k96-stageb-v2`;
- re-run the held-out full-corpus gate with BM25 rankings reused from cache.

Default Stage-C `k96/k96` full-corpus eval:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1-full-corpus-eval/m110_full_corpus_index_eval.json
```

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3154 | 0.2849 | 0.2111 | 0.1391 |
| SAE | 0.3394 | 0.3653 | 0.2570 | 0.1642 |
| BM25+SAE score fusion | 0.3388 | 0.3648 | 0.2490 | 0.1583 |

Interpretation:

- Stage C beats dense-only and BM25+dense score fusion on all reported
  full-corpus quality metrics.
- The new bottleneck is physical cost: default Stage C increases SAE postings
  to `1.981M/query`, accumulator entries to `691.958k/query`, and sparse
  elapsed to about `1.125s/query`.

## Fanout/Latency Sweep

The fanout sweep tested post-hoc sparse profile changes before retraining.

Artifacts:

```text
/home/huoju/leask/runs/bm25sae-m130-fanout-sweep-sample-v1
/home/huoju/leask/runs/bm25sae-m130-fanout-docq-sample-v1
/home/huoju/leask/runs/bm25sae-m130-fanout-full-doc64-q80-v1
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1-full-corpus-eval-doc64-q80
```

Key findings:

- Global DF stoplist is not a safe default. A `max_df_ratio=0.05` cap drops
  sample BM25+SAE Recall@100 from `0.3531` to `0.2937` for `query_active_k=48`,
  so high-DF atoms still carry useful semantic evidence.
- Query-only clipping helps, but doc-side clipping is better. `doc64/query80`
  keeps strong ranking quality while removing weak document-side semantic tail
  postings.
- The current production-shaped profile is `doc_active_k=64` and
  `query_active_k=80`.

Full-corpus `doc64/query80` eval:

```text
/home/huoju/leask/runs/bm25sae-pplx16384k96-stagec-v1-full-corpus-eval-doc64-q80/m110_full_corpus_index_eval.json
```

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.3154 | 0.2849 | 0.2111 | 0.1391 |
| SAE | 0.3302 | 0.3471 | 0.2457 | 0.1536 |
| BM25+SAE score fusion | 0.3349 | 0.3688 | 0.2521 | 0.1626 |

Physical comparison:

| Profile | SAE postings/query | Accumulator entries/query | Sparse elapsed/query |
| --- | ---: | ---: | ---: |
| Stage B default | 1,752,814 | 652,513 | 1.065 s |
| Stage C default `doc96/query96` | 1,980,963 | 691,958 | 1.125 s |
| Stage C clipped `doc64/query80` | 1,536,353 | 603,979 | 0.735 s |

Interpretation:

- `doc64/query80` keeps the M130 gate passed against dense and BM25+dense.
- Compared with default Stage C, it trades about `-0.0040` Recall@100 for
  higher MRR@20, NDCG@10, and MAP@100.
- It reduces SAE postings by about `22%`, accumulator entries by about `13%`,
  and sparse elapsed by about `35%` relative to default Stage C.
