# SAE Milestone 22 Full15 Miss Taxonomy Report

## Scope

This follow-up run answers two operational questions before another training
round:

- whether the latest `doc88/query96` deployment profile loses quality mostly
  in the native active-row traversal layer;
- whether further training should target semantic coverage or physical posting
  fanout.

The run also moves reusable BEIR working data away from `/tmp`. The stable local
scratch root is now:

```text
/Volumes/Betty/Tmp/ii42_sae_beir15_shared
```

This directory stores rebuildable documents, queries, qrels, embeddings, and
intermediate working artifacts. Final reports and evaluation outputs still live
under repo `results/`.

## Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_text_atom_miss_taxonomy.py \
    --data-root /Volumes/Betty/Tmp/ii42_sae_beir15_shared \
    --eval-dir \
        results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-doc80-88-96-eval \
    --active-run doc-88-query-96 \
    --datasets \
        arguana climate-fever cqadupstack dbpedia-entity fever fiqa \
        hotpotqa msmarco nfcorpus nq quora scidocs scifact \
        trec-covid webis-touche2020 \
    --output-dir \
        results/sae/text-atoms/m22-token-lse-tokenchar-h256-asym96-budget8-full15-miss-taxonomy \
    --sample-examples 8
```

## Macro Metrics

| Source | Queries | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 1342 | 0.8031 | 0.7633 | 0.6635 | 0.6145 |
| `dense` | 1342 | 0.8744 | 0.8575 | 0.7712 | 0.7293 |
| `student_full` | 1342 | 0.8111 | 0.7847 | 0.6842 | 0.6321 |
| `native_active` | 1342 | 0.8101 | 0.7856 | 0.6844 | 0.6297 |

`native_active` is effectively at parity with `student_full` for this
configuration. The remaining gap to dense retrieval is therefore mostly model
representation and training coverage, not the current active-row traversal.

## Miss Categories

| Category | Queries | Share |
| --- | ---: | ---: |
| `covered` | 1283 | 0.9560 |
| `semantic_recovered` | 12 | 0.0089 |
| `student_encoder_loss` | 30 | 0.0224 |
| `teacher_or_qrels_hard_miss` | 13 | 0.0097 |
| `native_traversal_loss` | 1 | 0.0007 |
| `native_hit_without_bm25_dense` | 2 | 0.0015 |
| `native_only_or_weight_shift` | 1 | 0.0007 |

The important signal is the asymmetry between `student_encoder_loss` and
`native_traversal_loss`: only one query is lost by active traversal after the
full sparse student would have found a positive, while 30 queries are recovered
by dense but not by the sparse student.

## Query Slice Signal

`student_encoder_loss` concentrates in the slices we already expected:

| Slice | Student Encoder Loss Queries |
| --- | ---: |
| `semantic_heavy` | 27 |
| `long_query` | 20 |
| `short_query` | 4 |
| `balanced_or_hard` | 3 |

This says another round should not primarily tune fusion weights or widen the
native candidate traversal. The next training attempt should improve
text-to-atom semantic coverage for dense-positive misses, especially
semantic-heavy and long-query cases, while keeping posting fanout bounded.

## Operational Note

No important final result was lost in the previous temporary-directory cleanup:
the committed reports, checkpoints, evaluation matrices, and repo `results/`
artifacts remain available. The lost work was only the rebuildable `/tmp`
working copy for sampled BEIR data and derived embeddings. This run rebuilt the
needed shared data root under `/Volumes/Betty/Tmp`.

## Next Step

The next training step should be a targeted semantic-coverage compression run:

- mine the 30 `student_encoder_loss` cases plus nearby dense positives as
  hard positives;
- add semantic-heavy and long-query weighting to the student objective;
- keep the doc88/query96 deployment profile fixed as the primary validation
  profile;
- reject any checkpoint that improves those misses by increasing native posting
  fanout without improving full15 NDCG/MAP.

The acceptance check should remain full15 and apples-to-apples: `bm25`,
`dense`, `student_full`, and `native_active`, with physical cost reported next
to quality.
