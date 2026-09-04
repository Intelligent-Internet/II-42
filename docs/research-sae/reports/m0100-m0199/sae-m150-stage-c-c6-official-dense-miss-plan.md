# SAE M150 Stage C C6 Official Dense-Miss Plan

Date: 2026-05-31

## Summary

C5 completed, but it did not replace C4. It improved over normalized Stage B
and M130 Stage C on the M130 continuity surface, but it did not beat the
current C4 `doc96/query96/max_df_ratio=0.12` balanced profile. The root cause
is visible in the row surface: C5 had only `31 / 5620` training rows in the
`dense_miss` category, so the dense-hit/SAE-miss failure mode was not actually
represented strongly enough.

C6 keeps the same broad M150 representation and normalized Stage-B
initialization, but changes the Stage-C row source. Instead of relying on the
M130 continuity rows, it builds a supervised row surface from official BEIR
`train`/`dev` qrels only. Official `test` qrels remain reserved for evaluation.

## Decision

Do not train on official `test.tsv`.

Use only non-test qrels that are available locally:

| Purpose | Dataset split specs |
| --- | --- |
| Train rows | `fiqa:train,nfcorpus:train,scifact:train` |
| Validation rows | `fiqa:dev,nfcorpus:dev,quora:dev` |

The large failure datasets `scidocs`, `cqadupstack`, and `trec-covid` do not
provide local train/dev qrels, so they stay evaluation/diagnostic surfaces.
C6 should still help them only if the learned dense-miss repair generalizes.

## Runner

```text
scripts/run_m150_a1_c6_official_dense_miss_spark.sh
```

Run name:

```text
bm25sae-m150-a1-c6-official-dense-miss-v1
```

The runner performs four stages:

1. Build qrels-backed BEIR surfaces using prefixed M150 ids.
2. Run the normalized Stage-B checkpoint on each non-test surface to create
   dense, BM25, BM25+dense, SAE, and BM25+SAE rankings.
3. Build and merge candidate rows with filtered embeddings so multi-dataset
   training does not require a giant aggregate corpus file.
4. Train Stage C from normalized Stage B and evaluate the resulting checkpoint
   on the M130 continuity surface.

## Key Configuration

| Setting | Value | Reason |
| --- | ---: | --- |
| Stage-B init | `bm25sae-m150-pplx16384k96-stageb-fusionnorm-v1` | current clean M150 geometry |
| Candidate K | `160` | keep dense positives visible |
| Source top K | `300` | mine deeper dense/BM25+dense misses |
| Doc/query active | `96/96` | match current C4 profile |
| Max DF ratio | `0.12` | match current C4 profile |
| Dense teacher weight | `0.80` | stronger semantic repair than C5 |
| BM25+dense rank weight | `0.75` | train toward the dense baseline surface |
| BM25+SAE rank weight | `0.15` | avoid self-distillation loop |
| Dense-miss row weight | `4.0` | C5 underrepresented this class |
| Score-low row weight | `2.5` | fix candidates present but under-ranked |
| Steps | `9000` | larger qrels-backed row surface |

## Promotion Gate

C6 is only promoted if it clears both checks:

- On the M130 continuity surface, it must at least match C4
  `doc96/query96/max_df_ratio=0.12` on Recall@100 and not materially regress
  MRR/NDCG/MAP.
- On official representative gates, it must close the `BM25+dense` gap on
  `fiqa`, `scidocs`, and `cqadupstack` without creating a large fanout
  regression.

If C6 improves only the continuity surface but not official gates, the next
step should be a model/data change, not another Stage-C row-weight sweep.
