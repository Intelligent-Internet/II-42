# SAE M28 Post-M27 Results Report

Status: first M28 implementation pass completed.

M28 tested whether the post-M27 blocker can be removed by either a stronger text-to-atoms encoder or a concept-vocabulary route. The answer from this pass is still negative: the read-only teacher-path harness is solid, but dense-removal remains blocked by model quality.

## Executive Result

| Area | Decision | Evidence |
| --- | --- | --- |
| T1 teacher-path harness | `allowed` | EATMH002 full15 PG strict parity remains `1.0000`; cached by-id mean latency remains sub-ms |
| T2 pretrained text-to-atoms | `failed-current-gate` | MiniLM projection improved with full15 training but stayed below `baseline_budget16` and far below `teacher_16384` |
| T3 concept vocabulary | `parked-after-m28-control` | concept atoms carry recall signal, but do not beat current student NDCG/MAP or close teacher ranking gap |
| T4 SoftSAE | `diagnostic-only` | M27 selector preserved quality but reduced postings by only `3.27%`, below the `20%` gate |

Final M28 state after this pass:

```text
teacher-path harness allowed, dense-removal blocked
```

## T1 Teacher-Path Harness

| Payload | PG strict | PG mean ms | PG p95 ms | C mean ms | Candidates | Postings | Rerank terms | Memory MB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `head16` | 1.0000 | 0.8112 | n/a | 0.1658 | 743.6 | 1048.5 | 109857.4 | 6.36 |
| `head16_doc128` | 1.0000 | 0.7460 | n/a | 0.1333 | 743.6 | 1048.5 | 84820.8 | 5.39 |

Real-corpus efficiency remains efficiency-only, not quality:

| Payload | C exact | C mean ms | Candidates | Postings | Rerank terms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `head16_scan` | 1.0000 | 0.3577 | 1079.0 | 1319.0 | 210656.7 |
| `head16_doc128_scan` | 1.0000 | 0.2503 | 1079.0 | 1319.0 | 136740.1 |

## T2 Pretrained Text-To-Atoms Reset

This pass changed the encoder architecture by using frozen `sentence-transformers/all-MiniLM-L6-v2` features followed by a learned query/doc atom projection head. It also included BM25 and dense hard-negative pair loss. This tests whether a stronger generic text encoder can close the teacher gap.

| Run | Best source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `five-train/full15-eval` | `pretrained_student_w0p25` | 0.7920 | 0.7878 | 0.6718 | 0.6318 | `failed-current-gate` |
| `full15-train/full15-eval` | `pretrained_student_w0p25` | 0.7998 | 0.7931 | 0.6803 | 0.6441 | `failed-current-gate` |

The full15-trained variant improved over the five-train variant, but still stayed below the old `baseline_budget16` row:

| Reference | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `teacher_16384` | 0.8455 | 0.8462 | 0.7530 | 0.7273 |
| `baseline_budget16` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| `best_m28_pretrained` | 0.7998 | 0.7931 | 0.6803 | 0.6441 |

Conclusion: this frozen-pretrained projection is not the needed breakthrough. It is useful negative evidence because it changes encoder architecture and adds more in-domain supervision, yet it still fails both the baseline regression gate and the teacher gap.

## T3 Concept Vocabulary Control

This pass built a token-to-SAE concept vocabulary from teacher doc atoms and tested two paths: query text to concept atoms over teacher doc atoms, and full query/doc text to concept atoms.

| Route | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE postings |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `query concept -> teacher docs` | `query_concept_teacher_docs_t8_a64_w0p25` | 0.8013 | 0.7900 | 0.6790 | 0.6461 | 3035.7 | 7008.2 |
| `query/doc concept` | `concept_docs_t8_a32_w0p5` | 0.7991 | 0.7875 | 0.6712 | 0.6426 | 3232.7 | 42389.0 |

T3 decision: `parked-after-m28-control`.

Concept atoms have recall signal but do not beat current text-student NDCG/MAP or close the teacher ranking gap.

The important observation is that concept atoms are not useless: they recover a meaningful semantic recall signal. The failure is ranking and calibration. This suggests a future concept route would need supervised ranking-aware concept learning, not a post-hoc token co-occurrence map.

## T4 SoftSAE Diagnostic

| Selector | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Postings | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `fixed high` | 0.8334 | 0.8387 | 0.7467 | 0.7110 | 985.1 | `parked` |
| `learned selected` | 0.8318 | 0.8384 | 0.7465 | 0.7102 | 952.9 | `parked` |

SoftSAE-style signals should remain diagnostics. They do not solve the product gate unless a future high-quality model creates a new fanout problem worth controlling.

## Product Gate Status

M28 does not unblock productization. The blocker is now sharper:

- PostgreSQL/runtime feasibility is not the limiting factor.
- Query-side concept atoms can recover recall but not ranking.
- Frozen generic pretrained text features are weaker than the old token-char student on full15 ranking.
- The next real attempt must change supervision/objective more deeply, or accept the teacher-path read-only prototype without a dense-removal claim.

Recommended next research direction:

1. Train a ranking-aware concept encoder end-to-end against qrels and teacher neighborhoods rather than deriving concepts from token co-occurrence.
2. Use the M28 concept map as initialization only, not final weights.
3. Keep EATMH/M21 as the physical-cost harness for every future model candidate.
