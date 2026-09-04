# SAE Block-Max Phase 3 PostgreSQL Read-Only Payload Report

## Scope

Phase 3 starts the PostgreSQL integration path without changing the mutable
index-maintenance model.

Implemented scope:

- read-only SAE block-max payload traversal inside PostgreSQL;
- reuse of the Phase 2 C traversal contract;
- SQL-facing rows with TID, score, document id, rank, and traversal diagnostics;
- optional `tid[]` mapping from `doc_ord` to heap TID;
- no delta overlay;
- no mutable maintenance;
- no PostgreSQL access-method write path.

This is intentionally a scaffold. The current research payload still contains
debug document ids and expected rows. A production payload should replace those
with compact TID and offset tables after the representation/layout questions
are resolved.

## SQL Surface

```sql
SELECT *
FROM ii42_sae_block_max_query(
    payload => $1::bytea,
    query_filter => NULL,
    doc_tids => NULL
);
```

The function returns:

```text
query_id
rank
ctid
doc_ord
document_id
score
opened_blocks
opened_docs
scored_docs
block_entry_visits
block_entry_binary_steps
posting_slice_hits
decoded_postings
memory_bytes
```

`doc_tids` is optional. When supplied, it must be ordered by `doc_ord`:

```sql
WITH doc_tids AS (
    SELECT array_agg(ctid ORDER BY doc_ord) AS tids
    FROM sae_document_mapping
)
SELECT *
FROM doc_tids,
     ii42_sae_block_max_query(
         $1::bytea,
         'query-alpha',
         doc_tids.tids
     );
```

This keeps the first PostgreSQL prototype read-only: the payload can remain a
portable bytea artifact while SQL can still receive heap TIDs.

## Implementation

New files:

```text
src/ii42_sae_blockmax.c
scripts/test_research_sae_block_max_pg_readonly.py
```

Updated files:

```text
Makefile
sql/ii42--0.4.7.sql
sql/ii42--0.4.6--0.4.7.sql
```

The C function:

1. decodes the `SBMXP001` binary payload from `bytea`;
2. builds the same `global_dim_id -> block_entry_range` lookup used by the
   standalone C reader;
3. computes block upper bounds for each query;
4. traverses blocks in decreasing bound order;
5. stops when the current top-k threshold dominates remaining block bounds;
6. emits materialized SQL rows with per-query diagnostics.

The PostgreSQL function is marked `STABLE PARALLEL SAFE` because the payload is
read-only and the function does not touch heap/index contents by itself.

## Verification

Build:

```bash
make -s \
  PG_CONFIG=/opt/homebrew/opt/postgresql@18/bin/pg_config \
  PG_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
```

PostgreSQL read-only smoke:

```bash
python3 scripts/test_research_sae_block_max_pg_readonly.py --temp-postgres
```

Result:

```text
block-max PostgreSQL read-only payload smoke passed
```

The smoke test verifies:

- SQL rankings match payload expected rankings for all queries;
- `query_filter` restricts execution to one query;
- `tid[]` mapping returns non-null TIDs;
- diagnostics such as `opened_docs` are populated.

Standalone C reader regression:

```bash
python3 scripts/test_research_sae_block_max_c_reader.py
```

Result:

```text
queries=3 exact_matches=3
block-max C reader smoke passed
```

## Current Bottleneck

Phase 3 confirms that PostgreSQL can host the read-only traversal, but it does
not solve the main retrieval-system issue identified in Phase 2.6:

```text
SAE block bounds still open most documents on current benchmark artifacts.
```

That is a representation/layout problem, not a C lookup problem. The next
useful engineering work should focus on making block upper bounds more
selective before adding mutable overlays or access-method maintenance.

## Next Step

The next minimal step should not add write-path complexity. It should replace
the debug bytea payload with a compact PostgreSQL-resident read-only payload:

- compact document table keyed by TID/doc_ord;
- compact block directory;
- compact dimension/block-entry/posting arrays;
- same SQL result contract;
- same diagnostics;
- exact parity with the bytea function and standalone C reader.

Only after that should the branch consider mutable delta overlay, MVCC delete
handling, shared generation publishing, or BM25+SAE unified source adapters.
