# M1177 Tail Rank-Distillation Supervision

M1177 exports qrel-aware pair/listwise supervision from M1174 doc-level
rank movements.  It is an objective-design artifact for the next training
branch, not a runtime gate or final scorer.

Artifacts:

- Script: `scripts/audit_m1177_tail_rank_distill_supervision.py`
- Summary JSON: `runs/m1177_tail_rank_distill_supervision_v1/tail_rank_distill_supervision.json`
- Pair examples: `runs/m1177_tail_rank_distill_supervision_v1/pair_examples.jsonl`
- Listwise docs: `runs/m1177_tail_rank_distill_supervision_v1/listwise_docs.jsonl`
- Query summary: `runs/m1177_tail_rank_distill_supervision_v1/query_summary.jsonl`

## Export Summary

- Queries: `138`.
- Listwise doc rows: `13,882`.
- Pair examples: `44,184`.
- Queries with pair examples: `118`.
- Pair count before per-query cap: `60,669`.

Pair sources:

| Source | Count |
| --- | ---: |
| rank_teacher_positive | 18,517 |
| harm_penalty | 25,667 |

Dataset split:

| Dataset | rank_teacher_positive | harm_penalty |
| --- | ---: | ---: |
| arguana | 196 | 0 |
| dbpedia-entity | 12,011 | 8,297 |
| fiqa | 2 | 96 |
| msmarco | 2,802 | 10,754 |
| nfcorpus | 296 | 1,008 |
| trec-covid | 3,210 | 5,512 |

## Interpretation

The supervision is large enough for a small signal test and is not restricted
to a single dataset.  However, harm examples dominate on `msmarco`, `nfcorpus`,
and `trec-covid`, so any objective that blindly imitates tail movement will
damage top-rank geometry.

The next test must answer whether this supervision can train a deployable
rank objective while preserving direct/P1 safety floors.
