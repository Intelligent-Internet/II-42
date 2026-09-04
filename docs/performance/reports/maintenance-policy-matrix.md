# Maintenance Policy Matrix

Date: 2026-03-24

## Scope

This report expands the earlier point policy sweep into a broader local
matrix across several churn shapes.

Raw data is stored in the corresponding diagnostics JSON.

Policies compared:

- `eager`
  - `auto_rebuild_threshold = 0`
- `count_only`
  - `auto_rebuild_threshold = 1000`
- `bytes_40000`
  - `auto_rebuild_threshold = 1000`
  - `auto_rebuild_delta_bytes = 40000`
- `tuned_current`
  - `auto_rebuild_threshold = 1000`
  - `auto_rebuild_delta_bytes = 50000`
  - `auto_rebuild_churn_ratio = 0.05`
- `tuned_relaxed`
  - `auto_rebuild_threshold = 1000`
  - `auto_rebuild_delta_bytes = 50000`
  - `auto_rebuild_churn_ratio = 0.09`

## Scenarios

The matrix covers:

- `small_mixed`
  - 5,000 initial docs
  - 50 insert / 50 update / 50 delete per cycle
- `heavy_mixed`
  - 10,000 initial docs
  - 100 insert / 100 update / 100 delete per cycle
- `heavy_insert_skew`
  - 10,000 initial docs
  - 150 insert / 50 update / 50 delete per cycle
- `heavy_update_skew`
  - 10,000 initial docs
  - 50 insert / 150 update / 50 delete per cycle
- `heavy_delete_skew`
  - 10,000 initial docs
  - 50 insert / 50 update / 150 delete per cycle

## Winners

Overall best policy by median QPS:

- `small_mixed`: `tuned_current`
- `heavy_mixed`: `tuned_current`
- `heavy_update_skew`: `tuned_current`
- `heavy_insert_skew`: `tuned_relaxed`
- `heavy_delete_skew`: `eager`

Best deferred policy by median QPS:

- `small_mixed`: `tuned_current`
- `heavy_mixed`: `tuned_current`
- `heavy_update_skew`: `tuned_current`
- `heavy_insert_skew`: `tuned_relaxed`
- `heavy_delete_skew`: `tuned_current`

Win counts:

- overall best policy wins:
  - `tuned_current`: `3`
  - `tuned_relaxed`: `1`
  - `eager`: `1`
- deferred-only best policy wins:
  - `tuned_current`: `4`
  - `tuned_relaxed`: `1`

## Main Finding

The earlier single heavy checkpoint that favored `bytes_40000` does not
hold up as the general recommendation once the local benchmark is
expanded into a broader matrix.

The current local guidance is now:

- `query_first`
  - still maps to `eager`
- default deferred recommendation
  - now maps back to `tuned_current`
- insert-skewed heavy churn
  - is the one shape where `tuned_relaxed` looks better
- delete-heavy churn
  - still strongly favors `eager` if query throughput is the only goal

## Practical Guidance

At the moment the most defensible recommendation is:

- keep `eager` for strict query-first workloads
- treat `tuned_current` as the default deferred preset
- treat `tuned_relaxed` as a candidate only when the workload is known
  to be strongly insert-skewed
- do not currently treat `bytes_40000` as the default heavy policy

This still does not justify another storage rewrite. It just refines the
consolidation guidance on top of the current maintenance design.
