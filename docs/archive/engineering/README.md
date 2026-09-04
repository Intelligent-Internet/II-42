# Engineering Archive

This directory preserves closed engineering decisions, experiments, and
qualification ledgers. Statements about deployment, APIs, defaults, and remaining
work describe the recorded revision and date, not the current release.

- [Query-First Background Maintenance](query-first-eventual-background-maintenance.md)
  preserves the early separation of foreground retrieval and background
  convergence, including backfill and clean-query measurements. Its
  query-triggered worker implementation has been superseded.

- [Eventual SAE Release Readiness History](eventual-sae-release-readiness-history.md)
  records the closed path from split experimental lifecycles to the unified
  page-native product contract.
- [Convergent Segmented Index Development Record](convergent-segmented-index-development-record.md)
  preserves the implementation programme, issue history, and beta acceptance
  evidence for the current storage architecture.
- [Product Roadmap Through v0.2.5](product-roadmap-through-v0.2.5.md)
  preserves the detailed release, qualification, and deferred-work ledger that
  existed before the current roadmap was reduced to active work only.
- [psql_bm25s project narrative](psql-bm25s-project-narrative.md)
  preserves the lexical engine's motivation and early engineering workflow.

## Current Authorities

- [Convergent Segmented Index](../../convergent-segmented-index.md): storage,
  COW publication, query execution, and background convergence.
- [Maintenance lifecycle](../../maintenance-lifecycle.md): scheduling and
  operational interpretation of current state.
- [Query semantics](../../query-semantics.md): exact and bounded-baseline
  retrieval contracts, including filtered queries.
- [Product roadmap](../../product-roadmap.md): current engineering work.

Preserve historical measurements and their provenance. Do not reroute active
documentation through old plans, turn unchecked archive items into current
tasks, or use an archive to reopen retired APIs, formats, or a second lifecycle.
