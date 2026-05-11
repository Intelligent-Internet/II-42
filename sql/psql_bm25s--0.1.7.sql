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
AS $$
    WITH idx AS (
        SELECT
            am.amname AS access_method,
            a.atttypid::regtype::text AS doc_type
        FROM pg_class idx
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
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', false,
                'supports_filtered_ranked', false,
                'canonical_api',
                    'psql_bm25s_search_query(index_name, query_text, ...)',
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Scalar text and varchar indexes are queried through the regclass helper APIs.',
                    'No @@, @@@, or <=> operators are defined for scalar text and varchar indexes.',
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
BEGIN
    EXECUTE 'EXPLAIN (FORMAT JSON) ' || sql_text
        INTO explain_row;
    RETURN psql_bm25s_plan_fast_path(
        index_name,
        explain_row::jsonb
    );
END;
$$;

CREATE FUNCTION psql_bm25s_refresh_index(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_refresh_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

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
    STORAGE text;

CREATE OPERATOR CLASS psql_bm25s_varchar_ops
DEFAULT FOR TYPE varchar USING psql_bm25s AS
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_prepared_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
AS $$
    SELECT ROW($1, $2, $3, $4, $5, $6)::psql_bm25s_prepared_query
$$;

CREATE FUNCTION psql_bm25s_query(
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_prepared_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
AS $$
    SELECT psql_bm25s_prepare_query($1, $2, $3, $4, $5, $6)
$$;

CREATE FUNCTION psql_bm25s_ranked_query(
    prepared_query psql_bm25s_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS psql_bm25s_ranked_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_ranked_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
AS $$
    SELECT ROW($1, COALESCE($2, 1.0))::psql_bm25s_weighted_query
$$;

CREATE FUNCTION psql_bm25s_weighted_query(
    index_name regclass,
    query_text text,
    weight real DEFAULT 1.0,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_weighted_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_weighted_query[]
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_field_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS psql_bm25s_field_query[]
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
AS $$
    SELECT ($1).prepared_query
$$;

CREATE FUNCTION psql_bm25s_order_tokens(
    prepared_query psql_bm25s_prepared_query
)
RETURNS text[]
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
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
AS $$
    SELECT ($1).order_tokens
$$;

CREATE FUNCTION psql_bm25s_order_tokens(
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text[]
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
AS $$
    SELECT psql_bm25s_order_tokens(
        psql_bm25s_query($1, $2, $3, $4, $5, $6)
    )
$$;






CREATE FUNCTION psql_bm25s_match_prepared_query(
    doc_tokens text[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
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
AS $$
    SELECT psql_bm25s_match_prepared_query_op(
        $1,
        $2
    )
$$;

CREATE FUNCTION psql_bm25s_match_query(
    doc_tokens text[],
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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

ALTER OPERATOR FAMILY psql_bm25s_text_array_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (text[], psql_bm25s_prepared_query);

ALTER OPERATOR FAMILY psql_bm25s_varchar_array_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (varchar[], psql_bm25s_prepared_query);

CREATE FUNCTION psql_bm25s_score_prepared_query(
    doc_tokens text[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
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

CREATE FUNCTION psql_bm25s_score_query(
    doc_tokens text[],
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS float8
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS float8
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
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
