# SAE Milestone 24 16384 Teacher Distillation Report

## Scope

This milestone tests whether the stronger 12288/16384 SAE teachers can produce a better direct text-to-atoms student. It intentionally does not add SQL/API productization or mutable index work.

The planned MPS path was attempted first, but the 16384 output head stalled in Metal tensor copy/sync and saturated swap before the first epoch completed. CPU dense support was also too memory-heavy for a complete full15 run. The final reproducible runs therefore use CPU sampled support loss and explicitly small support/retrieval/coverage batches with the same teacher, retrieval, and candidate-budget objectives.

## Reference Rows

| Reference | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Active8 Recall@100 | Candidates | SAE Posts | Payload MB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline_budget16` | `bm25_student_atoms` | 0.8324 | 0.8291 | 0.7225 | 0.6913 | 0.8286 | 740.9 | 787.4 | 6.64 |
| `focus6` | `bm25_student_atoms_w0p25` | 0.7865 | 0.7931 | 0.6734 | 0.6299 | 0.7761 | 768.1 | 809.6 | 9.26 |
| `teacher_16384` | `bm25_sae` | 0.8455 | 0.8462 | 0.7530 | 0.7273 | 0.8374 | 491.1 | 481.3 | 7.00 |

## Distillation Runs

| Run | Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Delta Recall vs baseline | Delta NDCG | Delta MAP | Recall Gap To 16384 Teacher | Active8 Recall@100 | Candidates | SAE Posts | Payload MB | Decision |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `m24-16384-budget16-w0p04-token-lse` | `bm25_student_atoms_w0p25` | 0.7829 | 0.7838 | 0.6656 | 0.6216 | -0.0496 | -0.0570 | -0.0696 | 0.0627 | 0.7679 | 393.3 | 235.3 | 9.41 | `cost-only` |
| `m24-12288-budget16-w0p04-token-lse` | `bm25_student_atoms_w0p25` | 0.7828 | 0.7846 | 0.6638 | 0.6203 | -0.0496 | -0.0587 | -0.0709 | 0.0663 | 0.7720 | 416.6 | 261.6 | 9.41 | `cost-only` |
| `m24-16384-budget16-w0p04-token-lse-h384` | `bm25_student_atoms_w0p25` | 0.7844 | 0.7860 | 0.6644 | 0.6219 | -0.0480 | -0.0582 | -0.0694 | 0.0611 | 0.7742 | 393.9 | 235.6 | 9.56 | `cost-only` |

## Decision

- Best M24 row by ranking quality: `m24-16384-budget16-w0p04-token-lse`.
- Promoted rows: `none`.
- Strong-pass rows: `none`.
- Decision: M24 does not promote the larger-teacher student line. Freeze 12288/16384 text-student loss tuning and move to read-only engineering with the teacher/read-only payload path.

## Retry Gate

- h384 retry required by primary loss slope: `True`.
- h384 retry executed: `True`.

## Artifacts

- Stable report root: `/Volumes/Betty/Tmp/ii42_sae_reports/m24-16384-distillation`
- Tracked report: `sae-milestone24-16384-distillation-report.md`
- Data root: `/Volumes/Betty/Tmp/ii42_sae_beir15_shared`
