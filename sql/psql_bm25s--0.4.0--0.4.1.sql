-- 0.4.1 updates binary behavior and refreshes scheduler-helper priority.

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

CREATE OR REPLACE FUNCTION psql_bm25s_index_maintain_due(
    max_indexes integer DEFAULT 1
)
RETURNS TABLE(index_oid regclass, result text)
LANGUAGE plpgsql VOLATILE PARALLEL UNSAFE
SET search_path FROM CURRENT
AS $$
DECLARE
    candidate regclass;
    extension_schema name;
    attempt text;
    emitted integer := 0;
BEGIN
    IF max_indexes IS NULL OR max_indexes < 1 THEN
        RETURN;
    END IF;

    SELECT n.nspname
    INTO extension_schema
    FROM pg_extension e
    JOIN pg_namespace n ON n.oid = e.extnamespace
    WHERE e.extname = 'psql_bm25s';

    IF extension_schema IS NULL THEN
        RAISE EXCEPTION 'psql_bm25s extension schema not found';
    END IF;

    FOR candidate IN EXECUTE format(
        $query$
        SELECT c.oid::regclass
        FROM pg_catalog.pg_class c
        JOIN pg_catalog.pg_am am ON am.oid = c.relam
        JOIN pg_catalog.pg_index i ON i.indexrelid = c.oid
        CROSS JOIN LATERAL %I.psql_bm25s_index_details(
            c.oid::regclass
        ) AS d
        WHERE c.relkind = 'i'
          AND am.amname = 'psql_bm25s'
          AND i.indisvalid
          AND i.indisready
          AND pg_catalog.pg_has_role(c.relowner, 'USAGE')
          AND coalesce(c.reloptions, ARRAY[]::text[])
              @> ARRAY['consistency=eventual']::text[]
          AND d.consistency = 'eventual'
          AND (
              d.stale
              OR (
                  d.pending_writes + d.pending_deletes > 0
                  AND (
                      d.pending_writes + d.pending_deletes <> d.delta_records
                      OR (
                          d.auto_rebuild_threshold > 0
                          AND d.pending_writes + d.pending_deletes
                              >= d.auto_rebuild_threshold
                      )
                      OR d.query_overlay_max_records = 0
                      OR d.delta_records > d.query_overlay_max_records
                      OR d.query_overlay_max_bytes = 0
                      OR d.delta_bytes > d.query_overlay_max_bytes
                  )
              )
          )
        ORDER BY d.stale DESC,
                 (
                     d.pending_writes + d.pending_deletes > 0
                     AND (
                         d.pending_writes + d.pending_deletes
                             <> d.delta_records
                         OR (
                             d.auto_rebuild_threshold > 0
                             AND d.pending_writes + d.pending_deletes
                                 >= d.auto_rebuild_threshold
                         )
                         OR d.query_overlay_max_records = 0
                         OR d.delta_records > d.query_overlay_max_records
                         OR d.query_overlay_max_bytes = 0
                         OR d.delta_bytes > d.query_overlay_max_bytes
                     )
                 ) DESC,
                 GREATEST(
                     d.pending_writes + d.pending_deletes,
                     d.delta_records
                 ) DESC,
                 d.delta_bytes DESC,
                 c.oid
        $query$,
        extension_schema
    )
    LOOP
        EXECUTE format(
            'SELECT %I.psql_bm25s_index_try_maintain($1)',
            extension_schema
        )
        INTO attempt
        USING candidate;
        IF attempt NOT LIKE '%reason=no_pending%'
           AND attempt NOT LIKE '%reason=lock_busy%'
           AND attempt NOT LIKE '%reason=concurrent_change%' THEN
            index_oid := candidate;
            result := attempt;
            RETURN NEXT;
            emitted := emitted + 1;
            IF emitted >= max_indexes THEN
                RETURN;
            END IF;
        END IF;
    END LOOP;
END;
$$;
