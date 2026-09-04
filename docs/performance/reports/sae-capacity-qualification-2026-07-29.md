# SAE Capacity Qualification

Date: 2026-07-29

This report qualifies the current model-backed index capacity work tracked as
`SAE-CAP-1` through `SAE-CAP-5`. It covers runtime concurrency, incremental
mutable-delta publication, the shared generation registry, transaction-local
snapshot views, and forward progress under shared-arena pressure.

The tests preserve the product architecture: one PostgreSQL index relation,
one durable unified delta, one public query API, and one shared model runtime.
No corpus scan, private persistent semantic overlay, second scheduler, or
deadline-based capacity workaround was introduced.

## Result

| Gate | Result | Qualification |
| --- | --- | --- |
| CAP-1 runtime parallelism | Accepted with a two-worker default | True postmaster worker-pool concurrency replaces the serial runtime. Four workers are an explicit measured option, not the default. |
| CAP-2 incremental publication | Accepted | One appended record remains tail-only at 1K, 10K, and 100K compiled prefixes. |
| CAP-3 shared registry | Accepted | Complete-key hash lookup and atomic leases remain stable at 65,536 slots and 64 clients. |
| CAP-4 private MVCC view | Accepted | Exact transaction-local reuse is bounded by records, bytes, and four retained entries. |
| CAP-5 pressure progress | Accepted | Aggregate multi-index pressure triggers early compaction; exact oversized identities fail fast through a bounded keyed registry. |

## CAP-1: Runtime Worker Pool

The runtime now starts a bounded pool of postmaster-managed inference workers.
Each worker owns its ONNX Runtime sessions. Document work may occupy at most
`worker_count - 1` healthy workers, preserving a physical execution lane for
online queries.

At 64 query clients:

| Workers | QPS | Request p95 | Aggregate worker RSS |
| ---: | ---: | ---: | ---: |
| 1 | 282.7259 | 207.7382 ms | 1,605,456 KiB |
| 2 | 438.2125 | 129.1982 ms | 3,216,384 KiB |
| 4 | 547.2927 | 98.0896 ms | 6,443,600 KiB |

Two workers provide `1.550x` single-worker throughput and are the accepted
default. Four workers provide `1.936x`; because this does not meet the original
`2x` promotion gate, four workers remain an explicitly sized deployment
choice. PostgreSQL cancellation and `statement_timeout` remain failure
boundaries, not capacity mechanisms.

See
[the detailed CAP-1 report](runtime-worker-pool-cap1-2026-07-29.md).

## CAP-2: Incremental Unified Delta

Five one-record appends were sampled at each durable prefix:

| Prefix | Median publish | Publish p95 | Median append lock | Oracle exact |
| ---: | ---: | ---: | ---: | --- |
| 1,000 | 70 us | 86 us | 1 us | yes |
| 10,000 | 49 us | 124 us | 1 us | yes |
| 100,000 | 58 us | 279 us | 1 us | yes |

The 100K/1K median publish ratio is `0.8286`; the append-lock ratio is `1.0`.
Every sample published one record and matched the full-overlay oracle.
Incremental chains consolidate at a bounded depth without changing durable
relation ownership.

The current-tree chain test advances through eight resident incremental
segments, with active bytes growing from `11,352` to `88,112`. The ninth
append consolidates the chain to one root entry (`88,536` bytes), retires the
leased parent segments, and the tenth append creates a two-entry chain
(`99,976` bytes). Update, delete, and VACUUM/tombstone paths remain oracle
exact.

A staged-package same-index race also covers the physical-page handoff between
an online compactor and a concurrent append. An older snapped record count may
end before the current physical page boundary only when a metapage reread
proves that the durable identity advanced. The same condition under a stable
identity remains corruption. Eight writers, eight readers, 288 CRUD
operations, maintenance, VACUUM, restart, and compaction all preserve
normal/oracle parity.

Raw evidence:
[CAP-2 1K/10K/100K](../data/raw/unified-delta-capacity-2026-07-29/cap2-cursor-sampled-1k-10k-100k.json),
[CAP-2 bounded chain](../data/raw/unified-delta-capacity-2026-07-29/cap2-incremental-chain-v2.json),
and
[same-index concurrency](../data/raw/unified-delta-capacity-2026-07-29/same-index-concurrency-v1.json).

## CAP-3: Shared Generation Registry

The accepted run uses three trials of 1,000 queries per client at 1,024,
10,000, and 65,536 registry slots.

| Measure | Result | Gate |
| --- | ---: | ---: |
| 65K/1K single-client p95 ratio | 1.0855 | <= 2.0 |
| 65K 64-client throughput scale over one client | 1.2702 | >= 1.25 |
| 65K/1K 64-client QPS ratio | 0.9819 | >= 0.8 |

The lifecycle probe also rejected an invalid startup fill, evicted nine
relations during churn, cleared 1,024 entries, reloaded one exact entry, and
preserved the query fingerprint before and after clear/reload.

Raw evidence:
[CAP-3 qualified matrix](../data/raw/shared-generation-registry-2026-07-29/cap3-registry-qualified-current-tree-v2.json).

## CAP-4: Transaction-Local Exact Views

At 1K, 10K, and 100K durable records, each unchanged identity performed one
build followed by 99 cache hits. Intervening local DML produced exactly one
new build followed by 99 hits. All normal results matched the MVCC oracle.

The 100K retained view used about 344.6 MB; 100 repeated queries grew backend
memory by only 8 KiB. Commit released every retained entry and byte. A
record-limit overflow returned SQLSTATE `54000` and retained no private state.

Raw evidence:
[CAP-4 1K/10K/100K](../data/raw/local-delta-cache-capacity-2026-07-29/cap4-1k-10k-100k-v2.json).

## CAP-5: Aggregate Pressure And Forward Progress

The final test used two semantic indexes in a 1 MiB shared arena:

- the current index estimated `713,472` additional bytes, below the `786,432`
  byte high watermark;
- resident parent segments held `435,064` active bytes;
- aggregate projected use reached `1,148,536` bytes and therefore entered
  `compact_due` before an admission failure;
- both indexes remained full-overlay-oracle exact through compaction;
- an exact compiled payload of `1,136,640` bytes exceeded the arena and
  returned SQLSTATE `54000` in 0.129 seconds;
- two alternating oversized indexes occupied two keyed registry entries;
- subsequent lookups failed fast in 0.006 and 0.019 seconds instead of waiting
  for the configured 30-second cache timeout;
- incremental compaction converged both indexes without `REINDEX`, and the
  final pressure state was `none`;
- PostgreSQL temporary-file growth was 4,624,048 bytes;
- both fast restart and immediate crash/restart recovered 100 exact results
  from each index.

The oversized registry is bounded at 256 exact identities. Eviction can remove
only the fail-fast optimization; it cannot change ranking or publish an
inexact cache. Pressure accounting sums every active unified-delta segment in
the shared arena and adds only the current index's missing tail estimate.
Safely evictable base generations are excluded from this compaction signal.

Raw evidence:
[CAP-5 multi-index pressure](../data/raw/unified-delta-pressure-2026-07-29/cap5-pressure-multi-index-v6.json).

## Operational Boundary

The accepted default is two runtime workers and one retained ONNX session per
worker. Larger pools multiply measured model RSS and require deployment-specific
qualification. Shared arena headroom defaults to 25 percent. Operators should
size runtime workers, model sessions, the shared arena, maintenance memory, and
temporary storage as one capacity envelope.

The worker pool removes the serial execution bottleneck. Timeouts still bound
caller cancellation and abnormal publisher waits, but they are not presented
as throughput or admission solutions.
