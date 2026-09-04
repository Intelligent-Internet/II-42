# Unified Delta Cache Acceptance Evidence

Date: 2026-07-26

These files are immutable raw outputs for the relation-owned unified delta
cache milestone:

| File | Purpose |
|---|---|
| `ii42-full-overlay-base4096-final-3x101.json` | Old test-oracle full-overlay latency and scan baseline. |
| `ii42-delta-cache-product-acceptance-final-3x101.json` | Exactness, latency, 32-backend single-flight, and restart acceptance. |
| `ii42-delta-memory-scaling-final.json` | Per-query retained-memory scaling at three base sizes. |
| `ii42-delta-local-fallback-final.json` | Exact backend-local fallback with a zero-byte shared arena. |
| `ii42-lifecycle-unified-delta-final.json` | 47/47 mutable lifecycle gates. |
| `ii42-replication-unified-delta-final.json` | Primary and hot-standby lifecycle and cache rebuild. |
| `ii42-shared-preload-lifecycle-closure-final.log` | Shared arena admission and pressure-eviction closure. |
| `ii42-unified-delta-product-maturity-final.json` | Package-bound product maturity suite, including the 50,000-document medium lifecycle. |

The benchmark used PostgreSQL 18 on macOS with a local P2 model checkout. The
performance gate used 4,096 compacted base documents, `top_k=20`, three
independent trials, and 101 warm queries per trial. These are controlled product
acceptance measurements, not a hardware-independent latency claim.
