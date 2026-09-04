# SAE M27 Exploration Closure Report

## Scope

This report executes the M27 closure harness. It is still model-side research; it does not freeze SQL/API, mutable index behavior, or a dense-removal product claim.

## Text-To-Atoms Closure

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Gate |
| --- | ---: | ---: | ---: | ---: | --- |
| `teacher_16384` | 0.8455 | 0.8462 | 0.7530 | 0.7273 | `reference` |
| `baseline_budget16` | 0.8324 | 0.8291 | 0.7225 | 0.6913 | `fail` |
| `best_existing_student` | 0.8324 | 0.8291 | 0.7225 | 0.6913 | `fail` |

- scanned full15 student rows: `290`
- best source: `bm25_student_atoms`
- best path: `/Users/leask/Documents/II/ii42_sae/results/sae/text-atoms/full15-shared-dense-budget16-w0p04-token-char/text_atom_training_matrix.json`
- decision: `open-final-push-failed`

## SoftSAE Learned Selector Closure

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Postings | Candidates |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `low` | 0.8197 | 0.8289 | 0.7352 | 0.6909 | 667.9452 | 523.8214 |
| `high` | 0.8334 | 0.8387 | 0.7467 | 0.7110 | 985.1131 | 721.2118 |
| `global_selected` | 0.8318 | 0.8384 | 0.7465 | 0.7102 | 952.9311 | 700.5406 |

- global selector rule: `coverage_slope>=7.25`
- postings drop vs high: `0.0327`
- decision: `parked`

## Concept Vocabulary Closure

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `teacher` | 0.8425 | 0.8458 | 0.7507 | 0.7249 |
| `current_student` | 0.8324 | 0.8291 | 0.7225 | 0.6913 |
| `bm25_sae_splade` | 0.7899 | 0.6814 | 0.6032 | 0.5043 |

- decision: `parked`
- reason: SPLADE/concept control does not beat current student on ranking

## Final Exit Decision

`dense-removal not ready`

The exit decision is intentionally conservative. A query-time dense dependency cannot be removed until the direct text-to-atoms path passes the ranking-quality gate.

## Artifacts

- JSON: `/Users/leask/Documents/II/ii42_sae/results/sae/m27/exploration-closure/m27_exploration_closure.json`
- Stable root: `/Volumes/Betty/Tmp/ii42_sae_reports/m27-exploration-closure`
