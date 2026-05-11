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

    /*
     * The C primitive owns the real due check and memory-budget admission.
     * Keep this external scheduler helper cheap enough to run from cron by
     * avoiding psql_bm25s_index_details on every large candidate.
     */
    FOR candidate IN
        SELECT c.oid::regclass
        FROM pg_catalog.pg_class c
        JOIN pg_catalog.pg_am am ON am.oid = c.relam
        JOIN pg_catalog.pg_index i ON i.indexrelid = c.oid
        WHERE c.relkind = 'i'
          AND am.amname = 'psql_bm25s'
          AND i.indisvalid
          AND i.indisready
          AND pg_catalog.pg_has_role(c.relowner, 'USAGE')
          AND coalesce(c.reloptions, ARRAY[]::text[])
              @> ARRAY['consistency=eventual']::text[]
        ORDER BY
          COALESCE((
              SELECT substring(opt FROM '^auto_preload=([0-9]+)$')::int4
              FROM pg_catalog.unnest(c.reloptions) AS opt
              WHERE opt LIKE 'auto_preload=%'
              LIMIT 1
          ), 0) DESC,
          pg_catalog.pg_relation_size(c.oid) DESC,
          c.oid
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
