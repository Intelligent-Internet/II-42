# M632 / P1.6 Failure Decomposition Report

Status: M632-A completed on smoke surface.

## Objective

M632-A separates the current first-stage failure into candidate availability,
dense-tail under-ranking, and false top100 head documents.  This is the gate
before training a deeper output head.

The input remains the M630 smoke audit dataset:

- `runs/m630_p1p4_score_geometry_audit_smoke_v1/m630_p1p4_score_geometry_dataset.jsonl`

Generated artifacts:

| Artifact | Path |
| --- | --- |
| JSON | `runs/m632_p1p6_output_head_smoke_v1/m632_failure_decomposition.json` |
| MD | `runs/m632_p1p6_output_head_smoke_v1/m632_failure_decomposition.md` |

## Decomposition

| Dataset | Queries | Candidate miss rate | Dense tail share | Dense top100 kept | False head/query |
| --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 100 | 0.293871 | 0.073100 | 0.926900 | 7.31 |
| `scifact` | 100 | 0.000000 | 0.069500 | 0.930500 | 6.95 |
| `trec-covid` | 50 | 0.328456 | 0.056800 | 0.943200 | 5.68 |
| `macro` | 250 | 0.322508 | 0.068400 | 0.931600 | 6.84 |

## Interpretation

The dense teacher support is not missing from the P1 top1000 candidate pool on
this smoke surface.  About `6.84%` of dense top100 support is available in P1
top1000 but displaced below P1 top100, and each query has about `6.84` false
head documents in the P1 top100 against the dense teacher.

There is also qrels candidate miss, especially on `nfcorpus` and `trec-covid`,
but that is not the immediate dense-tail output-head target.  M632 remains
valid as a first-stage dense-faithfulness probe because the dense-tail
under-ranking population is real and large enough for training.

## Decision

Proceed to M632-B/C smoke training, but require top100 guarded promotion.
Candidate availability is not enough to justify shared15 expansion; the model
must improve `Recall@100` without damaging `NDCG@10` or `MRR@20`.
