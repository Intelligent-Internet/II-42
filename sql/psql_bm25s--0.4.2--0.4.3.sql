-- generated release upgrade
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
