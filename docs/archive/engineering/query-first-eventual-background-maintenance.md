# Query-First Eventual Background Maintenance Report

> Historical record. The standalone query-triggered smoke and benchmark
> entrypoints described below were retired after linked-L0, page-native reads,
> and the shared maintenance service became the canonical v3 product path.
> Current validation is owned by `scripts/run_product_maturity_suite.py`.

## Archive Guide

The lasting design decision is query-first serving with asynchronous
convergence, not the particular query-triggered worker or maintenance GUCs
described in this milestone. Preserve the [functional validation](#functional-validation),
[backfill benchmark](#backfill-benchmark), and
[clean retrieval benchmark](#clean-retrieval-benchmark) as historical evidence;
they are not current-release performance claims.

The implemented successor is documented in
[Convergent Segmented Index](../../convergent-segmented-index.md) and
[Maintenance lifecycle](../../maintenance-lifecycle.md). Current compatible
accelerator baselines can outlive delta indefinitely; periodic maintenance is
not a serving-artifact expiry policy. See the [archive index](README.md) for
related closed records.

## Historical Milestone

This report records the local validation for the self-triggered background
maintenance milestone on the `codex/query-first-eventual-maintenance` branch.

## Scope

The milestone adds an opt-in no-pg_cron convergence path for indexes configured
with:

```sql
WITH (
    consistency = 'eventual'
);
```

Default exact indexes and `consistency = 'manual'` benchmark indexes are not
intended to change behavior.

## Implementation Summary

- committed write, UPDATE, and VACUUM debt can wake a dynamic background worker;
- later BM25 queries can also wake a worker when committed pending debt exists;
- the worker scans only opted-in query-first eventual indexes;
- each worker uses the existing non-blocking staged maintenance path;
- duplicate launches are tolerated by short-lock `try_maintain` semantics;
- skipped unchanged-key update debt is batched until commit, avoiding one
  metapage/WAL update per row during non-indexed-column backfills.

## Functional Validation

Local PostgreSQL 18 validation:

- `make -j8`: passed
- `make install`: passed
- `make installcheck`: passed
- `scripts/test_query_first_eventual_background_smoke.py`: passed
- `scripts/test_query_first_eventual_cancel_smoke.py`: passed
- `scripts/test_query_first_eventual_tail_smoke.py`: passed when run without a
  competing maintenance smoke

The tail smoke produced `lock_busy` when run in parallel with the cancellation
smoke. That is expected scheduler contention for the test environment rather
than an index corruption or convergence failure.

## Backfill Benchmark

Command shape:

```bash
II42_BENCH_DSN='dbname=postgres user=leask' \
python3 scripts/benchmark_query_first_eventual_backfill.py \
    --doc-count 100000 \
    --update-count 20000 \
    --statement-timeout-ms 2000 \
    --query-sleep-ms 5 \
    --convergence-timeout-ms 120000
```

Final measured run:

| Mode | Writer ms | Query p50 ms | Query p95 ms | Query p99 ms | Timeouts | Errors | Background convergence |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| eager exact | 172.26 | 0.32 | 0.70 | 3.64 | 0 | 0 | n/a |
| query-first eventual | 120.89 | 0.27 | 0.93 | 2.11 | 0 | 0 | 257.80 ms |

The query-first eventual run finished with:

```text
ii42_maintenance_state(rebuilds=2, pending_writes=0,
pending_deletes=0, delta_records=0, delta_bytes=0, stale=false)
```

Before the transaction-level batching fix, the same 100k/20k shape showed
writer time around `1983 ms` because every skipped unchanged-key update wrote
the metapage. After batching, writer time dropped to `121 ms` and the index
still converged automatically.

## Clean Retrieval Benchmark

The clean benchmark used `trec-covid` with:

- `ii42_ids`
- `ii42_text`
- `consistency = 'manual'`

The first three current runs, before the final write-path batching patch,
averaged within normal noise against the accepted
`/tmp/ii42_eventual_bench/online_v2_trec` baseline:

| Path | Build delta | Avg latency delta | QPS delta |
| --- | ---: | ---: | ---: |
| `ii42_ids` | +0.97% | -1.30% | +1.57% |
| `ii42_text` | -2.22% | -2.86% | +2.82% |

A fourth run after the final patch showed p95 outliers while Syncthing and
other desktop processes were active. The final patch only changes the
eventual-consistency unchanged-update write path, so that noisy clean-read run
is not treated as evidence of a stable read-path regression. A low-load
five-run repeat is still recommended before using clean benchmark numbers as
release evidence.

## Conclusion

The self-triggered background maintenance path is functionally complete for the
short-term design:

- foreground query behavior remains bounded and non-blocking under the
  query-first eventual policy;
- non-indexed-column backfills can avoid full delta payloads and avoid per-row
  metapage maintenance writes;
- committed debt converges automatically without pg_cron when write, VACUUM, or
  later query traffic reaches the database;
- explicit pg_cron or an external scheduler remains useful for time-based
  wakeups during completely idle periods.
