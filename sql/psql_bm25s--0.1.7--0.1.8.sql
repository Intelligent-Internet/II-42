CREATE OR REPLACE FUNCTION psql_bm25s_fast_path_advice(index_name regclass)
RETURNS jsonb
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    WITH idx AS (
        SELECT c.oid AS index_oid,
               c.relname AS index_name,
               am.amname AS access_method,
               format_type(opc.opcintype, NULL) AS doc_type
        FROM pg_class c
        JOIN pg_index i ON i.indexrelid = c.oid
        JOIN pg_am am ON am.oid = c.relam
        JOIN pg_opclass opc ON opc.oid = i.indclass[0]
        WHERE c.oid = $1
    )
    SELECT CASE
        WHEN access_method IS DISTINCT FROM 'psql_bm25s' THEN
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
                    'For scalar text and varchar indexes, prefer explicit psql_bm25s_query(...) or psql_bm25s_order_tokens(...) options when they differ from the index defaults.',
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
                    'Use the ids canonical API for exact benchmark-aligned retrieval.'
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
                    'Unknown psql_bm25s opclass input type.'
                ]::text[])
            )
    END
    FROM idx
$$;

CREATE FUNCTION psql_bm25s_score_scalar_text_op(
    doc_text text,
    query_tokens text[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
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
AS $$
    SELECT psql_bm25s_match_query_tokens_op(
        psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
        $2
    )
$$;

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
    LEFTARG = text,
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_scalar_op
);

CREATE OPERATOR @@ (
    LEFTARG = varchar,
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_scalar_op
);

ALTER OPERATOR FAMILY psql_bm25s_text_ops USING psql_bm25s
    ADD OPERATOR 1 @@ (text, text);

ALTER OPERATOR FAMILY psql_bm25s_text_ops USING psql_bm25s
    ADD OPERATOR 1 <=> (text, text[]) FOR ORDER BY pg_catalog.float_ops;

ALTER OPERATOR FAMILY psql_bm25s_varchar_ops USING psql_bm25s
    ADD OPERATOR 1 @@ (varchar, text);

ALTER OPERATOR FAMILY psql_bm25s_varchar_ops USING psql_bm25s
    ADD OPERATOR 1 <=> (varchar, text[]) FOR ORDER BY pg_catalog.float_ops;

CREATE FUNCTION psql_bm25s_match_prepared_query_scalar_op(
    doc_text text,
    prepared_query psql_bm25s_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
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
    LEFTARG = text,
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_scalar_op
);

CREATE OPERATOR @@@ (
    LEFTARG = varchar,
    RIGHTARG = psql_bm25s_prepared_query,
    PROCEDURE = psql_bm25s_match_prepared_query_scalar_op
);

ALTER OPERATOR FAMILY psql_bm25s_text_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (text, psql_bm25s_prepared_query);

ALTER OPERATOR FAMILY psql_bm25s_varchar_ops USING psql_bm25s
    ADD OPERATOR 1 @@@ (varchar, psql_bm25s_prepared_query);
