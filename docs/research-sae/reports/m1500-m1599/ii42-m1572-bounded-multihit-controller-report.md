# M1572 Bounded Multi-Hit Controller Report

## Decision

**Stop LSH and close the single-hop semantic posting source branch.**

M1572 uses the full predeclared `0.30x` posting-read budget and improves the
failed M1571 first-hit source, but remains far below M1565 route1000 and every
dense-equivalence gate. The strong radius-2 collision coverage cannot be
converted into a precise candidate set by cross-table collision counts.

Do not test alternative collision weights, per-table quotas, block sizes,
conjunctive hash keys, read budgets, or learned hashes on this branch.

## Surface And Integrity

- Official FiQA: 57,638 documents and 648 queries.
- Exact frozen M1571 hyperplanes, codes, radius-2 probes, and posting lists.
- Fixed 17,291 decoded postings per query (`0.299993x`).
- Exactly 1,256 unique output candidates per query.
- Maximum per-query collision count never exceeded 16 tables.
- Qrels-free candidate-surface SHA-256:
  `207ab4f6a0cb8160a43f28a5feb606c0fede6d725d6334564e613159ac11e88f`.
- ClearML task: `d719e308defc428a83994249d47c5e3d`.
- Runtime: 24.8 seconds.

## Result

| Variant | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M1571 first hit | 0.110494 | 0.077716 | 0.067570 | 0.081157 | 0.078132 | 0.061788 | 0.124024 | 0.084180 | 0.021920x |
| M1572 multi-hit | 0.286883 | 0.174907 | 0.141409 | 0.253876 | 0.209364 | 0.165130 | 0.301316 | 0.259486 | 0.299993x |
| M1565 route1000 | 0.921759 | 0.808503 | 0.702311 | 0.683089 | 0.383019 | 0.322550 | 0.468258 | 0.822706 | 0.021159x |

At the fixed read cap, M1572 observes 15,114 documents per query on average,
or 26.22% of the corpus. Selected candidates average only 2.15 table
collisions; p95 is 2.98. The extra reads therefore collect broad, weak
collision evidence rather than concentrating dense neighbors.

## Interpretation

M1571's radius coverage measures whether each dense neighbor has at least one
near hash code among 16 tables. M1572 shows that this event is not selective:
many non-neighbors satisfy comparable events, especially under BGE's
anisotropic hot buckets. Collision existence is high-recall but extremely
low-precision.

The failure is structural. A different score over the same observed votes
cannot bridge an O@100 gap of more than `0.63` to route1000. Increasing reads
would approach a broad corpus scan, repeating M1540/M1541 rather than creating
a deployable inverted source.

## Consequence

M1572 closes the final M1560+ single-hop source mechanism. The next work must
not be another post-hoc code, route, hash, term selector, or query gate. If a
pure unified posting index remains the research objective, it requires a new
retrieval-native discrete backbone training program in which corpus balance
and neighborhood retrieval are learned together, not a compiler attached to
an already frozen dense geometry.
