-- 0.4.0 applies the public SQL API cleanup for existing 0.3.2 installs.
-- Some development installs already carried these definitions under 0.3.2,
-- so rename steps are guarded and CREATE steps are idempotent.

DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_hit') IS NULL AND
       to_regtype('psql_bm25s_hit') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_hit RENAME TO psql_bm25s_result_hit;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_prepared_query') IS NULL AND
       to_regtype('psql_bm25s_prepared_query') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_prepared_query RENAME TO psql_bm25s_result_prepared_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_ranked_query') IS NULL AND
       to_regtype('psql_bm25s_ranked_query') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_ranked_query RENAME TO psql_bm25s_result_ranked_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_fusion_weighted_query') IS NULL AND
       to_regtype('psql_bm25s_weighted_query') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_weighted_query RENAME TO psql_bm25s_result_fusion_weighted_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_fusion_field_query') IS NULL AND
       to_regtype('psql_bm25s_field_query') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_field_query RENAME TO psql_bm25s_result_fusion_field_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_hybrid_candidate') IS NULL AND
       to_regtype('psql_bm25s_hybrid_candidate') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_hybrid_candidate RENAME TO psql_bm25s_result_hybrid_candidate;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regtype('psql_bm25s_result_hybrid_hit') IS NULL AND
       to_regtype('psql_bm25s_hybrid_hit') IS NOT NULL THEN
        ALTER TYPE psql_bm25s_hybrid_hit RENAME TO psql_bm25s_result_hybrid_hit;
    END IF;
END;
$$;

