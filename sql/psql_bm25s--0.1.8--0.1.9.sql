DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM pg_type t
        JOIN pg_namespace n ON n.oid = t.typnamespace
        WHERE t.typname = 'psql_bm25s_ranked_query'
          AND n.nspname = current_schema()
    ) THEN
        CREATE TYPE psql_bm25s_ranked_query AS (
            prepared_query psql_bm25s_prepared_query,
            order_tokens text[],
            k int4,
            weight_mask real[]
        );
    END IF;

    IF NOT EXISTS (
        SELECT 1
        FROM pg_type t
        JOIN pg_namespace n ON n.oid = t.typnamespace
        WHERE t.typname = 'psql_bm25s_weighted_query'
          AND n.nspname = current_schema()
    ) THEN
        CREATE TYPE psql_bm25s_weighted_query AS (
            prepared_query psql_bm25s_prepared_query,
            weight real
        );
    END IF;

    IF NOT EXISTS (
        SELECT 1
        FROM pg_type t
        JOIN pg_namespace n ON n.oid = t.typnamespace
        WHERE t.typname = 'psql_bm25s_field_query'
          AND n.nspname = current_schema()
    ) THEN
        CREATE TYPE psql_bm25s_field_query AS (
            field_name text,
            weighted_query psql_bm25s_weighted_query
        );
    END IF;
END;
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_fast_path_advice(index_name regclass)
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

CREATE OR REPLACE FUNCTION psql_bm25s_prepare_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_weighted_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_weighted_queries(
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

CREATE OR REPLACE FUNCTION psql_bm25s_field_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_field_queries(
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
        psql_bm25s_query($1, $2, $3, $4, $5, $6)
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_match_prepared_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_match_prepared_query(
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

CREATE OR REPLACE FUNCTION psql_bm25s_score_prepared_query(
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

CREATE OR REPLACE FUNCTION psql_bm25s_score_prepared_query(
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
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
        psql_bm25s_query($2, $3, $4, $5, $6, $7)
    )
$$;

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

CREATE FUNCTION psql_bm25s_index_maintenance_policy_details(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_index_maintenance_policy_details'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE OR REPLACE FUNCTION psql_bm25s_explain_fast_path(
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

DO $$
DECLARE
    extension_oid oid;
    proc record;
BEGIN
    SELECT oid
    INTO extension_oid
    FROM pg_extension
    WHERE extname = 'psql_bm25s';

    FOR proc IN
        SELECT p.oid::regprocedure AS signature
        FROM pg_proc p
        JOIN pg_depend d ON d.objid = p.oid
        JOIN pg_language l ON l.oid = p.prolang
        WHERE d.refobjid = extension_oid
          AND d.deptype = 'e'
          AND l.lanname IN ('sql', 'plpgsql')
          AND p.proname <> 'psql_bm25s_explain_fast_path'
    LOOP
        EXECUTE format(
            'ALTER FUNCTION %s SET search_path FROM CURRENT',
            proc.signature
        );
    END LOOP;
END;
$$;
