# Maintenance Policy Sweep

Date: 2026-03-24

## Scope

This report compares several deferred-maintenance policy presets on the
same local PostgreSQL instance and the same mainline maintenance
build.

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

## Small Scenario

Workload:

- 5,000 initial documents
- 6 churn cycles
- per cycle:
  - 50 inserts
  - 50 updates
  - 50 deletes
  - one `VACUUM`
- 100 measured queries after each cycle

| Policy | Median QPS | Avg commit ms | Avg vacuum ms | Final rebuilds | Final state |
| --- | ---: | ---: | ---: | ---: | --- |
| `eager` | 17252.67 | 16.61 | 22.93 | 13 | exact |
| `count_only` | 13571.21 | 2.67 | 9.30 | 2 | deferred |
| `bytes_40000` | 12115.78 | 4.98 | 5.69 | 2 | deferred |
| `tuned_current` | 14181.15 | 13.27 | 5.89 | 4 | exact |
| `tuned_relaxed` | 11750.42 | 5.66 | 5.10 | 3 | exact |

Best median QPS:

- `eager`

Interpretation:

- On the smaller local scenario, exact eager rebuilds are still the
  strongest query-first policy.
- Among deferred policies, `tuned_current` remains the best query-side
  compromise, but it does not beat eager rebuilds.

## Heavy Scenario

Workload:

- 10,000 initial documents
- 8 churn cycles
- per cycle:
  - 100 inserts
  - 100 updates
  - 100 deletes
  - one `VACUUM`
- 150 measured queries after each cycle

| Policy | Median QPS | Avg commit ms | Avg vacuum ms | Final rebuilds | Final state |
| --- | ---: | ---: | ---: | ---: | --- |
| `eager` | 10196.48 | 21.10 | 31.75 | 17 | exact |
| `count_only` | 10979.13 | 9.58 | 8.85 | 3 | deferred |
| `bytes_40000` | 12547.13 | 14.30 | 7.25 | 5 | exact |
| `tuned_current` | 9893.02 | 17.60 | 6.78 | 5 | exact |
| `tuned_relaxed` | 9305.14 | 10.50 | 7.57 | 3 | deferred |

Best median QPS:

- `bytes_40000`

Interpretation:

- On the heavier local scenario, the earlier churn-ratio-driven tuned
  policy is no longer the strongest query-side choice.
- The best local median QPS came from a simpler bytes-bounded policy:
  `auto_rebuild_threshold = 1000` plus
  `auto_rebuild_delta_bytes = 40000`.
- `count_only` stayed cheaper on writes, but finished in a deferred
  overlay state and did not match the query throughput of `bytes_40000`.

## Current Recommendation

The current benchmark-backed guidance is:

- if query throughput is the only priority, keep eager exact rebuilds
- if the workload is heavy mixed churn and deferred maintenance is
  needed, start with:
  - `auto_rebuild_threshold = 1000`
  - `auto_rebuild_delta_bytes = 40000`
- do not currently treat `auto_rebuild_churn_ratio = 0.05` as the
  default best tuned policy

This is still local guidance, not a universal rule. The main value of
the sweep is that it gives a repeatable way to tune consolidation
without touching the storage engine or the query hot path.
