# Maintenance Production-Shaped Trace Profiles

Date: 2026-03-24

## Scope

This report extends the maintenance-policy work beyond fixed
insert/update/delete ratios. The goal is to test the current policy
family against write shapes that look more like production behavior:
micro-batches, bursty ingest, hotset mutation, and retention cleanup.

Raw data is stored in the corresponding diagnostics JSON.

Compared policies:

- `eager`
- `count_only`
- `bytes_40000`
- `tuned_current`
- `tuned_relaxed`

Run shape:

- `repeat_count = 3`
- unique benchmark DB names per repeat
- the same canonical query path as the rest of the maintenance
  benchmark work

## Trace Profiles

The profiles cover:

- `read_heavy_microbatch`
  - 12 cycles
  - small frequent writes
  - recent-doc hot updates
  - oldest-doc cleanup deletes
- `bursty_ingest`
  - 12 cycles
  - alternating ingest burst / settle / cleanup waves
- `hotset_mutation`
  - 10 cycles
  - heavy updates concentrated on a recent hotset
- `retention_cleanup`
  - 10 cycles
  - repeated delete-heavy cleanup followed by insert-heavy recovery

## Main Finding

These production-shaped traces do not justify a new global deferred
recommendation profile.

What they do show is:

- `eager` remained the overall winner in `3/4` profiles
- deferred winners split by trace shape
- the current threshold family still looks healthy
- but there is still no single deferred preset that dominates across
  all realistic write patterns

That means the current recommendation surface should stay conservative.
The trace work strengthens the existing policy guidance rather than
replacing it.

## Winners

Overall winner by profile:

- `read_heavy_microbatch`: `eager` in `3/3`
- `bursty_ingest`: `eager` in `3/3`
- `hotset_mutation`: `eager` in `2/3`, `bytes_40000` in `1/3`
- `retention_cleanup`: `eager` in `2/3`, `tuned_current` in `1/3`

Best deferred winner by profile:

- `read_heavy_microbatch`
  - split evenly:
    - `bytes_40000` in `1/3`
    - `tuned_current` in `1/3`
    - `tuned_relaxed` in `1/3`
- `bursty_ingest`
  - `bytes_40000` in `2/3`
  - `tuned_relaxed` in `1/3`
- `hotset_mutation`
  - `tuned_relaxed` in `2/3`
  - `bytes_40000` in `1/3`
- `retention_cleanup`
  - `tuned_current` in `2/3`
  - `bytes_40000` in `1/3`

## Practical Impact

The recommendation helper should not add another broad profile based on
these traces alone.

Current practical read:

- keep `query_first` mapped to `eager`
- keep `balanced` on the broader matrix-backed `tuned_current`
- keep `heavy_insert_skew` and `longrun_mixed_churn` as the only extra
  workload-specific profiles currently justified
- treat the trace results as proof that deferred policy choice remains
  workload-dependent even after the broader matrix and repeatability work

If a future recommendation change happens, it should be driven by:

- more repeats on one specific trace family, or
- production-captured workload replay,

not by a single new synthetic point benchmark.
