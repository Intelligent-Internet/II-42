# SAE M28 Post-M27 Next Phase Plan

Date: 2026-05-17
Status: active next-phase plan; first implementation pass completed

## Summary

M27 closed the previous exploration cycle without passing the productization
gate. M28 therefore resets the next phase into two explicitly separate lines:

- Engineering safety line: keep `EATMH/M21` as a read-only teacher-path
  evaluation harness.
- Model breakthrough line: reset direct `text -> atoms` training around
  stronger encoders, supervision, concept vocabulary, or teacher objectives.

M28 does not freeze SQL/API, does not start mutable index work, and does not
claim query-time dense embedding can be removed. Dense removal can only be
reopened if a new text-to-atoms or concept-encoder path passes the same M27
quality and cost gates.

The first M28 implementation pass is recorded in
`sae-m28-post-m27-results-report.md`. It did not pass the dense-removal gate:
the teacher-path harness remains usable, but both the pretrained text-to-atoms
reset and the concept-vocabulary control stayed below the current
`baseline_budget16` ranking frontier.

## M27 Inputs

M27 produced three final decisions:

| Track | Decision | Evidence |
| --- | --- | --- |
| Text-to-atoms | failed gate | scanned `290` full15 student rows; best remained `baseline_budget16` |
| SoftSAE selector | parked | learned `coverage_slope>=7.25`; quality was close to fixed high, but postings dropped only `3.27%` |
| Concept vocabulary / SPLADE | parked | `bm25_sae_splade` was below current text-student ranking quality |

The remaining teacher gap from the best text-student baseline is:

| Metric | Gap vs `teacher_16384` |
| --- | ---: |
| Recall@100 | -0.0131 |
| MRR@20 | -0.0171 |
| NDCG@10 | -0.0305 |
| MAP@100 | -0.0360 |

The M28 status label is:

```text
teacher-path harness allowed, dense-removal blocked
```

## T1: Teacher-Path Harness Stabilization

Goal: keep Snowflake-SAE teacher/read-only payload execution reproducible as a
research harness.

Allowed work:

- reuse `EATMH002/doc128` and the M21 parity/cost harness;
- rerun full15 parity and cost measurements;
- rerun real-corpus efficiency measurements for arxiv, pubmed, and policy;
- report Python/C/PostgreSQL parity, score tolerance, tie tolerance, mean/p95
  latency, candidate docs, postings, rerank terms, resident memory, and payload
  MB.

Forbidden work:

- stable product API freeze;
- mutable index or maintenance design;
- dense-removal claim;
- target-workload Recall/MRR/NDCG/MAP without qrels or defensible proxy-qrels.

## T2: Direct Text-To-Atoms Model Reset

Goal: reduce the ranking-quality gap between `baseline_budget16` and
`teacher_16384`.

M28 should not continue the current tiny text-student family with more
weight/budget sweeps. Any new run must change at least one core factor:

- encoder architecture;
- retrieval supervision;
- concept vocabulary;
- teacher objective.

Required supervision mix:

- qrels positives;
- BM25 hard negatives;
- dense near-misses;
- SAE near-misses;
- candidate-budget and fanout terms.

Promotion gate, inherited from M27:

| Gate | Requirement |
| --- | --- |
| Recall | gap vs `teacher_16384` <= `0.010` |
| MRR | gap vs `teacher_16384` <= `0.015` |
| NDCG/MAP | gap vs `teacher_16384` <= `0.025` |
| Cost | candidate docs, postings, rerank terms, and payload MB must not materially exceed `EATMH002/doc128` guardrails |
| Robustness | no full15 dataset collapse; pass LODO or dataset-family holdout |

Any M28 model below current `baseline_budget16` on NDCG@10 or MAP@100 does not
proceed to another training round.

## T3: Concept Vocabulary Reopen As True Concept Encoder

Goal: test concept vocabulary as a real `text encoder -> SAE concept atoms`
route, not as another off-the-shelf SPLADE rerun.

Required comparison set:

- BM25;
- `teacher_16384`;
- current `baseline_budget16` text-student;
- new concept encoder.

Required cost evidence:

- QD-FLOPs-style cost;
- posting reads;
- candidate docs;
- rerank terms;
- payload MB;
- mean/p95 latency where a runtime path exists.

Stop condition:

```text
If concept encoder NDCG/MAP does not beat current text-student, or if
fanout/payload materially worsens, keep concept vocabulary parked.
```

## T4: SoftSAE Only As Cost Diagnostic

M27 showed adaptive selection has a signal but not enough standalone value.
SoftSAE remains useful as a diagnostic feature source, not as an independent
optimization line.

Keep:

- coverage slope;
- activation entropy;
- top atom mass;
- predicted fanout.

Do not run standalone selector optimization again unless T2 or T3 creates a
new high-quality model with excessive fanout. In that case, SoftSAE-style
signals can be reintroduced as a cost-control layer.

## Test Plan

Every promoted M28 experiment must report both quality and physical cost:

- Full15 quality matrix: Recall@20, Recall@100, MRR@20, NDCG@10, MAP@100.
- Full15 physical matrix: candidate docs, SAE postings, BM25 postings, rerank
  terms, payload MB, mean/p95 latency.
- Parity: Python/C/PostgreSQL top-k parity, score tolerance, and tie tolerance.
- Generalization: LODO or dataset-family holdout.
- Real corpus: arxiv/pubmed/policy efficiency only unless qrels/proxy-qrels
  exist.

## Execution Order

1. Re-run or stabilize T1 only as a harness validation task.
2. Design one T2 model-reset experiment that changes a core factor.
3. Run T3 concept encoder only if its representation can be evaluated in the
   same full15/cost harness.
4. Use T4 only for diagnostic reporting unless a new high-quality model needs
   fanout control.
5. Use `sae-m28-post-m27-results-report.md` as the first-pass decision
   checkpoint before deciding whether to continue model research.

## Final Decision Rule

If T2 and T3 both fail to beat the current text-student frontier, M28 should
record:

```text
stop direct dense-removal product line
keep teacher-path read-only research prototype
```

If either T2 or T3 passes the inherited M27 gate, read-only engineering can be
reopened around that model lineage, still without freezing public SQL/API until
runtime parity and product workload evidence are complete.
