# Maintenance Long-Run Mixed-Churn Benchmark

Date: 2026-03-24

## Scope

This report extends the local maintenance-policy work with a heavier
mixed-churn checkpoint that runs longer than the earlier point sweeps
and policy matrix.

Raw data is stored in the corresponding diagnostics JSON.

Run shape:

- `doc_count = 15000`
- `query_count = 150`
- `cycle_count = 12`
- `insert_per_cycle = 150`
- `update_per_cycle = 150`
- `delete_per_cycle = 150`
- `repeat_count = 3`
- the same five maintenance presets used in the local policy matrix

## Main Finding

For this heavier long-run mixed workload:

- `eager` remained the strongest pure query-first policy
- among deferred policies, `tuned_relaxed` won `2/3` repeats
- `bytes_40000` won the remaining deferred repeat
- `tuned_current` was no longer the strongest deferred preset

This does not overturn the broader matrix result that keeps
`tuned_current` as the default deferred recommendation. It does justify
an additional explicit profile for users whose workload is closer to
this heavier long-run mixed pattern.

## Aggregate Snapshot

- `eager`
  - average median QPS: `10442.34`
  - range across repeats: `9473.51` to `11479.19`
  - average commit time: `32.88 ms`
- `tuned_relaxed`
  - average median QPS: `10071.30`
  - range across repeats: `8457.73` to `10889.76`
  - average commit time: `24.91 ms`
- `bytes_40000`
  - average median QPS: `10019.34`
  - range across repeats: `8305.05` to `11377.96`
  - average commit time: `25.10 ms`
- `tuned_current`
  - average median QPS: `8710.41`
  - range across repeats: `7565.47` to `10443.75`
  - average commit time: `23.67 ms`
- `count_only`
  - average median QPS: `8787.37`
  - range across repeats: `7172.44` to `9683.33`
  - average commit time: `8.54 ms`

## Practical Impact

The recommendation helper now exposes a dedicated
`longrun_mixed_churn` profile:

- recommended reloptions:
  - `auto_rebuild_threshold = 1000`
  - `auto_rebuild_delta_bytes = 50000`
  - `auto_rebuild_churn_ratio = 0.09`
- confidence:
  - `medium`

This keeps the previous guidance intact:

- `query_first` still maps to eager exact rebuilds
- `balanced` still stays on the broader-matrix default
- `heavy_mixed_churn` remains intentionally conservative

The new profile exists because the longer mixed-churn run produced a
useful workload-specific deviation without justifying a global default
change.
