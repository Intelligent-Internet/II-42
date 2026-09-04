# SAE M35 Teacher-Neighborhood Distillation Plan

Status: closed after M35 q100 and M35b diversified full15 runs.

## Summary

M34 showed that `trec-covid` is not just a biomedical vocabulary problem. It
represents a broader retrieval class:

```text
high-DF broad natural-language query
+ many relevant documents
+ teacher has large semantic-neighborhood advantage over BM25
```

M35 therefore changes the supervision source. Instead of M33's
`query-like text -> same document` pseudo self-qrels, M35 trains on:

```text
broad query-like text -> teacher top-k semantic neighborhood
```

This is still not `trec-covid` specialization. The builder selects broad
high-DF pseudo queries from multiple corpora and labels them with the current
Snowflake-SAE teacher distribution. Runtime/product logic remains unchanged:

```text
text -> atoms -> unified sparse evidence engine
```

## Workstreams

### M35.0 Artifact Builder

Build a reusable teacher-neighborhood artifact generator:

- Input: an already materialized BEIR-style root with document embeddings and
  teacher doc latents.
- Query generation: extract title / first useful sentence candidates from
  documents, score candidates by content-token document frequency, and
  questionize them to better match broad natural-language query style.
- Teacher labeling: encode generated query text with the canonical Snowflake
  query prefix, encode SAE query atoms, run the `BM25 + SAE` teacher, and write
  teacher top-k as pseudo neighborhood labels.
- Output: a normal dataset directory with `documents.jsonl`, `queries.jsonl`,
  `quality_qrels.json`, `qrels.jsonl`, and `shared_sae_8192_64/query_latents`.
- Mark all generated labels as `quality_claim_allowed=false`.

### M35.1 First Training Arm

Merge:

- M32 general train split; and
- M35 teacher-neighborhood pseudo datasets.

Use the same M32 teacher-anchor final-ranking configuration:

- fixed teacher doc atoms;
- query-side text-to-atoms training only;
- teacher listwise target;
- qrel residual from teacher-neighborhood pseudo labels;
- BM25-preservation and fanout penalties unchanged.

### M35.2 Evaluation

Evaluate against the same current full15 regression surface:

- full15 aggregate quality;
- hard collapse table for `trec-covid`, `msmarco`, `dbpedia-entity`;
- physical cost: candidate docs, BM25 postings, SAE postings.

Do not use generated pseudo labels as quality evidence.

## Acceptance Gate

| Gate | Requirement |
| --- | --- |
| `trec-covid` | materially improve NDCG@10 and MAP@100 versus M32/M33 without hiding behind aggregate mean |
| `msmarco` / `dbpedia-entity` | no worse collapse than M32 teacher-anchor |
| Aggregate | full15 mean should remain near or above M32 teacher-anchor |
| Cost | SAE postings should remain near the M32/M33 profile unless quality gain is decisive |

## Result

M35 implemented the artifact builder and ran two full training arms:

- `m35-neighborhood-q100`
- `m35-neighborhood-diverse-q100`

Both failed the no-collapse gate. The first arm gave a small aggregate
improvement but did not fix `trec-covid`. The diversified retry reduced
physical cost and improved some aggregate ranking metrics, but worsened
`trec-covid` NDCG/MAP. See
`sae-m35-teacher-neighborhood-distillation-results-report.md`.

The stop condition is now met. Do not keep sweeping document-sentence
pseudo-query generation. The next valid direction is a query-distribution
reset: real train/dev query replay, synthetic-query validation before training,
or a stronger query encoder under the same fixed-doc no-collapse gate.
