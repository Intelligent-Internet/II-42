# SAE M102 Runtime Scorer Candidate Path Report

Date: 2026-05-22

## Decision

M102 wires the exported M101 Stage-B ranker into a minimal runtime scoring
path and evaluates candidate-budget behavior over the existing M81 candidate
surface. The exported scorer works cleanly as a reusable runtime component,
but the sweep also shows that the current M81 surface is too small to prove
full-corpus candidate control: it averages only `118.7` candidates per query
with a maximum of `128`.

The key result is therefore two-sided:

- The scorer is viable outside the training loop and keeps the M101 quality
  profile when it receives enough candidates.
- Candidate-budget evidence remains only a surface approximation. The next
  blocker is postings-driven full-corpus candidate generation, not another
  Stage-B ranker change.

## Inputs

- Score cache:
  `/home/huoju/leask/runs/m98-stage-b-ranking-calibration-v0/m98_candidate_scores.pt`
- Exported scorer:
  `/home/huoju/leask/runs/m101-batched-stage-b-ranker-v0/m101_stage_b_ranker_export.json`
- Spark output:
  `/home/huoju/leask/runs/m102-runtime-scorer-candidate-path-v0`
- Local artifact mirror:
  `results/sae/m102-runtime-scorer-candidate-path`

## Data

| Field | Value |
| --- | ---: |
| Rows | `4806` |
| Train rows | `3920` |
| Validation rows | `486` |
| Holdout rows | `400` |
| Eval rows | `886` |
| Query feature dim | `11` |
| Candidate feature dim | `27` |
| Sweep elapsed | `32.55s` |
| Full surface mean candidates | `118.7` |
| Full surface p95 candidates | `128.0` |

## Full-Surface Matrix

| Run | Recall@10 | MRR | NDCG@10 | Mean Candidates |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.3178 | 0.5960 | 0.4505 | 118.7 |
| `bm25_dense_w2` | 0.3530 | 0.6631 | 0.5137 | 118.7 |
| `bm25_sae_w2` | 0.3564 | 0.6649 | 0.5177 | 118.7 |
| `m101_full_surface` | 0.4036 | 0.6930 | 0.5602 | 118.7 |

## Candidate-Budget Findings

The strongest low-cost point is not a replacement for M101, but it is useful
as a latency profile:

| Variant | Recall@10 | MRR | NDCG@10 | Mean Cand | P95 Cand | Positive Coverage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `bm255_sae10` | 0.3617 | 0.6670 | 0.5191 | 12.8 | 15.0 | 0.3943 |
| `bm255_sae40` | 0.3613 | 0.6686 | 0.5200 | 41.8 | 45.0 | 0.4978 |
| `bm255_sae80` | 0.3930 | 0.6792 | 0.5460 | 80.6 | 83.0 | 0.7611 |
| `bm255_sae120` | 0.4049 | 0.6908 | 0.5590 | 114.7 | 121.0 | 0.9761 |
| `m101_full_surface` | 0.4036 | 0.6930 | 0.5602 | 118.7 | 128.0 | 1.0000 |

Interpretation:

- `bm255_sae10` uses about `10.8%` of the full surface and still beats fixed
  full-surface `bm25_sae_w2` on NDCG@10 (`0.5191` vs `0.5177`), but it is far
  below M101 full-surface ranking quality.
- `bm255_sae80` is a middle profile: about `67.9%` of candidates, most of the
  recall gain, but still `-0.0142` NDCG behind M101 full surface.
- `bm255_sae120` is effectively a high-quality profile, but it only reduces
  mean candidates from `118.7` to `114.7`. That is not a meaningful runtime
  candidate-control win.

## Engineering Notes

The script normalizes candidate features with the exported M101 metadata, not
with per-run sweep statistics. This keeps the scorer contract aligned with the
exported runtime artifact.

The evaluation uses full candidate-surface labels as the denominator for
Recall@10 and NDCG@10. This is important because otherwise a smaller candidate
pool could look artificially better after dropping positives.

Query-level distribution features still come from the M98 candidate surface in
this approximation. A production-like full-corpus path must recompute or
replace these features from the generated candidate pool and postings
diagnostics.

## Decision

M102 promotes the exported M101 scorer as the runtime scoring component for
the next read-only evaluation step. It does not promote the current candidate
surface budget as a product path.

Next work should be:

1. Build a postings-driven full-corpus candidate generator for BM25 atoms and
   SAE atoms.
2. Recompute runtime-safe query/candidate features from that generated pool.
3. Run the same M101/M102 scorer matrix with real candidate-doc, posting-read,
   and rerank-term costs.
4. Only after that, decide whether the Stage-B scorer is ready for the
   PostgreSQL read-only payload path.

