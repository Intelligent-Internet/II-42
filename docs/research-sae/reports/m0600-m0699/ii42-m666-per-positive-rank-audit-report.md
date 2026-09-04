# M666 / Per-Positive Rank Audit

Status: `per_positive_rank_audit_complete`

M666 exports exact positive-document ranks for dense root, M549,
M658 baseline, and M661 compiler on the M659-M664 broad smoke surface.

## Setup

- Shared root: `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`
- Tasks: `FiQA2018,SCIDOCS,TRECCOVID,ArguAna`
- Splits: `dev,test`
- TopK / CUB K: `100` / `1000`

## Summary

| Scope | Positives | Top100 delta | CUB delta | Avg rank delta | Class counts |
| --- | ---: | ---: | ---: | ---: | --- |
| `macro` | 12455 | +2 | -1 | +0.49 | `{"candidate_miss_both": 5694, "candidate_upper_bound_gain": 1, "candidate_upper_bound_loss": 2, "present_below_top100": 4272, "promoted_to_top100": 2, "stable_top100": 2484}` |
| `dev` | 6202 | +1 | -1 | +0.62 | `{"candidate_miss_both": 2965, "candidate_upper_bound_loss": 1, "present_below_top100": 2110, "promoted_to_top100": 1, "stable_top100": 1125}` |
| `test` | 6253 | +1 | +0 | +0.36 | `{"candidate_miss_both": 2729, "candidate_upper_bound_gain": 1, "candidate_upper_bound_loss": 1, "present_below_top100": 2162, "promoted_to_top100": 1, "stable_top100": 1359}` |
| `ArguAna:dev` | 96 | +0 | +0 | +0.00 | `{"stable_top100": 96}` |
| `ArguAna:test` | 128 | +0 | +0 | +0.00 | `{"stable_top100": 128}` |
| `FiQA2018:dev` | 250 | +0 | +0 | +0.05 | `{"candidate_miss_both": 8, "present_below_top100": 38, "stable_top100": 204}` |
| `FiQA2018:test` | 338 | +0 | +0 | -0.05 | `{"candidate_miss_both": 17, "present_below_top100": 46, "stable_top100": 275}` |
| `SCIDOCS:dev` | 471 | +0 | +0 | -0.07 | `{"candidate_miss_both": 80, "present_below_top100": 137, "stable_top100": 254}` |
| `SCIDOCS:test` | 632 | +1 | +0 | -0.13 | `{"candidate_miss_both": 163, "present_below_top100": 176, "promoted_to_top100": 1, "stable_top100": 292}` |
| `TRECCOVID:dev` | 5385 | +1 | -1 | +0.72 | `{"candidate_miss_both": 2877, "candidate_upper_bound_loss": 1, "present_below_top100": 1935, "promoted_to_top100": 1, "stable_top100": 571}` |
| `TRECCOVID:test` | 5155 | +0 | +0 | +0.46 | `{"candidate_miss_both": 2549, "candidate_upper_bound_gain": 1, "candidate_upper_bound_loss": 1, "present_below_top100": 1940, "stable_top100": 664}` |

## Diagnosis

Observed result: M661 has `Top100 delta = +2` but `CUB delta = -1`
across 12,455 positive query-document pairs.  It also leaves 4,272
positives present below top100 and 5,694 positives outside top1000 in
both M658 and M661.  This confirms M661 is a near-boundary ranking
movement, not a candidate-generation shape breakthrough.

The top100 delta measures ranking recovery. The CUB delta measures
candidate-generation shape. A positive top100 delta with zero CUB delta
means M661 is mostly moving already-present positives across rank 100.

## Decision Rule

If CUB delta is near zero while top100 delta is positive, the next
useful phase is a scorer/boundary-ranker. If CUB delta is positive and
stable by task, the next useful phase is first-stage candidate-shape
training with CUB + Recall + dense-overlap gates.

For this run, the rule selects scorer/boundary-ranker first for the
present-below-top100 pool.  First-stage CUB training should be resumed
only with an explicit objective for the miss-both pool, because the
M661 missing-positive objective did not expand CUB.

## Artifacts

- JSON: `runs/ii42-m666-per-positive-rank-audit-v1/m666_per_positive_rank_audit_seed6661/m666_per_positive_rank_audit_seed6661.json`
- Positive rows: `runs/ii42-m666-per-positive-rank-audit-v1/m666_per_positive_rank_audit_seed6661/m666_per_positive_rank_audit_seed6661_positive_rows.jsonl`

