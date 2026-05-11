CREATE TYPE psql_bm25s_index;

CREATE FUNCTION psql_bm25s_handler(internal)
RETURNS index_am_handler
AS 'MODULE_PATHNAME', 'psql_bm25s_handler'
LANGUAGE C;

CREATE FUNCTION psql_bm25s_in(cstring)
RETURNS psql_bm25s_index
AS 'MODULE_PATHNAME', 'psql_bm25s_in'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_out(psql_bm25s_index)
RETURNS cstring
AS 'MODULE_PATHNAME', 'psql_bm25s_out'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_recv(internal)
RETURNS psql_bm25s_index
AS 'MODULE_PATHNAME', 'psql_bm25s_recv'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_send(psql_bm25s_index)
RETURNS bytea
AS 'MODULE_PATHNAME', 'psql_bm25s_send'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE TYPE psql_bm25s_index (
    INPUT = psql_bm25s_in,
    OUTPUT = psql_bm25s_out,
    RECEIVE = psql_bm25s_recv,
    SEND = psql_bm25s_send,
    INTERNALLENGTH = variable,
    STORAGE = extended,
    ALIGNMENT = int4
);

CREATE TYPE psql_bm25s_hit AS (
    ctid tid,
    doc_id int4,
    score real
);

CREATE TYPE psql_bm25s_prepared_query AS (
    index_name regclass,
    query_text text,
    lowercase boolean,
    stopwords text[],
    stem_english boolean,
    fold_diacritics boolean
);

CREATE TYPE psql_bm25s_ranked_query AS (
    prepared_query psql_bm25s_prepared_query,
    order_tokens text[],
    k int4,
    weight_mask real[]
);

CREATE TYPE psql_bm25s_weighted_query AS (
    prepared_query psql_bm25s_prepared_query,
    weight real
);

CREATE TYPE psql_bm25s_field_query AS (
    field_name text,
    weighted_query psql_bm25s_weighted_query
);

CREATE TYPE psql_bm25s_hybrid_candidate AS (
    source_name text,
    ctid tid,
    raw_value real,
    source_rank int4,
    weight real,
    normalizer text,
    direction text
);

CREATE TYPE psql_bm25s_hybrid_hit AS (
    ctid tid,
    score real,
    source_count int4,
    source_names text[],
    raw_values real[],
    normalized_scores real[],
    weighted_scores real[],
    ranks int4[]
);

CREATE ACCESS METHOD psql_bm25s TYPE INDEX HANDLER psql_bm25s_handler;

COMMENT ON ACCESS METHOD psql_bm25s IS
'BM25S index access method backed by a serialized eager sparse index';

