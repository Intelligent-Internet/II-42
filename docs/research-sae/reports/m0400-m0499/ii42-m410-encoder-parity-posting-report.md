# II-42 M410 Encoder Parity Posting Report

Date: 2026-06-27

## Question

M410 checks whether raw text can be passed through the actual dense encoder and
recover the materialized dense/posting geometry:

```text
raw text -> pplx dense encoder -> deterministic M408 posting
```

This is the required bridge between M409 and any smaller student encoder.

## Runs

Remote hosts: `spark-2`, `spark-1`

Artifacts:

- `/home/huoju/leask/runs/ii42-m410-encoder-parity-posting-v1/fiqa_parity_seed410/m410_fiqa_parity_seed410.json`
- `/home/huoju/leask/runs/ii42-m410-encoder-parity-posting-v1/fiqa_parity_noprefix_seed410/m410_fiqa_parity_noprefix_seed410.json`
- `/home/huoju/leask/runs/ii42-m410-encoder-parity-posting-v1/fiqa_spark1_queryonly_noprefix_seed410/m410_fiqa_spark1_queryonly_noprefix_seed410.json`

Model:

- `perplexity-ai/pplx-embed-v1-0.6B`
- SentenceTransformer modules: Transformer, mean pooling, FlexibleQuantizer
- output dimension: `1024`

## Sample Parity Matrix

FiQA sample: 256 documents, 128 queries.

| Run | Query prefix | Doc dense cos | Query dense cos | Doc active Jaccard | Query active Jaccard | Doc sketch cos | Query sketch cos |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `fiqa_parity_seed410` | `query: ` | 0.99919 | 0.91077 | 0.99412 | 0.56938 | 0.99515 | 0.93773 |
| `fiqa_parity_noprefix_seed410` | none | 0.99919 | 1.00000 | 0.99412 | 1.00000 | 0.99515 | 1.00000 |

## Findings

1. Document parity passes: re-encoded documents match the materialized dense
   vectors and posting support almost exactly.
2. Query parity fails when `query: ` is added.  This proves the materialized
   query embeddings were generated without that prefix.
3. Query parity is perfect with no prefix.  Therefore the correct M410/M411
   raw-text path for this data is plain text for both documents and queries.
4. The M409 collapse was not caused by impossible posting targets.  It was
   caused by trying to replace a semantic dense encoder with a lexical hash
   model.

## Current State

Full FiQA retrieval was run with:

```text
--query-prefix ""
```

The final `spark-1` gate reused materialized document vectors and re-encoded
all FiQA queries from raw text.  This avoids the slow full-document re-encode
while directly testing the query-side runtime path that changes ranking.

## Full FiQA Query-Only Retrieval Gate

FiQA heldout split: 324 eval queries, 57,638 documents.

| Route | NDCG@10 | Recall@100 | MRR@20 | MAP@100 | Overlap@100 | Touched ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense_materialized` | 0.54555 | 0.84599 | 0.63475 | 0.48868 | 1.00000 | 1.00000 |
| `exact_dense_reencoded` | 0.54530 | 0.84599 | 0.63462 | 0.48850 | 1.00000 | 1.00000 |
| `m410_materialized_structural_tail` | 0.44514 | 0.63166 | 0.54608 | 0.38564 | 0.55290 | 0.08002 |
| `m410_reencoded_postprocess` | 0.44468 | 0.63166 | 0.54582 | 0.38587 | 0.55281 | 0.08002 |
| `m410_materialized_structural_tail_bm25_zblend_a010` | 0.54140 | 0.83941 | 0.63280 | 0.48476 | 0.79679 | 0.14526 |
| `m410_reencoded_postprocess_bm25_zblend_a010` | 0.54135 | 0.83941 | 0.63264 | 0.48472 | 0.79685 | 0.14525 |

Encoder parity from the same run:

- document dense cosine: `0.99730611`
- document active-support Jaccard: `0.98536978`
- query dense cosine: `0.99998868`
- query active-support Jaccard: `0.99322264`
- query sketch cosine: `0.99999559`

## Conclusion

M410 passes.  The raw text path can recover the materialized dense/posting
ranking surface when it uses the same `pplx` encoder and no query prefix.  The
remaining problem is not the deterministic posting target; it is training a
smaller/search-optimized student that approximates this dense encoder well
enough before applying posting constraints.

## Next

1. Start M411:

```text
raw text -> smaller/pretrained student encoder -> dense/posting target
```

2. M411 should train in stages:
   - dense vector imitation;
   - posting coordinate and active-support imitation;
   - optional retrieval-aware fine-tuning only after representation parity is
     strong.
