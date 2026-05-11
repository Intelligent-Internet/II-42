-- 0.4.6 moves on-disk payloads to append-only generations. Existing 0.4.5
-- indexes that still use the legacy page-data layout must be rebuilt during
-- extension upgrade; otherwise the new reader intentionally reports the old
-- storage layout as unsupported.
DROP FUNCTION IF EXISTS psql_bm25s_index_details(regclass);

CREATE FUNCTION psql_bm25s_index_details(index_name regclass)
RETURNS TABLE(
    index_name regclass,
    source_type text,
    docs int8,
    index_bytes int8,
    pages int8,
    stale bool,
    consistency text,
    rebuilds int8,
    pending_writes int8,
    pending_deletes int8,
    delta_records int8,
    delta_bytes int8,
    auto_rebuild_threshold int4,
    auto_rebuild_delta_bytes int4,
    auto_rebuild_churn_ratio float8,
    query_overlay_max_records int4,
    query_overlay_max_bytes int4
)
AS 'MODULE_PATHNAME', 'psql_bm25s_index_details'
LANGUAGE C STABLE PARALLEL SAFE;

DO $$
DECLARE
    candidate regclass;
    rebuilt integer := 0;
BEGIN
    FOR candidate IN
        SELECT c.oid::regclass
        FROM pg_catalog.pg_class c
        JOIN pg_catalog.pg_am am ON am.oid = c.relam
        JOIN pg_catalog.pg_index i ON i.indexrelid = c.oid
        WHERE c.relkind = 'i'
          AND am.amname = 'psql_bm25s'
          AND i.indisvalid
          AND i.indisready
        ORDER BY c.oid
    LOOP
        RAISE NOTICE
            'reindexing psql_bm25s index % for 0.4.6 storage',
            candidate;
        EXECUTE format('REINDEX INDEX %s', candidate);
        rebuilt := rebuilt + 1;
    END LOOP;

    IF rebuilt > 0 THEN
        RAISE NOTICE
            'reindexed % psql_bm25s index(es) for 0.4.6 storage',
            rebuilt;
    END IF;
END;
$$;