CREATE FUNCTION psql_bm25s_index_describe(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_index_describe'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_index_maintenance_state(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_index_maintenance_state'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_index_maintenance_policy(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_index_maintenance_policy'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_index_maintenance_policy_details(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_index_maintenance_policy_details'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_recommend_maintenance_policy(
    index_name regclass,
    profile text DEFAULT 'balanced'
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_recommend_maintenance_policy'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_fast_path_advice(index_name regclass)
RETURNS jsonb
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH idx AS (
        SELECT
            am.amname AS access_method,
            i.indnatts AS indnatts,
            a.atttypid::regtype::text AS doc_type
        FROM pg_class idx
        JOIN pg_index i
            ON i.indexrelid = idx.oid
        JOIN pg_am am
            ON am.oid = idx.relam
        LEFT JOIN pg_attribute a
            ON a.attrelid = idx.oid
           AND a.attnum = 1
           AND NOT a.attisdropped
        WHERE idx.oid = $1
    )
    SELECT CASE
        WHEN access_method <> 'psql_bm25s' THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', false,
                'supports_filtered_ranked', false,
                'canonical_api', NULL,
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Index does not use the psql_bm25s access method.'
                ]::text[])
            )
        WHEN indnatts > 1
            AND doc_type IN ('text[]', 'character varying[]', 'varchar[]') THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', false,
                'supports_filtered_ranked', false,
                'canonical_api',
                    'psql_bm25s_search_tokens(index_name, query_tokens, ...)',
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Multicolumn fusion indexes use direct regclass retrieval APIs.',
                    '@@, @@@, and <=> are not exposed for multicolumn fusion indexes.',
                    'Use psql_bm25s_search_tokens(...) or psql_bm25s_search_query(...) for fused retrieval.'
                ]::text[])
            )
        WHEN indnatts > 1
            AND doc_type IN ('text', 'character varying', 'varchar') THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', false,
                'supports_filtered_ranked', false,
                'canonical_api',
                    'psql_bm25s_search_query(index_name, query_text, ...)',
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Scalar multicolumn fusion indexes use direct regclass retrieval APIs.',
                    '@@, @@@, and <=> are not exposed for multicolumn fusion indexes.',
                    'Each indexed scalar column is tokenized with the index text options before fusion.'
                ]::text[])
            )
        WHEN doc_type IN ('text[]', 'character varying[]', 'varchar[]') THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', true,
                'supports_prepared_match', true,
                'supports_ordering', true,
                'supports_filtered_ranked', true,
                'canonical_api',
                    'psql_bm25s_search_query(index_name, query_text, ...)',
                'recommended_filter',
                    'tokens @@@ psql_bm25s_query(index_name, query_text, ...)',
                'recommended_order',
                    'ORDER BY tokens <=> psql_bm25s_order_tokens(index_name, query_text, ...) ASC LIMIT k',
                'notes', to_jsonb(ARRAY[
                    '@@ is a boolean text predicate.',
                    '@@@ is the structured prepared-query predicate.',
                    '<=> aligns to BM25 ordering only when PostgreSQL uses a real psql_bm25s index scan.',
                    'For filtered ranked SQL, prefer @@@ plus psql_bm25s_order_tokens(index_name, query_text, ...).'
                ]::text[])
            )
        WHEN doc_type IN ('text', 'character varying', 'varchar') THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', true,
                'supports_prepared_match', true,
                'supports_ordering', true,
                'supports_filtered_ranked', true,
                'canonical_api',
                    'psql_bm25s_search_query(index_name, query_text, ...)',
                'recommended_filter',
                    'column @@@ psql_bm25s_query(index_name, query_text, ...)',
                'recommended_order',
                    'ORDER BY column <=> psql_bm25s_order_tokens(index_name, query_text, ...) ASC LIMIT k',
                'notes', to_jsonb(ARRAY[
                    '@@ is available for plain-text filtering on scalar text and varchar indexes.',
                    '@@@ is the structured prepared-query predicate and is preferred when text options matter.',
                    '<=> aligns to BM25 ordering only when PostgreSQL uses a real psql_bm25s index scan.',
                    'Outside a real index scan, scalar text and varchar operators use explicit query options, not hidden index reloptions.',
                    'psql_bm25s_query(index_name, ...) resolves omitted scalar text options from the named index reloptions.',
                    'When text options matter outside index scans, prefer psql_bm25s_match_query(...), psql_bm25s_score_query(...), or explicit psql_bm25s_query(...) and psql_bm25s_order_tokens(...).',
                    'Phrase and verified retrieval retokenize heap values with the index text options.'
                ]::text[])
            )
        WHEN doc_type IN ('integer[]', 'int4[]') THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', true,
                'supports_filtered_ranked', false,
                'canonical_api',
                    'psql_bm25s_search_ids(index_name, query_ids, ...)',
                'recommended_filter', NULL,
                'recommended_order',
                    'ORDER BY token_ids <=> query_ids ASC LIMIT k',
                'notes', to_jsonb(ARRAY[
                    'int4[] indexes only expose ordered retrieval through <=>.',
                    'No @@ or @@@ predicate operators are defined for int4[] indexes.',
                    'Use the ids result API for the clearest exact BM25 contract.'
                ]::text[])
            )
        ELSE
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', false,
                'supports_filtered_ranked', false,
                'canonical_api', NULL,
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Unrecognized psql_bm25s index key type.'
                ]::text[])
            )
    END
    FROM idx
$$;

CREATE FUNCTION psql_bm25s_plan_fast_path(
    index_name regclass,
    explain_plan jsonb
)
RETURNS jsonb
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH RECURSIVE target AS (
        SELECT relname
        FROM pg_class
        WHERE oid = $1
    ),
    root(plan_node) AS (
        SELECT CASE
            WHEN jsonb_typeof($2) = 'array' THEN ($2->0)->'Plan'
            WHEN $2 ? 'Plan' THEN $2->'Plan'
            ELSE $2
        END
    ),
    nodes(plan_node) AS (
        SELECT plan_node
        FROM root
        WHERE plan_node IS NOT NULL
        UNION ALL
        SELECT child
        FROM nodes n
        CROSS JOIN LATERAL jsonb_array_elements(
            COALESCE(n.plan_node->'Plans', '[]'::jsonb)
        ) child
    ),
    annotated AS (
        SELECT
            plan_node->>'Node Type' AS node_type,
            plan_node->>'Index Name' AS index_name,
            plan_node->>'Index Cond' AS index_cond,
            plan_node->>'Recheck Cond' AS recheck_cond,
            plan_node->>'Filter' AS filter_cond,
            plan_node->>'Order By' AS order_by_expr,
            plan_node->>'Sort Key' AS sort_key_expr
        FROM nodes
    )
    SELECT jsonb_build_object(
        'index_name', $1::text,
        'matched_index_nodes',
        COUNT(*) FILTER (
            WHERE annotated.index_name = target.relname
        ),
        'used_psql_bm25s_index',
        COALESCE(
            BOOL_OR(annotated.index_name = target.relname),
            false
        ),
        'used_index_scan',
        COALESCE(
            BOOL_OR(
                annotated.index_name = target.relname
                AND annotated.node_type = 'Index Scan'
            ),
            false
        ),
        'used_bitmap_index_scan',
        COALESCE(
            BOOL_OR(
                annotated.index_name = target.relname
                AND annotated.node_type = 'Bitmap Index Scan'
            ),
            false
        ),
        'used_ordered_index_scan',
        COALESCE(
            BOOL_OR(
                annotated.index_name = target.relname
                AND annotated.node_type = 'Index Scan'
                AND POSITION('<=>' IN
                    COALESCE(annotated.order_by_expr, '')
                ) > 0
            ),
            false
        ),
        'uses_plain_match',
        COALESCE(
            BOOL_OR(
                annotated.index_name = target.relname
                AND POSITION('@@@' IN
                    CONCAT_WS(
                        ' ',
                        COALESCE(annotated.index_cond, ''),
                        COALESCE(annotated.recheck_cond, ''),
                        COALESCE(annotated.filter_cond, '')
                    )
                ) = 0
                AND POSITION('@@' IN
                    CONCAT_WS(
                        ' ',
                        COALESCE(annotated.index_cond, ''),
                        COALESCE(annotated.recheck_cond, ''),
                        COALESCE(annotated.filter_cond, '')
                    )
                ) > 0
            ),
            false
        ),
        'uses_prepared_match',
        COALESCE(
            BOOL_OR(
                annotated.index_name = target.relname
                AND POSITION('@@@' IN
                    CONCAT_WS(
                        ' ',
                        COALESCE(annotated.index_cond, ''),
                        COALESCE(annotated.recheck_cond, ''),
                        COALESCE(annotated.filter_cond, '')
                    )
                ) > 0
            ),
            false
        ),
        'uses_ordering_operator',
        COALESCE(
            BOOL_OR(
                annotated.index_name = target.relname
                AND (
                    POSITION('<=>' IN
                        COALESCE(annotated.order_by_expr, '')
                    ) > 0
                    OR POSITION('<=>' IN
                        COALESCE(annotated.sort_key_expr, '')
                    ) > 0
                )
            ),
            false
        ),
        'matched_node_types',
        COALESCE(
            to_jsonb(
                ARRAY(
                    SELECT DISTINCT a.node_type
                    FROM annotated a, target
                    WHERE a.index_name = target.relname
                    ORDER BY 1
                )
            ),
            '[]'::jsonb
        )
    )
    FROM annotated, target
