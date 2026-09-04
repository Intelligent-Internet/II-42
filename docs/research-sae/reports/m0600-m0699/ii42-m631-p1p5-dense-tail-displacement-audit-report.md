# M631 / P1.5 Dense-Tail Displacement Audit Report

Status: M631-A completed on smoke surface.

## Objective

M631-A tests whether the current P1 posting feature surface contains a
recoverable first-stage signal for documents that dense teacher ranks inside
top100 but P1 leaves in ranks 101..1000.

This audit is intentionally BM25-free and qrels-free for training.  Dense rank
is used only to define audit labels:

- Dense-tail positive: dense top100 and P1 rank 101..1000.
- Displaced head negative: not dense top100 and P1 rank <= 100.

## Surface

Input:

- `runs/m630_p1p4_score_geometry_audit_smoke_v1/m630_p1p4_score_geometry_dataset.jsonl`

Generated artifacts:

| Artifact | Path |
| --- | --- |
| Audit JSONL | `runs/m631_p1p5_dense_tail_probe_smoke_v1/m631_dense_tail_audit.jsonl` |
| Audit JSON | `runs/m631_p1p5_dense_tail_probe_smoke_v1/m631_dense_tail_audit.json` |
| Audit MD | `runs/m631_p1p5_dense_tail_probe_smoke_v1/m631_dense_tail_audit.md` |

Datasets:

- `nfcorpus`
- `scifact`
- `trec-covid`

## Contrast Matrix

| Dataset | Queries | Dense-tail positives | Displaced head negatives | Rows |
| --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | 100 | 731 | 731 | 1462 |
| `scifact` | 100 | 695 | 695 | 1390 |
| `trec-covid` | 50 | 284 | 284 | 568 |
| `macro` | 250 | 1710 | 1710 | 3420 |

## Interpretation

The audit confirms that the smoke surface has a real dense-tail displacement
population.  M630 had already shown dense top100 is fully present in P1 top1000
on this surface; M631-A makes the displacement set explicit and balanced for
feature-channel tests.

This audit by itself does not prove a usable ranking signal.  It only proves
there are enough tail/head contrast rows to run M631-B separability and
top100-swap probes.

## Decision

M631-A is retained as a useful diagnostic dataset.  The next required gate is
M631-B: at least one feature family must show non-zero Recall@100 improvement
under conservative top100 swap simulation before M631-C can train a global
first-stage scorer.
