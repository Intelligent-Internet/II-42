# ii42 M160A Stage-B Direct C6 Official Dense-Miss Plan

Date: 2026-06-04

## Summary

The latest `ii42-m160a-c6-style-fullsurface-v1` run closed the corrected data
pipeline, but it did not become the quality frontier. It mixed broad Stage-B
learning and C6-style dense-miss correction in one large surface. The result was
healthy and reproducible, but the continuity full-corpus gate regressed versus
the best M160A broad Stage-B checkpoint.

This run returns to the lesson from M150 C6:

1. Build a broad Stage-B representation first.
2. Then apply a narrow, official non-test dense-miss Stage-C correction.
3. Keep test qrels out of training.

## Starting Point

Use the strongest clean M160A broad Stage-B checkpoint:

```text
/home/huoju/leask/runs/bm25sae-m160a-latest-stageb-direct-v1/bm25sae_stageb_best.pt
```

That checkpoint is a better initializer than the full-surface checkpoint on the
continuity full-corpus gate:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M160A broad Stage-B direct | 0.3421 | 0.3581 | 0.2528 | 0.1557 |
| ii42 fullsurface mixed B/C | 0.3085 | 0.2693 | 0.2035 | 0.1384 |

## Training Surface

Reuse the exact C6 qrel split discipline:

| Purpose | Dataset split specs |
| --- | --- |
| Train rows | `fiqa:train,nfcorpus:train,scifact:train` |
| Validation rows | `fiqa:dev,nfcorpus:dev,quora:dev` |

The point is not to maximize row count. The point is to rebuild the specific
dense-miss and score-low correction surface that made M150 C6 strong, but with
the corrected M160A/PPLX representation.

## Runner

```text
scripts/run_ii42_m160a_stageb_direct_c6_official_dense_miss_spark.sh
```

Run name:

```text
ii42-m160a-stageb-direct-c6-official-dense-miss-v1
```

## Guardrails

- Do not use official `test.tsv` qrels for training.
- Keep PPLX 1024-dimensional materialized rows.
- Keep SAE latent size at `16384`.
- Keep `doc_active_k=96`, `query_active_k=96`, `max_df_ratio=0.12`.
- Treat BM25 as bounded correction, not as an equal semantic peer.

## Acceptance

Primary continuity gate:

- Must beat `bm25sae-m160a-latest-stageb-direct-v1` on at least one of
  Recall@100, MRR@20, NDCG@10, or MAP@100 without a large regression on the
  others.
- If it does not beat M160A broad Stage-B, then exact C6 correction does not
  transfer cleanly to this representation and the next step should be fusion
  policy rather than another dense-miss row-weight sweep.

Historical frontier reference:

```text
bm25sae-m150-a1-c6-official-dense-miss-v1
```

M150 C6 remains the historical strongest parsed continuity-surface row. This
M160A run is a transfer test, not an automatic replacement.