$$;

CREATE FUNCTION psql_bm25s_explain_fast_path(
    index_name regclass,
    sql_text text
)
RETURNS jsonb
LANGUAGE plpgsql VOLATILE PARALLEL UNSAFE
AS $$
DECLARE
    explain_row text;
    extension_schema name;
    result jsonb;
BEGIN
    SELECT n.nspname
    INTO extension_schema
    FROM pg_extension e
    JOIN pg_namespace n ON n.oid = e.extnamespace
    WHERE e.extname = 'psql_bm25s';

    IF extension_schema IS NULL THEN
        RAISE EXCEPTION 'psql_bm25s extension schema not found';
    END IF;

    EXECUTE 'EXPLAIN (FORMAT JSON) ' || sql_text
        INTO explain_row;
    EXECUTE format(
        'SELECT %I.psql_bm25s_plan_fast_path($1, $2)',
        extension_schema
    )
    INTO result
    USING index_name, explain_row::jsonb;
    RETURN result;
END;
$$;

CREATE FUNCTION psql_bm25s_refresh_index(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_refresh_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE FUNCTION psql_bm25s_maintain_index(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_maintain_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE FUNCTION psql_bm25s_try_maintain_index(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_try_maintain_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE FUNCTION psql_bm25s_maintain_due_indexes(max_indexes integer DEFAULT 1)
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
        ORDER BY c.oid
    LOOP
        EXECUTE format(
            'SELECT %I.psql_bm25s_try_maintain_index($1)',
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

CREATE FUNCTION psql_bm25s_score_ids_op(doc_ids int4[], query_ids int4[])
RETURNS float8
AS 'MODULE_PATHNAME', 'psql_bm25s_score_ids_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_score_tokens_op(doc_tokens text[], query_tokens text[])
RETURNS float8
AS 'MODULE_PATHNAME', 'psql_bm25s_score_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_score_tokens_op(
    doc_tokens text[],
    query_tokens varchar[]
)
RETURNS float8
AS 'MODULE_PATHNAME', 'psql_bm25s_score_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_score_tokens_op(
    doc_tokens varchar[],
    query_tokens text[]
)
RETURNS float8
AS 'MODULE_PATHNAME', 'psql_bm25s_score_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_score_tokens_op(
    doc_tokens varchar[],
    query_tokens varchar[]
)
RETURNS float8
AS 'MODULE_PATHNAME', 'psql_bm25s_score_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_match_query_tokens_op(doc_tokens text[], query_text text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'psql_bm25s_match_query_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_match_query_tokens_op(
    doc_tokens varchar[],
    query_text text
)
RETURNS boolean
AS 'MODULE_PATHNAME', 'psql_bm25s_match_query_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_score_scalar_text_op(
    doc_text text,
    query_tokens text[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_score_tokens_op(
            psql_bm25s_tokenize_text($1, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE FUNCTION psql_bm25s_score_scalar_text_op(
    doc_text text,
    query_tokens varchar[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_score_tokens_op(
            psql_bm25s_tokenize_text($1, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE FUNCTION psql_bm25s_score_scalar_text_op(
    doc_text varchar,
    query_tokens text[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_score_tokens_op(
            psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE FUNCTION psql_bm25s_score_scalar_text_op(
    doc_text varchar,
    query_tokens varchar[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_score_tokens_op(
            psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE FUNCTION psql_bm25s_match_query_scalar_op(
    doc_text text,
    query_text text
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_query_tokens_op(
        psql_bm25s_tokenize_text($1, true, NULL, false, false),
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_query_scalar_op(
    doc_text varchar,
    query_text text
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_query_tokens_op(
        psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
        $2
    )
$$;

CREATE OPERATOR <=> (
    LEFTARG = int4[],
    RIGHTARG = int4[],
    PROCEDURE = psql_bm25s_score_ids_op
);

CREATE OPERATOR <=> (
    LEFTARG = text[],
    RIGHTARG = text[],
    PROCEDURE = psql_bm25s_score_tokens_op
);

CREATE OPERATOR <=> (
    LEFTARG = text[],
    RIGHTARG = varchar[],
    PROCEDURE = psql_bm25s_score_tokens_op
);

CREATE OPERATOR <=> (
    LEFTARG = varchar[],
    RIGHTARG = text[],
    PROCEDURE = psql_bm25s_score_tokens_op
);

CREATE OPERATOR <=> (
    LEFTARG = varchar[],
    RIGHTARG = varchar[],
    PROCEDURE = psql_bm25s_score_tokens_op
);

CREATE OPERATOR <=> (
    LEFTARG = text,
    RIGHTARG = text[],
    PROCEDURE = psql_bm25s_score_scalar_text_op
);

CREATE OPERATOR <=> (
    LEFTARG = text,
    RIGHTARG = varchar[],
    PROCEDURE = psql_bm25s_score_scalar_text_op
);

CREATE OPERATOR <=> (
    LEFTARG = varchar,
    RIGHTARG = text[],
    PROCEDURE = psql_bm25s_score_scalar_text_op
);

CREATE OPERATOR <=> (
    LEFTARG = varchar,
    RIGHTARG = varchar[],
    PROCEDURE = psql_bm25s_score_scalar_text_op
);

CREATE OPERATOR @@ (
    LEFTARG = text[],
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_tokens_op
);

CREATE OPERATOR @@ (
    LEFTARG = varchar[],
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_tokens_op
);

CREATE OPERATOR @@ (
    LEFTARG = text,
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_scalar_op
);

CREATE OPERATOR @@ (
    LEFTARG = varchar,
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_scalar_op
);

CREATE OPERATOR CLASS psql_bm25s_text_array_ops
DEFAULT FOR TYPE text[] USING psql_bm25s AS
    OPERATOR 1 @@ (text[], text),
    OPERATOR 1 <=> (text[], text[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE text[];

CREATE OPERATOR CLASS psql_bm25s_varchar_array_ops
DEFAULT FOR TYPE varchar[] USING psql_bm25s AS
    OPERATOR 1 @@ (varchar[], text),
    OPERATOR 1 <=> (varchar[], text[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE varchar[];

CREATE OPERATOR CLASS psql_bm25s_text_ops
DEFAULT FOR TYPE text USING psql_bm25s AS
    OPERATOR 1 @@ (text, text),
    OPERATOR 1 <=> (text, text[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE text;

CREATE OPERATOR CLASS psql_bm25s_varchar_ops
DEFAULT FOR TYPE varchar USING psql_bm25s AS
    OPERATOR 1 @@ (varchar, text),
    OPERATOR 1 <=> (varchar, text[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE varchar;

CREATE OPERATOR CLASS psql_bm25s_int4_array_ops
DEFAULT FOR TYPE int4[] USING psql_bm25s AS
    OPERATOR 1 <=> (int4[], int4[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE int4[];

CREATE FUNCTION psql_bm25s_search_ids(
    index_name regclass,
    query_ids int4[],
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_search_ids'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_search_tokens(
    index_name regclass,
    query_tokens text[],
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_search_tokens'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_search_query(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_search_query'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_search_query_cfg(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS SETOF psql_bm25s_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_search_query_cfg'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_prepare_query(
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_prepared_query
AS 'MODULE_PATHNAME', 'psql_bm25s_prepare_query_resolved'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_query(
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_prepared_query
AS 'MODULE_PATHNAME', 'psql_bm25s_prepare_query_resolved'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_ranked_query(
    prepared_query psql_bm25s_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS psql_bm25s_ranked_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW(
        $1,
        psql_bm25s_order_tokens($1),
        $2,
        $3
    )::psql_bm25s_ranked_query
$$;

CREATE FUNCTION psql_bm25s_ranked_query(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_ranked_query
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_ranked_query(
        psql_bm25s_query($1, $2, $5, $6, $7, $8),
        $3,
        $4
    )
$$;

CREATE FUNCTION psql_bm25s_weighted_query(
    prepared_query psql_bm25s_prepared_query,
    weight real DEFAULT 1.0
)
RETURNS psql_bm25s_weighted_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW($1, COALESCE($2, 1.0))::psql_bm25s_weighted_query
$$;

CREATE FUNCTION psql_bm25s_weighted_query(
    index_name regclass,
    query_text text,
    weight real DEFAULT 1.0,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_weighted_query
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_weighted_query(
        psql_bm25s_query($1, $2, $4, $5, $6, $7),
        $3
    )
$$;

CREATE FUNCTION psql_bm25s_weighted_queries(
    index_names regclass[],
    query_text text,
    weights real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_weighted_query[]
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT COALESCE(
        array_agg(
            psql_bm25s_weighted_query(
                index_names[i],
                query_text,
                COALESCE(weights[i], 1.0),
                lowercase,
                stopwords,
                stem_english,
                fold_diacritics
            )
            ORDER BY i
        ),
        ARRAY[]::psql_bm25s_weighted_query[]
    )
    FROM generate_subscripts(COALESCE(index_names, ARRAY[]::regclass[]), 1) AS i
$$;

CREATE FUNCTION psql_bm25s_field_query(
    field_name text,
    weighted_query psql_bm25s_weighted_query
)
RETURNS psql_bm25s_field_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW($1, $2)::psql_bm25s_field_query
$$;

CREATE FUNCTION psql_bm25s_field_query(
    field_name text,
    prepared_query psql_bm25s_prepared_query,
    weight real DEFAULT 1.0
)
RETURNS psql_bm25s_field_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_field_query(
        $1,
        psql_bm25s_weighted_query($2, $3)
    )
$$;

CREATE FUNCTION psql_bm25s_field_query(
    field_name text,
    index_name regclass,
    query_text text,
    weight real DEFAULT 1.0,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_field_query
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_field_query(
        $1,
        psql_bm25s_weighted_query(
            $2,
            $3,
            $4,
            $5,
            $6,
            $7,
            $8
        )
    )
$$;

CREATE FUNCTION psql_bm25s_field_queries(
    field_names text[],
    index_names regclass[],
    query_text text,
    weights real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_field_query[]
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT COALESCE(
        array_agg(
            psql_bm25s_field_query(
                COALESCE(field_names[i], format('field_%s', i)),
                index_names[i],
                query_text,
                COALESCE(weights[i], 1.0),
                lowercase,
                stopwords,
                stem_english,
                fold_diacritics
            )
            ORDER BY i
        ),
        ARRAY[]::psql_bm25s_field_query[]
    )
    FROM generate_subscripts(COALESCE(index_names, ARRAY[]::regclass[]), 1) AS i
$$;

CREATE FUNCTION psql_bm25s_search_prepared_query(
    prepared_query psql_bm25s_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_search_query_cfg(
        ($1).index_name,
        ($1).query_text,
        $2,
        $3,
        ($1).lowercase,
        ($1).stopwords,
        ($1).stem_english,
        ($1).fold_diacritics
    )
$$;



CREATE FUNCTION psql_bm25s_filter_query(
    ranked_query psql_bm25s_ranked_query
)
RETURNS psql_bm25s_prepared_query
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ($1).prepared_query
$$;

CREATE FUNCTION psql_bm25s_order_tokens(
    prepared_query psql_bm25s_prepared_query
)
RETURNS text[]
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_tokenize_text(
        ($1).query_text,
        ($1).lowercase,
        ($1).stopwords,
        ($1).stem_english,
        ($1).fold_diacritics
    )
$$;

CREATE FUNCTION psql_bm25s_order_tokens(
    ranked_query psql_bm25s_ranked_query
)
RETURNS text[]
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ($1).order_tokens
$$;

CREATE FUNCTION psql_bm25s_order_tokens(
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS text[]
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_order_tokens(
        psql_bm25s_query($1, $2, $3, $4, $5, $6)
    )
$$;






CREATE FUNCTION psql_bm25s_hybrid_check_fusion(fusion text)
RETURNS text
LANGUAGE plpgsql IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
DECLARE
    value text := lower(COALESCE(fusion, 'rrf'));
BEGIN
    IF value NOT IN ('rrf', 'score') THEN
        RAISE EXCEPTION 'unsupported psql_bm25s hybrid fusion method: %',
            fusion
            USING HINT = 'Use rrf or score.';
    END IF;
    RETURN value;
END;
$$;

CREATE FUNCTION psql_bm25s_hybrid_check_normalizer(normalizer text)
RETURNS text
LANGUAGE plpgsql IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
DECLARE
    value text := lower(COALESCE(normalizer, 'identity'));
BEGIN
    IF value NOT IN (
        'identity',
        'negative_distance',
        'inverse_distance',
        'minmax',
        'zscore',
        'rank'
    ) THEN
        RAISE EXCEPTION 'unsupported psql_bm25s hybrid normalizer: %',
            normalizer
            USING HINT = 'Use identity, negative_distance, inverse_distance, minmax, zscore, or rank.';
    END IF;
    RETURN value;
END;
$$;

CREATE FUNCTION psql_bm25s_hybrid_check_direction(direction text)
RETURNS text
LANGUAGE plpgsql IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
DECLARE
    value text := lower(COALESCE(direction, 'higher_is_better'));
BEGIN
    IF value NOT IN ('higher_is_better', 'lower_is_better') THEN
        RAISE EXCEPTION 'unsupported psql_bm25s hybrid direction: %',
            direction
            USING HINT = 'Use higher_is_better or lower_is_better.';
    END IF;
    RETURN value;
END;
$$;

CREATE FUNCTION psql_bm25s_hybrid_candidate(
    source_name text,
    ctid tid,
    raw_value real,
    source_rank int4,
    weight real DEFAULT 1.0,
    normalizer text DEFAULT 'identity',
    direction text DEFAULT 'higher_is_better'
)
RETURNS psql_bm25s_hybrid_candidate
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW(
        COALESCE(NULLIF($1, ''), 'source'),
        $2,
        $3,
        $4,
        COALESCE($5, 1.0),
        psql_bm25s_hybrid_check_normalizer($6),
        psql_bm25s_hybrid_check_direction($7)
    )::psql_bm25s_hybrid_candidate
$$;

CREATE FUNCTION psql_bm25s_hybrid_bm25_candidate(
    source_name text,
    ctid tid,
    score real,
    source_rank int4,
    weight real DEFAULT 1.0,
    normalizer text DEFAULT 'identity'
)
RETURNS psql_bm25s_hybrid_candidate
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_hybrid_candidate(
        $1,
        $2,
        $3,
        $4,
        $5,
        COALESCE($6, 'identity'),
        'higher_is_better'
    )
$$;

CREATE FUNCTION psql_bm25s_hybrid_vector_candidate(
    source_name text,
    ctid tid,
    distance real,
    source_rank int4,
    weight real DEFAULT 1.0,
    normalizer text DEFAULT 'negative_distance'
)
RETURNS psql_bm25s_hybrid_candidate
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_hybrid_candidate(
        $1,
        $2,
        $3,
        $4,
        $5,
        COALESCE($6, 'negative_distance'),
        'lower_is_better'
    )
$$;

CREATE FUNCTION psql_bm25s_hybrid_bm25_candidates(
    source_name text,
    index_name regclass,
    query_text text,
    weight real DEFAULT 1.0,
    candidate_k int4 DEFAULT 100,
    normalizer text DEFAULT 'identity',
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS SETOF psql_bm25s_hybrid_candidate
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_hybrid_bm25_candidate(
        $1,
        h.ctid,
        h.score,
        row_number() OVER (ORDER BY h.score DESC, h.ctid::text)::int4,
        $4,
        $6
    )
    FROM psql_bm25s_search_prepared_query(
        psql_bm25s_query($2, $3, $7, $8, $9, $10),
        GREATEST(COALESCE($5, 100), 0),
        NULL
    ) AS h
    ORDER BY h.score DESC, h.ctid::text
$$;

CREATE FUNCTION psql_bm25s_hybrid_fuse_candidates_sql_reference(
    candidates psql_bm25s_hybrid_candidate[],
    k int4 DEFAULT 10,
    fusion text DEFAULT 'rrf',
    rrf_k real DEFAULT 60.0,
    epsilon real DEFAULT 0.000001
)
RETURNS SETOF psql_bm25s_hybrid_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH params AS (
        SELECT
            psql_bm25s_hybrid_check_fusion($3) AS fusion_method,
            GREATEST(COALESCE($4, 60.0), 0.000001)::float8 AS rrf_k,
            GREATEST(COALESCE($5, 0.000001), 0.000000000001)::float8 AS epsilon
    ),
    raw_input AS (
        SELECT
            COALESCE(NULLIF((c).source_name, ''), 'source') AS source_name,
            (c).ctid AS ctid,
            (c).raw_value::float8 AS raw_value,
            (c).source_rank AS source_rank,
            COALESCE((c).weight, 1.0)::float8 AS weight,
            psql_bm25s_hybrid_check_direction((c).direction) AS direction,
            (c).normalizer AS normalizer
        FROM unnest(
            COALESCE($1, ARRAY[]::psql_bm25s_hybrid_candidate[])
        ) AS c
        WHERE
            (c).ctid IS NOT NULL
            AND (c).raw_value IS NOT NULL
            AND (c).raw_value > '-Infinity'::real
            AND (c).raw_value < 'Infinity'::real
    ),
    normalized_input AS (
        SELECT
            source_name,
            ctid,
            raw_value,
            weight,
            direction,
            psql_bm25s_hybrid_check_normalizer(
                COALESCE(
                    normalizer,
                    CASE
                        WHEN direction = 'lower_is_better'
                        THEN 'negative_distance'
                        ELSE 'identity'
                    END
                )
            ) AS normalizer,
            COALESCE(
                NULLIF(source_rank, 0),
                row_number() OVER (
                    PARTITION BY source_name
                    ORDER BY
                        CASE
                            WHEN direction = 'lower_is_better'
                            THEN raw_value
                        END ASC NULLS LAST,
                        CASE
                            WHEN direction = 'higher_is_better'
                            THEN raw_value
                        END DESC NULLS LAST,
                        ctid::text
                )::int4
            ) AS source_rank
        FROM raw_input
    ),
    stats AS (
        SELECT
            source_name,
            min(raw_value) AS min_value,
            max(raw_value) AS max_value,
            avg(raw_value) AS avg_value,
            stddev_pop(raw_value) AS stddev_value
        FROM normalized_input
        GROUP BY source_name
    ),
    scored AS (
        SELECT
            n.source_name,
            n.ctid,
            n.raw_value,
            n.source_rank,
            n.direction,
            n.weight,
            CASE
                WHEN p.fusion_method = 'rrf'
                THEN 1.0 / (p.rrf_k + n.source_rank)
                WHEN n.normalizer = 'identity'
                THEN
                    CASE
                        WHEN n.direction = 'lower_is_better'
                        THEN -n.raw_value
                        ELSE n.raw_value
                    END
                WHEN n.normalizer = 'negative_distance'
                THEN -n.raw_value
                WHEN n.normalizer = 'inverse_distance'
                THEN 1.0 / GREATEST(p.epsilon, n.raw_value + p.epsilon)
                WHEN n.normalizer = 'minmax'
                THEN
                    CASE
                        WHEN s.max_value = s.min_value
                        THEN 1.0
                        WHEN n.direction = 'lower_is_better'
                        THEN (s.max_value - n.raw_value)
                            / (s.max_value - s.min_value)
                        ELSE (n.raw_value - s.min_value)
                            / (s.max_value - s.min_value)
                    END
                WHEN n.normalizer = 'zscore'
                THEN
                    CASE
                        WHEN s.stddev_value IS NULL OR s.stddev_value = 0.0
                        THEN 0.0
                        WHEN n.direction = 'lower_is_better'
                        THEN (s.avg_value - n.raw_value) / s.stddev_value
                        ELSE (n.raw_value - s.avg_value) / s.stddev_value
                    END
                WHEN n.normalizer = 'rank'
                THEN 1.0 / GREATEST(n.source_rank, 1)
            END AS normalized_score
        FROM normalized_input n
        JOIN stats s ON s.source_name = n.source_name
        CROSS JOIN params p
        WHERE n.source_rank IS NULL OR n.source_rank > 0
    ),
    weighted AS (
        SELECT
            source_name,
            ctid,
            raw_value,
            source_rank,
            direction,
            normalized_score,
            normalized_score * weight AS weighted_score
        FROM scored
        WHERE normalized_score IS NOT NULL
    ),
    deduped AS (
        SELECT *
        FROM (
            SELECT
                w.*,
                row_number() OVER (
                    PARTITION BY w.source_name, w.ctid
                    ORDER BY
                        w.weighted_score DESC,
                        w.source_rank,
                        CASE
                            WHEN w.direction = 'lower_is_better'
                            THEN w.raw_value
                        END ASC NULLS LAST,
                        CASE
                            WHEN w.direction = 'higher_is_better'
                            THEN w.raw_value
                        END DESC NULLS LAST
                ) AS duplicate_rank
            FROM weighted w
        ) d
        WHERE duplicate_rank = 1
    ),
    fused AS (
        SELECT
            ctid,
            sum(weighted_score) AS score,
            count(*)::int4 AS source_count,
            array_agg(source_name ORDER BY source_name) AS source_names,
            array_agg(raw_value::real ORDER BY source_name) AS raw_values,
            array_agg(normalized_score::real ORDER BY source_name)
                AS normalized_scores,
            array_agg(weighted_score::real ORDER BY source_name)
                AS weighted_scores,
            array_agg(source_rank ORDER BY source_name) AS ranks
        FROM deduped
        GROUP BY ctid
    )
    SELECT
        ctid,
        score::real,
        source_count,
        source_names,
        raw_values,
        normalized_scores,
        weighted_scores,
        ranks
    FROM fused
    ORDER BY score DESC, ctid::text
    LIMIT GREATEST(COALESCE($2, 10), 0)
$$;

CREATE FUNCTION psql_bm25s_hybrid_fuse_candidates(
    candidates psql_bm25s_hybrid_candidate[],
    k int4 DEFAULT 10,
    fusion text DEFAULT 'rrf',
    rrf_k real DEFAULT 60.0,
    epsilon real DEFAULT 0.000001
)
RETURNS SETOF psql_bm25s_hybrid_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_hybrid_fuse_candidates'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_match_prepared_query(
    doc_tokens text[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query_op(
        $1,
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_prepared_query(
    doc_tokens varchar[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query_op(
        $1,
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_prepared_query(
    doc_text text,
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        psql_bm25s_tokenize_text(
            $1,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ),
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_prepared_query(
    doc_text varchar,
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        psql_bm25s_tokenize_text(
            $1::text,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ),
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_query(
    doc_tokens text[],
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS boolean
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_match_query(
    doc_tokens varchar[],
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS boolean
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_match_query(
    doc_text text,
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS boolean
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_match_query(
    doc_text varchar,
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS boolean
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_match_prepared_query_op(
    doc_tokens text[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
AS 'MODULE_PATHNAME', 'psql_bm25s_match_prepared_query_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_match_prepared_query_op(
    doc_tokens varchar[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
AS 'MODULE_PATHNAME', 'psql_bm25s_match_prepared_query_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_match_prepared_query_scalar_op(
    doc_text text,
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        psql_bm25s_tokenize_text(
            $1,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ),
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_prepared_query_scalar_op(
    doc_text varchar,
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_match_prepared_query(
        psql_bm25s_tokenize_text(
            $1::text,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ),
        $2
    )
$$;

CREATE OPERATOR @@@ (
    LEFTARG = text[],
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_op
);

CREATE OPERATOR @@@ (
    LEFTARG = varchar[],
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_op
);

CREATE OPERATOR @@@ (
    LEFTARG = text,
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_scalar_op
);

CREATE OPERATOR @@@ (
    LEFTARG = varchar,
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_scalar_op
);

ALTER OPERATOR FAMILY psql_bm25s_text_array_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (text[], psql_bm25s_prepared_query);

ALTER OPERATOR FAMILY psql_bm25s_varchar_array_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (varchar[], psql_bm25s_prepared_query);

ALTER OPERATOR FAMILY psql_bm25s_text_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (text, psql_bm25s_prepared_query);

ALTER OPERATOR FAMILY psql_bm25s_varchar_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (varchar, psql_bm25s_prepared_query);

CREATE FUNCTION psql_bm25s_score_prepared_query(
    doc_tokens text[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH query_tokens AS (
        SELECT psql_bm25s_tokenize_text(
            ($2).query_text,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ) AS tokens
    )
    SELECT CASE
        WHEN COALESCE(array_ndims($1), 0) <> 1 THEN 0::float8
        WHEN COALESCE(array_ndims(tokens), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_score_tokens_op($1, tokens)
    END
    FROM query_tokens
$$;

CREATE FUNCTION psql_bm25s_score_prepared_query(
    doc_tokens varchar[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH query_tokens AS (
        SELECT psql_bm25s_tokenize_text(
            ($2).query_text,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ) AS tokens
    )
    SELECT CASE
        WHEN COALESCE(array_ndims($1), 0) <> 1 THEN 0::float8
        WHEN COALESCE(array_ndims(tokens), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_score_tokens_op($1, tokens)
    END
    FROM query_tokens
$$;

CREATE FUNCTION psql_bm25s_score_prepared_query(
    doc_text text,
    prepared_query psql_bm25s_prepared_query
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_score_prepared_query(
        psql_bm25s_tokenize_text(
            $1,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ),
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_score_prepared_query(
    doc_text varchar,
    prepared_query psql_bm25s_prepared_query
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_score_prepared_query(
        psql_bm25s_tokenize_text(
            $1::text,
            ($2).lowercase,
            ($2).stopwords,
            ($2).stem_english,
            ($2).fold_diacritics
        ),
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_score_query(
    doc_tokens text[],
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS float8
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_score_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_score_query(
    doc_tokens varchar[],
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS float8
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_score_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_score_query(
    doc_text text,
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS float8
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_score_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_score_query(
    doc_text varchar,
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS float8
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_score_prepared_query(
        $1,
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE FUNCTION psql_bm25s_normalize_tokens(
    tokens text[],
    lowercase boolean DEFAULT true,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text[]
AS 'MODULE_PATHNAME', 'psql_bm25s_normalize_tokens_sql'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_normalize_tokens(
    tokens varchar[],
    lowercase boolean DEFAULT true,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text[]
AS 'MODULE_PATHNAME', 'psql_bm25s_normalize_tokens_sql'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_tokenize_text(
    input_text text,
    lowercase boolean DEFAULT true,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text[]
AS 'MODULE_PATHNAME', 'psql_bm25s_tokenize_text_sql'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_highlight_tokens(
    doc_tokens text[],
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_highlight_tokens'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_highlight_tokens(
    doc_tokens varchar[],
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_highlight_tokens'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_highlight_tokens(
    doc_text text,
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_highlight_tokens(
        psql_bm25s_tokenize_text($1, false, NULL, false, false),
        $2,
        $3,
        $4
    )
$$;

CREATE FUNCTION psql_bm25s_highlight_tokens(
    doc_text varchar,
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_highlight_tokens(
        psql_bm25s_tokenize_text($1::text, false, NULL, false, false),
        $2,
        $3,
        $4
    )
$$;

CREATE FUNCTION psql_bm25s_highlight_tokens_cfg(
    doc_tokens text[],
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_highlight_tokens_cfg'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_highlight_tokens_cfg(
    doc_tokens varchar[],
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_highlight_tokens_cfg'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_highlight_tokens_cfg(
    doc_text text,
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_highlight_tokens_cfg(
        psql_bm25s_tokenize_text($1, $5, $6, $7, $8),
        $2,
        $3,
        $4,
        $5,
        $6,
        $7,
        $8
    )
$$;

CREATE FUNCTION psql_bm25s_highlight_tokens_cfg(
    doc_text varchar,
    query_text text,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_highlight_tokens_cfg(
        psql_bm25s_tokenize_text($1::text, $5, $6, $7, $8),
        $2,
        $3,
        $4,
        $5,
        $6,
        $7,
        $8
    )
$$;

CREATE FUNCTION psql_bm25s_snippet_tokens(
    doc_tokens text[],
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_snippet_tokens'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_snippet_tokens(
    doc_tokens varchar[],
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_snippet_tokens'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_snippet_tokens(
    doc_text text,
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_snippet_tokens(
        psql_bm25s_tokenize_text($1, false, NULL, false, false),
        $2,
        $3,
        $4,
        $5
    )
$$;

CREATE FUNCTION psql_bm25s_snippet_tokens(
    doc_text varchar,
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>'
)
RETURNS text
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_snippet_tokens(
        psql_bm25s_tokenize_text($1::text, false, NULL, false, false),
        $2,
        $3,
        $4,
        $5
    )
$$;

CREATE FUNCTION psql_bm25s_snippet_tokens_cfg(
    doc_tokens text[],
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_snippet_tokens_cfg'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_snippet_tokens_cfg(
    doc_text text,
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_snippet_tokens_cfg(
        psql_bm25s_tokenize_text($1, $6, $7, $8, $9),
        $2,
        $3,
        $4,
        $5,
        $6,
        $7,
        $8,
        $9
    )
$$;

CREATE FUNCTION psql_bm25s_snippet_tokens_cfg(
    doc_text varchar,
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_snippet_tokens_cfg(
        psql_bm25s_tokenize_text($1::text, $6, $7, $8, $9),
        $2,
        $3,
        $4,
        $5,
        $6,
        $7,
        $8,
        $9
    )
$$;
CREATE FUNCTION psql_bm25s_snippet_tokens_cfg(
    doc_tokens varchar[],
    query_text text,
    max_tokens int4 DEFAULT 24,
    start_tag text DEFAULT '<b>',
    end_tag text DEFAULT '</b>',
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_snippet_tokens_cfg'
LANGUAGE C IMMUTABLE PARALLEL SAFE;