DROP FUNCTION IF EXISTS psql_bm25s_search_fields(
    text[],
    regclass[],
    text,
    real[],
    int4,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_search_fields(
    regclass[],
    text,
    real[],
    int4,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_search_field_queries(
    psql_bm25s_result_fusion_field_query[],
    int4,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_weighted_queries(
    psql_bm25s_result_fusion_weighted_query[],
    int4,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_weighted_queries(
    psql_bm25s_result_fusion_weighted_query[],
    text,
    int4,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_weighted_query(
    psql_bm25s_result_prepared_query,
    real
);

DROP FUNCTION IF EXISTS psql_bm25s_weighted_query(
    regclass,
    text,
    real,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_weighted_queries(
    regclass[],
    text,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_field_query(
    text,
    psql_bm25s_result_fusion_weighted_query
);

DROP FUNCTION IF EXISTS psql_bm25s_field_query(
    text,
    psql_bm25s_result_prepared_query,
    real
);

DROP FUNCTION IF EXISTS psql_bm25s_field_query(
    text,
    regclass,
    text,
    real,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_field_queries(
    text[],
    regclass[],
    text,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_fuse(
    psql_bm25s_result_hit[],
    real,
    psql_bm25s_result_hit[],
    real,
    int4
);

DROP FUNCTION IF EXISTS psql_bm25s_fuse(
    psql_bm25s_result_hit[],
    int4
);

DROP FUNCTION IF EXISTS psql_bm25s_fusion_hits(
    psql_bm25s_result_hit[],
    real,
    psql_bm25s_result_hit[],
    real,
    int4
);

DROP FUNCTION IF EXISTS psql_bm25s_fusion_hits(
    psql_bm25s_result_hit[],
    int4
);

DROP FUNCTION IF EXISTS psql_bm25s_search_indexes(
    text[],
    regclass[],
    text,
    real[],
    int4,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_search_indexes(
    regclass[],
    text,
    real[],
    int4,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DO $$
BEGIN
    IF to_regtype('psql_bm25s_result') IS NOT NULL THEN
        EXECUTE $drop$
            DROP FUNCTION IF EXISTS psql_bm25s_fuse_results(
                psql_bm25s_result,
                real,
                psql_bm25s_result,
                real,
                int4
            )
        $drop$;
    END IF;
END;
$$;

DROP FUNCTION IF EXISTS psql_bm25s_search_result(
    psql_bm25s_result_prepared_query,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_result(psql_bm25s_result_ranked_query);

DROP FUNCTION IF EXISTS psql_bm25s_search_result(
    regclass,
    text,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_search(
    psql_bm25s_result_prepared_query,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search(psql_bm25s_result_ranked_query);

DROP FUNCTION IF EXISTS psql_bm25s_search(
    regclass,
    text,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_hits(
    psql_bm25s_result_prepared_query,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_hits(psql_bm25s_result_ranked_query);

DROP FUNCTION IF EXISTS psql_bm25s_hits(
    regclass,
    text,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_query_hits(
    regclass,
    text,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_search_prepared_query_result(
    psql_bm25s_result_prepared_query,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_ids_result(
    regclass,
    int4[],
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_tokens_result(
    regclass,
    text[],
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_query_result(
    regclass,
    text,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_query_result_cfg(
    regclass,
    text,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DROP TYPE IF EXISTS psql_bm25s_result CASCADE;

DROP FUNCTION IF EXISTS psql_bm25s_match_query_tokens(
    text[],
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_match_query_tokens(
    varchar[],
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_score_query_tokens(
    text[],
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_score_query_tokens(
    varchar[],
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_search_query(
    regclass,
    text,
    int4,
    real[]
);

DROP FUNCTION IF EXISTS psql_bm25s_search_query_cfg(
    regclass,
    text,
    int4,
    real[],
    boolean,
    text[],
    boolean,
    boolean
);

DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_query_ids(regclass, int4[], int4, real[])') IS NULL AND
       to_regprocedure('psql_bm25s_search_ids(regclass, int4[], int4, real[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_search_ids(regclass, int4[], int4, real[]) RENAME TO psql_bm25s_query_ids;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_query_tokens(regclass, text[], int4, real[])') IS NULL AND
       to_regprocedure('psql_bm25s_search_tokens(regclass, text[], int4, real[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_search_tokens(regclass, text[], int4, real[]) RENAME TO psql_bm25s_query_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_prepared_query(regclass, text, boolean, text[], boolean, boolean)') IS NULL AND
       to_regprocedure('psql_bm25s_query(regclass, text, boolean, text[], boolean, boolean)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_query(regclass, text, boolean, text[], boolean, boolean) RENAME TO psql_bm25s_prepared_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_query_prepared(psql_bm25s_result_prepared_query, int4, real[])') IS NULL AND
       to_regprocedure('psql_bm25s_search_prepared_query(psql_bm25s_result_prepared_query, int4, real[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_search_prepared_query(psql_bm25s_result_prepared_query, int4, real[]) RENAME TO psql_bm25s_query_prepared;
    END IF;
END;
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_query(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_search_query_cfg'
LANGUAGE C STABLE PARALLEL SAFE;

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens(
    text[],
    text,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens(
    varchar[],
    text,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens(
    text,
    text,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens(
    varchar,
    text,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens_cfg(
    text[],
    text,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens_cfg(
    varchar[],
    text,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens_cfg(
    text,
    text,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_highlight_tokens_cfg(
    varchar,
    text,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

CREATE OR REPLACE FUNCTION psql_bm25s_highlight(
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

CREATE OR REPLACE FUNCTION psql_bm25s_highlight(
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

CREATE OR REPLACE FUNCTION psql_bm25s_highlight(
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
    SELECT psql_bm25s_highlight(
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

CREATE OR REPLACE FUNCTION psql_bm25s_highlight(
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
    SELECT psql_bm25s_highlight(
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

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens(
    text[],
    text,
    int4,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens(
    varchar[],
    text,
    int4,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens(
    text,
    text,
    int4,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens(
    varchar,
    text,
    int4,
    text,
    text
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens_cfg(
    text[],
    text,
    int4,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens_cfg(
    varchar[],
    text,
    int4,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens_cfg(
    text,
    text,
    int4,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

DROP FUNCTION IF EXISTS psql_bm25s_snippet_tokens_cfg(
    varchar,
    text,
    int4,
    text,
    text,
    boolean,
    text[],
    boolean,
    boolean
);

CREATE OR REPLACE FUNCTION psql_bm25s_snippet(
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

CREATE OR REPLACE FUNCTION psql_bm25s_snippet(
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

CREATE OR REPLACE FUNCTION psql_bm25s_snippet(
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
    SELECT psql_bm25s_snippet(
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

CREATE OR REPLACE FUNCTION psql_bm25s_snippet(
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
    SELECT psql_bm25s_snippet(
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

DROP FUNCTION IF EXISTS psql_bm25s_hybrid_fuse_candidates_sql_reference(
    psql_bm25s_result_hybrid_candidate[],
    int4,
    text,
    real,
    real
);

DROP FUNCTION IF EXISTS psql_bm25s_hybrid_check_fusion(text);
DROP FUNCTION IF EXISTS psql_bm25s_hybrid_check_normalizer(text);
DROP FUNCTION IF EXISTS psql_bm25s_hybrid_check_direction(text);

DROP CAST IF EXISTS (psql_bm25s_index AS bytea);
DROP CAST IF EXISTS (bytea AS psql_bm25s_index);

DROP FUNCTION IF EXISTS psql_bm25s_describe(psql_bm25s_index);
DROP FUNCTION IF EXISTS psql_bm25s_index_value(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_to_bytea(psql_bm25s_index);
DROP FUNCTION IF EXISTS psql_bm25s_from_bytea(bytea);
DROP FUNCTION IF EXISTS psql_bm25s_build_ids(
    jsonb,
    text,
    text,
    real,
    real,
    real,
    boolean
);
DROP FUNCTION IF EXISTS psql_bm25s_build_tokens(
    jsonb,
    text,
    text,
    real,
    real,
    real
);
DROP FUNCTION IF EXISTS psql_bm25s_num_docs(psql_bm25s_index);
DROP FUNCTION IF EXISTS psql_bm25s_vocab_size(psql_bm25s_index);
DROP FUNCTION IF EXISTS psql_bm25s_topk_ids(
    psql_bm25s_index,
    int4[],
    int4,
    boolean,
    real[]
);
DROP FUNCTION IF EXISTS psql_bm25s_topk_tokens(
    psql_bm25s_index,
    text[],
    int4,
    boolean,
    real[]
);

CREATE OR REPLACE FUNCTION psql_bm25s_fast_path_advice(index_name regclass)
RETURNS jsonb
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH idx AS (
        SELECT
            am.amname AS access_method,
            i.indnatts AS indnatts,
            a.atttypid::regtype::text AS doc_type,
            EXISTS (
                SELECT 1
                FROM unnest(coalesce(idx.reloptions, ARRAY[]::text[])) AS opt
                WHERE opt = 'field_aware=true'
            ) AS field_aware
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
            AND field_aware
            AND doc_type IN (
                'text[]',
                'character varying[]',
                'varchar[]',
                'text',
                'character varying',
                'varchar'
            ) THEN
            jsonb_build_object(
                'index_name', $1::text,
                'access_method', access_method,
                'doc_type', doc_type,
                'supports_plain_match', false,
                'supports_prepared_match', false,
                'supports_ordering', false,
                'supports_filtered_ranked', false,
                'canonical_api',
                    'psql_bm25s_field_aware_query_tokens(index_name, query_tokens, field_names, weights, ...)',
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Field-aware indexes use field-scoped internal tokens.',
                    'Generic token and simple raw-query APIs search all fields with equal weight.',
                    'Use psql_bm25s_field_aware_query_tokens(...) or psql_bm25s_field_aware_query(...) for custom field weights.'
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
                    'psql_bm25s_query_tokens(index_name, query_tokens, ...)',
                'recommended_filter', NULL,
                'recommended_order', NULL,
                'notes', to_jsonb(ARRAY[
                    'Multicolumn fusion indexes use direct regclass retrieval APIs.',
                    '@@, @@@, and <=> are not exposed for multicolumn fusion indexes.',
                    'Use psql_bm25s_query_tokens(...) or psql_bm25s_query(...) for fused retrieval.'
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
                    'psql_bm25s_query(index_name, query_text, ...)',
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
                    'psql_bm25s_query(index_name, query_text, ...)',
                'recommended_filter',
                    'tokens @@@ psql_bm25s_prepared_query(index_name, query_text, ...)',
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
                    'psql_bm25s_query(index_name, query_text, ...)',
                'recommended_filter',
                    'column @@@ psql_bm25s_prepared_query(index_name, query_text, ...)',
                'recommended_order',
                    'ORDER BY column <=> psql_bm25s_order_tokens(index_name, query_text, ...) ASC LIMIT k',
                'notes', to_jsonb(ARRAY[
                    '@@ is available for plain-text filtering on scalar text and varchar indexes.',
                    '@@@ is the structured prepared-query predicate and is preferred when text options matter.',
                    '<=> aligns to BM25 ordering only when PostgreSQL uses a real psql_bm25s index scan.',
                    'Outside a real index scan, scalar text and varchar operators use explicit query options, not hidden index reloptions.',
                    'psql_bm25s_prepared_query(index_name, ...) resolves omitted scalar text options from the named index reloptions.',
                    'When text options matter outside index scans, prefer psql_bm25s_match_query(...), psql_bm25s_score_query(...), or explicit psql_bm25s_prepared_query(...) and psql_bm25s_order_tokens(...).',
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
                    'psql_bm25s_query_ids(index_name, query_ids, ...)',
                'recommended_filter', NULL,
                'recommended_order',
                    'ORDER BY token_ids <=> query_ids ASC LIMIT k',
                'notes', to_jsonb(ARRAY[
                    'int4[] indexes only expose ordered retrieval through <=>.',
                    'No @@ or @@@ predicate operators are defined for int4[] indexes.',
                    'Use the ids rowset API for the clearest exact BM25 contract.'
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

CREATE OR REPLACE FUNCTION psql_bm25s_field_aware_query_tokens(
    index_name regclass,
    query_tokens text[],
    field_names text[],
    weights real[] DEFAULT NULL,
    k int4 DEFAULT 10
)
RETURNS SETOF psql_bm25s_result_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_field_aware_query_tokens'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_field_aware_query(
    index_name regclass,
    query_text text,
    field_names text[],
    weights real[] DEFAULT NULL,
    k int4 DEFAULT 10
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_field_aware_query_tokens(
        $1,
        psql_bm25s_order_tokens($1, $2),
        $3,
        $4,
        $5
    )
$$;

DROP FUNCTION IF EXISTS psql_bm25s_plan_fast_path(regclass, jsonb);
DROP FUNCTION IF EXISTS psql_bm25s_explain_fast_path(regclass, text);

CREATE OR REPLACE FUNCTION psql_bm25s_fast_path_plan(
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

CREATE OR REPLACE FUNCTION psql_bm25s_fast_path_explain(
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
        'SELECT %I.psql_bm25s_fast_path_plan($1, $2)',
        extension_schema
    )
    INTO result
    USING index_name, explain_row::jsonb;
    RETURN result;
END;
$$;

DROP FUNCTION IF EXISTS psql_bm25s_recommend_maintenance_policy(regclass, text);
DROP FUNCTION IF EXISTS psql_bm25s_index_describe(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_index_maintenance_state(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_index_maintenance_policy(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_index_maintenance_policy_details(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_index_maintenance_policy_recommend(regclass, text);
DROP FUNCTION IF EXISTS psql_bm25s_index_details(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_refresh_index(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_maintain_index(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_try_maintain_index(regclass);
DROP FUNCTION IF EXISTS psql_bm25s_maintain_due_indexes(integer);

CREATE OR REPLACE FUNCTION psql_bm25s_index_details(index_name regclass)
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

CREATE OR REPLACE FUNCTION psql_bm25s_index_policy_recommend(
    index_name regclass,
    profile text DEFAULT 'balanced'
)
RETURNS TABLE(
    index_name regclass,
    profile text,
    confidence text,
    recommended_options text,
    recommended_consistency text,
    recommended_auto_rebuild_threshold int4,
    recommended_auto_rebuild_delta_bytes int4,
    recommended_auto_rebuild_churn_ratio float8,
    matches_current bool,
    refresh_now bool,
    docs int8,
    pending_total int8,
    reason text
)
AS 'MODULE_PATHNAME', 'psql_bm25s_index_policy_recommend'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_index_refresh(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_refresh_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_index_maintain(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_maintain_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_index_try_maintain(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_try_maintain_index'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_index_maintain_due(max_indexes integer DEFAULT 1)
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

DROP FUNCTION IF EXISTS psql_bm25s_prepare_query(regclass, text, boolean, text[], boolean, boolean);

DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_ids(int4[], int4[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_ids_op(int4[], int4[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_ids_op(int4[], int4[]) RENAME TO psql_bm25s_op_score_ids;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_tokens(text[], text[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_tokens_op(text[], text[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_tokens_op(text[], text[]) RENAME TO psql_bm25s_op_score_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_tokens(text[], varchar[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_tokens_op(text[], varchar[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_tokens_op(text[], varchar[]) RENAME TO psql_bm25s_op_score_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_tokens(varchar[], text[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_tokens_op(varchar[], text[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_tokens_op(varchar[], text[]) RENAME TO psql_bm25s_op_score_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_tokens(varchar[], varchar[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_tokens_op(varchar[], varchar[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_tokens_op(varchar[], varchar[]) RENAME TO psql_bm25s_op_score_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_query_tokens(text[], text)') IS NULL AND
       to_regprocedure('psql_bm25s_match_query_tokens_op(text[], text)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_query_tokens_op(text[], text) RENAME TO psql_bm25s_op_match_query_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_query_tokens(varchar[], text)') IS NULL AND
       to_regprocedure('psql_bm25s_match_query_tokens_op(varchar[], text)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_query_tokens_op(varchar[], text) RENAME TO psql_bm25s_op_match_query_tokens;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_scalar_text(text, text[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_scalar_text_op(text, text[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_scalar_text_op(text, text[]) RENAME TO psql_bm25s_op_score_scalar_text;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_scalar_text(text, varchar[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_scalar_text_op(text, varchar[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_scalar_text_op(text, varchar[]) RENAME TO psql_bm25s_op_score_scalar_text;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_scalar_text(varchar, text[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_scalar_text_op(varchar, text[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_scalar_text_op(varchar, text[]) RENAME TO psql_bm25s_op_score_scalar_text;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_score_scalar_text(varchar, varchar[])') IS NULL AND
       to_regprocedure('psql_bm25s_score_scalar_text_op(varchar, varchar[])') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_score_scalar_text_op(varchar, varchar[]) RENAME TO psql_bm25s_op_score_scalar_text;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_query_scalar(text, text)') IS NULL AND
       to_regprocedure('psql_bm25s_match_query_scalar_op(text, text)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_query_scalar_op(text, text) RENAME TO psql_bm25s_op_match_query_scalar;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_query_scalar(varchar, text)') IS NULL AND
       to_regprocedure('psql_bm25s_match_query_scalar_op(varchar, text)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_query_scalar_op(varchar, text) RENAME TO psql_bm25s_op_match_query_scalar;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_prepared_query(text[], psql_bm25s_result_prepared_query)') IS NULL AND
       to_regprocedure('psql_bm25s_match_prepared_query_op(text[], psql_bm25s_result_prepared_query)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_prepared_query_op(text[], psql_bm25s_result_prepared_query) RENAME TO psql_bm25s_op_match_prepared_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_prepared_query(varchar[], psql_bm25s_result_prepared_query)') IS NULL AND
       to_regprocedure('psql_bm25s_match_prepared_query_op(varchar[], psql_bm25s_result_prepared_query)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_prepared_query_op(varchar[], psql_bm25s_result_prepared_query) RENAME TO psql_bm25s_op_match_prepared_query;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_prepared_query_scalar(text, psql_bm25s_result_prepared_query)') IS NULL AND
       to_regprocedure('psql_bm25s_match_prepared_query_scalar_op(text, psql_bm25s_result_prepared_query)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_prepared_query_scalar_op(text, psql_bm25s_result_prepared_query) RENAME TO psql_bm25s_op_match_prepared_query_scalar;
    END IF;
END;
$$;
DO $$
BEGIN
    IF to_regprocedure('psql_bm25s_op_match_prepared_query_scalar(varchar, psql_bm25s_result_prepared_query)') IS NULL AND
       to_regprocedure('psql_bm25s_match_prepared_query_scalar_op(varchar, psql_bm25s_result_prepared_query)') IS NOT NULL THEN
        ALTER FUNCTION psql_bm25s_match_prepared_query_scalar_op(varchar, psql_bm25s_result_prepared_query) RENAME TO psql_bm25s_op_match_prepared_query_scalar;
    END IF;
END;
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_op_score_scalar_text(
    doc_text text,
    query_tokens text[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_op_score_tokens(
            psql_bm25s_tokenize_text($1, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_op_score_scalar_text(
    doc_text text,
    query_tokens varchar[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_op_score_tokens(
            psql_bm25s_tokenize_text($1, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_op_score_scalar_text(
    doc_text varchar,
    query_tokens text[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_op_score_tokens(
            psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_op_score_scalar_text(
    doc_text varchar,
    query_tokens varchar[]
)
RETURNS float8
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT CASE
        WHEN COALESCE(array_ndims($2), 0) <> 1 THEN 0::float8
        ELSE -psql_bm25s_op_score_tokens(
            psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
            $2
        )
    END
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_op_match_query_scalar(
    doc_text text,
    query_text text
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_op_match_query_tokens(
        psql_bm25s_tokenize_text($1, true, NULL, false, false),
        $2
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_op_match_query_scalar(
    doc_text varchar,
    query_text text
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_op_match_query_tokens(
        psql_bm25s_tokenize_text($1::text, true, NULL, false, false),
        $2
    )
$$;

-- Refresh result-type SQL wrappers after type renames.

CREATE OR REPLACE FUNCTION psql_bm25s_prepared_query(
    index_name regclass,
    query_text text,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_result_prepared_query
AS 'MODULE_PATHNAME', 'psql_bm25s_prepare_query_resolved'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_ranked_query(
    prepared_query psql_bm25s_result_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS psql_bm25s_result_ranked_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW(
        $1,
        psql_bm25s_order_tokens($1),
        $2,
        $3
    )::psql_bm25s_result_ranked_query
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_ranked_query(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_result_ranked_query
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_ranked_query(
        psql_bm25s_prepared_query($1, $2, $5, $6, $7, $8),
        $3,
        $4
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_weighted_query(
    prepared_query psql_bm25s_result_prepared_query,
    weight real DEFAULT 1.0
)
RETURNS psql_bm25s_result_fusion_weighted_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW($1, COALESCE($2, 1.0))::psql_bm25s_result_fusion_weighted_query
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_weighted_query(
    index_name regclass,
    query_text text,
    weight real DEFAULT 1.0,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_result_fusion_weighted_query
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_fusion_weighted_query(
        psql_bm25s_prepared_query($1, $2, $4, $5, $6, $7),
        $3
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_weighted_queries(
    index_names regclass[],
    query_text text,
    weights real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_result_fusion_weighted_query[]
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT COALESCE(
        array_agg(
            psql_bm25s_fusion_weighted_query(
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
        ARRAY[]::psql_bm25s_result_fusion_weighted_query[]
    )
    FROM generate_subscripts(COALESCE(index_names, ARRAY[]::regclass[]), 1) AS i
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_field_query(
    field_name text,
    weighted_query psql_bm25s_result_fusion_weighted_query
)
RETURNS psql_bm25s_result_fusion_field_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ROW($1, $2)::psql_bm25s_result_fusion_field_query
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_field_query(
    field_name text,
    prepared_query psql_bm25s_result_prepared_query,
    weight real DEFAULT 1.0
)
RETURNS psql_bm25s_result_fusion_field_query
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_fusion_field_query(
        $1,
        psql_bm25s_fusion_weighted_query($2, $3)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_field_query(
    field_name text,
    index_name regclass,
    query_text text,
    weight real DEFAULT 1.0,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_result_fusion_field_query
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_fusion_field_query(
        $1,
        psql_bm25s_fusion_weighted_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_field_queries(
    field_names text[],
    index_names regclass[],
    query_text text,
    weights real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS psql_bm25s_result_fusion_field_query[]
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT COALESCE(
        array_agg(
            psql_bm25s_fusion_field_query(
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
        ARRAY[]::psql_bm25s_result_fusion_field_query[]
    )
    FROM generate_subscripts(COALESCE(index_names, ARRAY[]::regclass[]), 1) AS i
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_query_prepared(
    prepared_query psql_bm25s_result_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_filter_query(
    ranked_query psql_bm25s_result_ranked_query
)
RETURNS psql_bm25s_result_prepared_query
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ($1).prepared_query
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_order_tokens(
    prepared_query psql_bm25s_result_prepared_query
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

CREATE OR REPLACE FUNCTION psql_bm25s_order_tokens(
    ranked_query psql_bm25s_result_ranked_query
)
RETURNS text[]
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT ($1).order_tokens
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_order_tokens(
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
        psql_bm25s_prepared_query($1, $2, $3, $4, $5, $6)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion(
    left_hits psql_bm25s_result_hit[],
    left_weight real,
    right_hits psql_bm25s_result_hit[],
    right_weight real,
    k int4 DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH left_hits AS (
        SELECT
            (h).ctid AS ctid,
            (h).doc_id AS doc_id,
            (h).score::float8 * COALESCE($2, 1.0)::float8 AS score
        FROM unnest(COALESCE($1, ARRAY[]::psql_bm25s_result_hit[])) AS h
    ),
    right_hits AS (
        SELECT
            (h).ctid AS ctid,
            (h).doc_id AS doc_id,
            (h).score::float8 * COALESCE($4, 1.0)::float8 AS score
        FROM unnest(COALESCE($3, ARRAY[]::psql_bm25s_result_hit[])) AS h
    ),
    fused AS (
        SELECT
            ctid,
            MIN(doc_id) AS doc_id,
            SUM(score) AS score
        FROM (
            SELECT * FROM left_hits
            UNION ALL
            SELECT * FROM right_hits
        ) s
        GROUP BY ctid
    )
    SELECT ctid, doc_id, score::real
    FROM fused
    ORDER BY score DESC, ctid::text
    LIMIT COALESCE(
        $5,
        GREATEST(
            COALESCE(cardinality($1), 0),
            COALESCE(cardinality($3), 0)
        )
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion(
    hits psql_bm25s_result_hit[],
    k int4 DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT (h).ctid, (h).doc_id, (h).score
    FROM unnest(COALESCE($1, ARRAY[]::psql_bm25s_result_hit[])) AS h
    ORDER BY (h).score DESC, (h).ctid::text
    LIMIT COALESCE($2, COALESCE(cardinality($1), 0))
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_query(
    field_names text[],
    index_names regclass[],
    query_text text,
    weights real[] DEFAULT NULL,
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_fusion_query_fields(
        psql_bm25s_fusion_field_queries(
            $1,
            $2,
            $3,
            $4,
            $8,
            $9,
            $10,
            $11
        ),
        $5,
        $6,
        $7
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_query(
    index_names regclass[],
    query_text text,
    weights real[] DEFAULT NULL,
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_fusion_query_weighted(
        psql_bm25s_fusion_weighted_queries(
            $1,
            $2,
            $3,
            $7,
            $8,
            $9,
            $10
        ),
        $4,
        $5,
        $6
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_query_fields(
    field_queries psql_bm25s_result_fusion_field_query[],
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_fusion_query_weighted(
        COALESCE(
            ARRAY(
                SELECT (q).weighted_query
                FROM unnest(
                    COALESCE($1, ARRAY[]::psql_bm25s_result_fusion_field_query[])
                ) AS q
            ),
            ARRAY[]::psql_bm25s_result_fusion_weighted_query[]
        ),
        $2,
        $3,
        $4
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_query_weighted(
    weighted_queries psql_bm25s_result_fusion_weighted_query[],
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    WITH queries AS (
        SELECT
            (q).prepared_query AS prepared_query,
            COALESCE((q).weight, 1.0)::float8 AS weight
        FROM unnest(COALESCE($1, ARRAY[]::psql_bm25s_result_fusion_weighted_query[])) AS q
    ),
    hits AS (
        SELECT
            h.ctid,
            h.doc_id,
            h.score::float8 * q.weight AS score
        FROM queries q
        CROSS JOIN LATERAL psql_bm25s_query_prepared(
            q.prepared_query,
            COALESCE($3, $2),
            $4
        ) AS h
    ),
    fused AS (
        SELECT
            ctid,
            MIN(doc_id) AS doc_id,
            SUM(score) AS score
        FROM hits
        GROUP BY ctid
    )
    SELECT ctid, doc_id, score::real
    FROM fused
    ORDER BY score DESC, ctid::text
    LIMIT $2
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fusion_query_weighted(
    weighted_queries psql_bm25s_result_fusion_weighted_query[],
    query_text text,
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL
)
RETURNS SETOF psql_bm25s_result_hit
LANGUAGE SQL STABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT *
    FROM psql_bm25s_fusion_query_weighted(
        ARRAY(
            SELECT psql_bm25s_fusion_weighted_query(
                ((q).prepared_query).index_name,
                $2,
                (q).weight,
                ((q).prepared_query).lowercase,
                ((q).prepared_query).stopwords,
                ((q).prepared_query).stem_english,
                ((q).prepared_query).fold_diacritics
            )
            FROM unnest(
                COALESCE($1, ARRAY[]::psql_bm25s_result_fusion_weighted_query[])
            ) AS q
        ),
        $3,
        $4,
        $5
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_hybrid_candidate(
    source_name text,
    ctid tid,
    raw_value real,
    source_rank int4,
    weight real DEFAULT 1.0,
    normalizer text DEFAULT 'identity',
    direction text DEFAULT 'higher_is_better'
)
RETURNS psql_bm25s_result_hybrid_candidate
LANGUAGE plpgsql IMMUTABLE PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
DECLARE
    normalized_normalizer text := lower(COALESCE($6, 'identity'));
    normalized_direction text := lower(COALESCE($7, 'higher_is_better'));
BEGIN
    IF normalized_normalizer NOT IN (
        'identity',
        'negative_distance',
        'inverse_distance',
        'minmax',
        'zscore',
        'rank'
    ) THEN
        RAISE EXCEPTION 'unsupported psql_bm25s hybrid normalizer: %',
            $6
            USING HINT = 'Use identity, negative_distance, inverse_distance, minmax, zscore, or rank.';
    END IF;
    IF normalized_direction NOT IN ('higher_is_better', 'lower_is_better') THEN
        RAISE EXCEPTION 'unsupported psql_bm25s hybrid direction: %',
            $7
            USING HINT = 'Use higher_is_better or lower_is_better.';
    END IF;
    RETURN ROW(
        COALESCE(NULLIF($1, ''), 'source'),
        $2,
        $3,
        $4,
        COALESCE($5, 1.0),
        normalized_normalizer,
        normalized_direction
    )::psql_bm25s_result_hybrid_candidate;
END;
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_hybrid_bm25_candidate(
    source_name text,
    ctid tid,
    score real,
    source_rank int4,
    weight real DEFAULT 1.0,
    normalizer text DEFAULT 'identity'
)
RETURNS psql_bm25s_result_hybrid_candidate
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

CREATE OR REPLACE FUNCTION psql_bm25s_hybrid_vector_candidate(
    source_name text,
    ctid tid,
    distance real,
    source_rank int4,
    weight real DEFAULT 1.0,
    normalizer text DEFAULT 'negative_distance'
)
RETURNS psql_bm25s_result_hybrid_candidate
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

CREATE OR REPLACE FUNCTION psql_bm25s_hybrid_bm25_candidates(
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
RETURNS SETOF psql_bm25s_result_hybrid_candidate
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
    FROM psql_bm25s_query_prepared(
        psql_bm25s_prepared_query($2, $3, $7, $8, $9, $10),
        GREATEST(COALESCE($5, 100), 0),
        NULL
    ) AS h
    ORDER BY h.score DESC, h.ctid::text
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_hybrid_fuse_candidates(
    candidates psql_bm25s_result_hybrid_candidate[],
    k int4 DEFAULT 10,
    fusion text DEFAULT 'rrf',
    rrf_k real DEFAULT 60.0,
    epsilon real DEFAULT 0.000001
)
RETURNS SETOF psql_bm25s_result_hybrid_hit
AS 'MODULE_PATHNAME', 'psql_bm25s_hybrid_fuse_candidates'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_match_prepared_query(
    doc_tokens text[],
    prepared_query psql_bm25s_result_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_op_match_prepared_query(
        $1,
        $2
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_match_prepared_query(
    doc_tokens varchar[],
    prepared_query psql_bm25s_result_prepared_query
)
RETURNS boolean
LANGUAGE SQL IMMUTABLE STRICT PARALLEL SAFE
SET search_path FROM CURRENT
AS $$
    SELECT psql_bm25s_op_match_prepared_query(
        $1,
        $2
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_match_prepared_query(
    doc_text text,
    prepared_query psql_bm25s_result_prepared_query
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

CREATE OR REPLACE FUNCTION psql_bm25s_match_prepared_query(
    doc_text varchar,
    prepared_query psql_bm25s_result_prepared_query
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

CREATE OR REPLACE FUNCTION psql_bm25s_match_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_match_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_match_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_match_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_prepared_query(
    doc_tokens text[],
    prepared_query psql_bm25s_result_prepared_query
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
        ELSE -psql_bm25s_op_score_tokens($1, tokens)
    END
    FROM query_tokens
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_prepared_query(
    doc_tokens varchar[],
    prepared_query psql_bm25s_result_prepared_query
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
        ELSE -psql_bm25s_op_score_tokens($1, tokens)
    END
    FROM query_tokens
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_prepared_query(
    doc_text text,
    prepared_query psql_bm25s_result_prepared_query
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

CREATE OR REPLACE FUNCTION psql_bm25s_score_prepared_query(
    doc_text varchar,
    prepared_query psql_bm25s_result_prepared_query
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

CREATE OR REPLACE FUNCTION psql_bm25s_score_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_query(
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
        psql_bm25s_prepared_query($2, $3, $4, $5, $6, $7)
    )
$$;
