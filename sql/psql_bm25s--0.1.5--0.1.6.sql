-- Add varchar[] support alongside existing text[] token-array APIs.

CREATE OR REPLACE FUNCTION psql_bm25s_fast_path_advice(index_name regclass)
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

CREATE FUNCTION psql_bm25s_match_query_tokens_op(
    doc_tokens varchar[],
    query_text text
)
RETURNS boolean
AS 'MODULE_PATHNAME', 'psql_bm25s_match_query_tokens_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

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
    LEFTARG = varchar[],
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_tokens_op
);

CREATE OPERATOR CLASS psql_bm25s_varchar_array_ops
DEFAULT FOR TYPE varchar[] USING psql_bm25s AS
    OPERATOR 1 @@ (varchar[], text),
    OPERATOR 1 <=> (varchar[], text[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE varchar[];

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
    doc_tokens varchar[],
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
AS 'MODULE_PATHNAME', 'psql_bm25s_match_prepared_query_op'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR @@@ (
    LEFTARG = varchar[],
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_op
);

ALTER OPERATOR FAMILY psql_bm25s_varchar_array_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (varchar[], psql_bm25s_prepared_query);

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
    tokens varchar[],
    lowercase boolean DEFAULT true,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS text[]
AS 'MODULE_PATHNAME', 'psql_bm25s_normalize_tokens_sql'
LANGUAGE C IMMUTABLE PARALLEL SAFE;

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
