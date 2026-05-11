CREATE EXTENSION psql_bm25s WITH SCHEMA public;

CREATE SCHEMA bm25_it;
SET search_path = bm25_it, public;

COPY (
    SELECT count(*)::text
    FROM pg_proc p
    JOIN pg_namespace n
        ON n.oid = p.pronamespace
    WHERE n.nspname = 'public'
      AND p.proname IN (
        'psql_bm25s_hybrid_check_direction',
        'psql_bm25s_hybrid_check_fusion',
        'psql_bm25s_hybrid_check_normalizer',
        'psql_bm25s_hybrid_fuse_candidates_sql_reference',
        'psql_bm25s_fuse_results',
        'psql_bm25s_hits',
        'psql_bm25s_match_query_tokens',
        'psql_bm25s_query_hits',
        'psql_bm25s_search',
        'psql_bm25s_search_ids',
        'psql_bm25s_search_ids_result',
        'psql_bm25s_search_prepared_query',
        'psql_bm25s_search_prepared_query_result',
        'psql_bm25s_search_query',
        'psql_bm25s_search_query_result',
        'psql_bm25s_search_query_result_cfg',
        'psql_bm25s_search_result',
        'psql_bm25s_search_tokens',
        'psql_bm25s_search_tokens_result',
        'psql_bm25s_score_query_tokens'
      )
) TO STDOUT;

COPY (
    SELECT count(*)::text
    FROM pg_type t
    JOIN pg_namespace n
        ON n.oid = t.typnamespace
    WHERE n.nspname = 'public'
      AND t.typname = 'psql_bm25s_result'
) TO STDOUT;

COPY (
    SELECT count(*)::text
    FROM pg_proc p
    JOIN pg_namespace n
        ON n.oid = p.pronamespace
    WHERE n.nspname = 'public'
      AND p.proname = 'psql_bm25s_query'
      AND pg_get_function_identity_arguments(p.oid)
          = 'regclass, text, boolean, text[], boolean, boolean'
) TO STDOUT;

COPY (
    SELECT count(*)::text
    FROM pg_proc p
    JOIN pg_namespace n
        ON n.oid = p.pronamespace
    WHERE n.nspname = 'public'
      AND p.proname IN (
        'psql_bm25s_build_ids',
        'psql_bm25s_build_tokens',
        'psql_bm25s_describe',
        'psql_bm25s_from_bytea',
        'psql_bm25s_index_value',
        'psql_bm25s_num_docs',
        'psql_bm25s_to_bytea',
        'psql_bm25s_topk_ids',
        'psql_bm25s_topk_tokens',
        'psql_bm25s_vocab_size'
      )
) TO STDOUT;

CREATE FUNCTION bm25_it.hit_arrays(hits public.psql_bm25s_result_hit[])
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL IMMUTABLE PARALLEL SAFE
AS $$
    SELECT
        COALESCE(
            array_agg((h).doc_id ORDER BY (h).score DESC, (h).doc_id),
            ARRAY[]::int4[]
        ) AS doc_ids,
        COALESCE(
            array_agg((h).score ORDER BY (h).score DESC, (h).doc_id),
            ARRAY[]::real[]
        ) AS scores
    FROM unnest(COALESCE($1, ARRAY[]::public.psql_bm25s_result_hit[])) AS h
$$;

CREATE FUNCTION bm25_it.search_ids(
    index_name regclass,
    query_ids int4[],
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query_ids($1, $2, $3, $4) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_tokens(
    index_name regclass,
    query_tokens text[],
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query_tokens($1, $2, $3, $4) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_query(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query($1, $2, $3, $4) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_query_cfg(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT false,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT false,
    fold_diacritics boolean DEFAULT false
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query(
            $1,
            $2,
            $3,
            $4,
            $5,
            $6,
            $7,
            $8
        ) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_prepared_query(
    prepared_query public.psql_bm25s_result_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query_prepared($1, $2, $3) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search(
    prepared_query public.psql_bm25s_result_prepared_query,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query_prepared($1, $2, $3) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search(ranked_query public.psql_bm25s_result_ranked_query)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query_prepared(
            ($1).prepared_query,
            ($1).k,
            ($1).weight_mask
        ) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search(
    index_name regclass,
    query_text text,
    k int4 DEFAULT 10,
    weight_mask real[] DEFAULT NULL,
    lowercase boolean DEFAULT NULL,
    stopwords text[] DEFAULT NULL,
    stem_english boolean DEFAULT NULL,
    fold_diacritics boolean DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_query_prepared(
            public.psql_bm25s_prepared_query($1, $2, $5, $6, $7, $8),
            $3,
            $4
        ) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_indexes(
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
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_fusion_query(
            $1,
            $2,
            $3,
            $4,
            $5,
            $6,
            $7,
            $8,
            $9,
            $10,
            $11
        ) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_indexes(
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
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_fusion_query(
            $1,
            $2,
            $3,
            $4,
            $5,
            $6,
            $7,
            $8,
            $9,
            $10
        ) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_field_queries(
    field_queries public.psql_bm25s_result_fusion_field_query[],
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_fusion_query_fields($1, $2, $3, $4) AS h
    ))
$$;

CREATE FUNCTION bm25_it.search_weighted_queries(
    weighted_queries public.psql_bm25s_result_fusion_weighted_query[],
    k int4 DEFAULT 10,
    candidate_k int4 DEFAULT NULL,
    weight_mask real[] DEFAULT NULL
)
RETURNS TABLE(doc_ids int4[], scores real[])
LANGUAGE SQL STABLE PARALLEL SAFE
AS $$
    SELECT *
    FROM bm25_it.hit_arrays(ARRAY(
        SELECT h
        FROM public.psql_bm25s_fusion_query_weighted($1, $2, $3, $4) AS h
    ))
$$;

CREATE TABLE docs_tokens (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_tokens VALUES
    (1, ARRAY['cat','cat','feline']),
    (2, ARRAY['dog','friend']),
    (3, ARRAY['cat','bird','bird']),
    (4, ARRAY[]::text[]);

CREATE INDEX docs_tokens_bm25_idx
    ON docs_tokens USING psql_bm25s (tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_tokens_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_tokens_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_generation_cache_clear() >= 0
) TO STDOUT;

COPY (
    SELECT
        public.psql_bm25s_generation_cache_state(
            'bm25_it.docs_tokens_bm25_idx'::regclass
        ) ~ 'cache_epoch=1, share_eligible=false'
) TO STDOUT;

COPY (
    SELECT
        public.psql_bm25s_generation_cache_preload(
            'bm25_it.docs_tokens_bm25_idx'::regclass
        ) ~ 'tier=(backend_local|shared_preload)'
) TO STDOUT;

COPY (
    WITH advice AS (
        SELECT public.psql_bm25s_fast_path_advice(
            'bm25_it.docs_tokens_bm25_idx'::regclass
        ) AS advice
    )
    SELECT
        advice->>'access_method',
        advice->>'doc_type',
        advice->>'supports_plain_match',
        advice->>'supports_prepared_match',
        advice->>'supports_ordering',
        advice->>'supports_filtered_ranked',
        advice->>'canonical_api',
        advice->>'recommended_filter',
        advice->>'recommended_order'
    FROM advice
) TO STDOUT;

SET enable_seqscan = off;
COPY (
    WITH plan_info AS (
        SELECT public.psql_bm25s_fast_path_explain(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            $q$
                SELECT id
                FROM bm25_it.docs_tokens
                WHERE tokens @@@ public.psql_bm25s_prepared_query(
                    'bm25_it.docs_tokens_bm25_idx'::regclass,
                    'bird cat missing'
                )
                ORDER BY tokens <=> public.psql_bm25s_order_tokens(
                    'bm25_it.docs_tokens_bm25_idx'::regclass,
                    'bird cat missing'
                ) ASC
                LIMIT 3
            $q$
        ) AS plan_info
    )
    SELECT
        plan_info->>'used_psql_bm25s_index',
        plan_info->>'used_index_scan',
        plan_info->>'used_bitmap_index_scan',
        plan_info->>'used_ordered_index_scan',
        plan_info->>'uses_prepared_match',
        plan_info->>'uses_ordering_operator'
    FROM plan_info
) TO STDOUT;
RESET enable_seqscan;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_tokens_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT id, score
FROM public.psql_bm25s_query_tokens(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    ARRAY['bird','cat','missing'],
    3,
    NULL
) h
JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
ORDER BY score DESC, id;

SELECT *
FROM public.psql_bm25s_query_tokens(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    ARRAY['bird','cat','missing'],
    3,
    NULL
);

SELECT id, score
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'bird cat missing',
    3,
    NULL
) h
JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
ORDER BY score DESC, id;

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'bird cat missing',
    3,
    NULL
);

CREATE TABLE docs_tokens_varchar (
    id int primary key,
    tokens varchar[] not null
);

INSERT INTO docs_tokens_varchar VALUES
    (1, ARRAY['cat','cat','feline']::varchar[]),
    (2, ARRAY['dog','friend']::varchar[]),
    (3, ARRAY['cat','bird','bird']::varchar[]),
    (4, ARRAY[]::varchar[]);

CREATE INDEX docs_tokens_varchar_bm25_idx
    ON docs_tokens_varchar USING psql_bm25s (tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_tokens_varchar_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    WITH advice AS (
        SELECT public.psql_bm25s_fast_path_advice(
            'bm25_it.docs_tokens_varchar_bm25_idx'::regclass
        ) AS advice
    )
    SELECT
        advice->>'access_method',
        advice->>'doc_type',
        advice->>'supports_plain_match',
        advice->>'supports_prepared_match',
        advice->>'supports_ordering',
        advice->>'supports_filtered_ranked'
    FROM advice
) TO STDOUT;

COPY (
    SELECT doc_ids, scores
    FROM bm25_it.search_tokens(
        'bm25_it.docs_tokens_varchar_bm25_idx'::regclass,
        ARRAY['bird','cat','missing'],
        3,
        NULL
    )
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens_varchar
    WHERE tokens @@ 'cat'
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens_varchar
    WHERE tokens @@@ public.psql_bm25s_prepared_query(
        'bm25_it.docs_tokens_varchar_bm25_idx'::regclass,
        'cat'
    )
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens_varchar
    WHERE tokens @@ 'cat'
    ORDER BY tokens <=> ARRAY['bird','cat','missing']::text[] ASC
    LIMIT 2
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_normalize_tokens(
        ARRAY['Cat','Bird']::varchar[],
        true,
        ARRAY['bird']::text[]
    )
) TO STDOUT;

CREATE TABLE docs_text_scalar (
    id int primary key,
    content text not null
);

INSERT INTO docs_text_scalar VALUES
    (1, 'Cat cat feline'),
    (2, 'Dog friend'),
    (3, 'cat bird bird'),
    (4, 'Café bird');

CREATE INDEX docs_text_scalar_bm25_idx
    ON docs_text_scalar USING psql_bm25s (content)
    WITH (
        text_lowercase = true,
        text_fold_diacritics = true
    );

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_scalar_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    WITH advice AS (
        SELECT public.psql_bm25s_fast_path_advice(
            'bm25_it.docs_text_scalar_bm25_idx'::regclass
        ) AS advice
    )
    SELECT
        advice->>'access_method',
        advice->>'doc_type',
        advice->>'supports_plain_match',
        advice->>'supports_prepared_match',
        advice->>'supports_ordering',
        advice->>'supports_filtered_ranked',
        advice->>'canonical_api',
        COALESCE(advice->>'recommended_filter', '<null>'),
        COALESCE(advice->>'recommended_order', '<null>')
    FROM advice
) TO STDOUT;

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        'cafe bird',
        2,
        NULL
    ) h
    JOIN bm25_it.docs_text_scalar d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT id
    FROM public.psql_bm25s_query_tokens(
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        ARRAY['cat'],
        2,
        NULL
) h
JOIN bm25_it.docs_text_scalar d ON d.ctid = h.ctid
ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT
        (q).lowercase,
        COALESCE(array_to_string((q).stopwords, ','), '<null>'),
        (q).stem_english,
        (q).fold_diacritics
    FROM (
        SELECT public.psql_bm25s_prepared_query(
            'bm25_it.docs_text_scalar_bm25_idx'::regclass,
            'CAFE BIRD'
        ) AS q
    ) prepared
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_text_scalar
    WHERE public.psql_bm25s_match_query(
        content,
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        'CAFE BIRD'
    )
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_text_scalar
    WHERE public.psql_bm25s_match_query(
        content,
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        'CAFE BIRD',
        false,
        NULL,
        false,
        false
    )
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_text_scalar
    ORDER BY public.psql_bm25s_score_query(
        content,
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        'CAFE BIRD'
    ) DESC, id
    LIMIT 1
) TO STDOUT;

SET enable_seqscan = off;
COPY (
    WITH plan_info AS (
        SELECT public.psql_bm25s_fast_path_explain(
            'bm25_it.docs_text_scalar_bm25_idx'::regclass,
            $q$
                SELECT id
                FROM bm25_it.docs_text_scalar
                WHERE content @@@ public.psql_bm25s_prepared_query(
                    'bm25_it.docs_text_scalar_bm25_idx'::regclass,
                    'cafe bird',
                    true,
                    NULL,
                    false,
                    true
                )
                ORDER BY content <=> public.psql_bm25s_order_tokens(
                    'bm25_it.docs_text_scalar_bm25_idx'::regclass,
                    'cafe bird',
                    true,
                    NULL,
                    false,
                    true
                ) ASC
                LIMIT 2
            $q$
        ) AS plan_info
    )
    SELECT
        plan_info->>'used_psql_bm25s_index',
        plan_info->>'used_index_scan',
        plan_info->>'used_bitmap_index_scan',
        plan_info->>'used_ordered_index_scan',
        plan_info->>'uses_prepared_match',
        plan_info->>'uses_ordering_operator'
    FROM plan_info
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_text_scalar
    WHERE content @@ '"cafe bird"'
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_text_scalar
    WHERE content @@@ public.psql_bm25s_prepared_query(
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        'cafe bird',
        true,
        NULL,
        false,
        true
    )
    ORDER BY content <=> public.psql_bm25s_order_tokens(
        'bm25_it.docs_text_scalar_bm25_idx'::regclass,
        'cafe bird',
        true,
        NULL,
        false,
        true
    ) ASC, id
    LIMIT 2
) TO STDOUT;
RESET enable_seqscan;

CREATE TABLE docs_text_scalar_default (
    id int primary key,
    content text not null
);

INSERT INTO docs_text_scalar_default VALUES
    (1, 'Cat bird'),
    (2, 'dog friend'),
    (3, 'CAT');

CREATE INDEX docs_text_scalar_default_bm25_idx
    ON docs_text_scalar_default USING psql_bm25s (content);

COPY (
    SELECT
        (q).lowercase::text,
        (q).stem_english::text,
        (q).fold_diacritics::text
    FROM (
        SELECT public.psql_bm25s_prepared_query(
            'bm25_it.docs_text_scalar_default_bm25_idx'::regclass,
            'CAT'
        ) AS q
    ) prepared
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_order_tokens(
        'bm25_it.docs_text_scalar_default_bm25_idx'::regclass,
        'CAT'
    )::text
) TO STDOUT;

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_scalar_default_bm25_idx'::regclass,
        'CAT',
        10,
        NULL
    ) h
    JOIN bm25_it.docs_text_scalar_default d ON d.ctid = h.ctid
    WHERE h.score > 0
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_scalar_default_bm25_idx'::regclass,
        'CAT',
        10,
        NULL
    ) h
    JOIN bm25_it.docs_text_scalar_default d ON d.ctid = h.ctid
    WHERE h.score > 0
    ORDER BY id
) TO STDOUT;

CREATE TABLE docs_varchar_scalar (
    id int primary key,
    content varchar not null
);

INSERT INTO docs_varchar_scalar VALUES
    (1, 'Cat cat feline'),
    (2, 'Dog friend'),
    (3, 'cat bird bird'),
    (4, 'Café bird');

CREATE INDEX docs_varchar_scalar_bm25_idx
    ON docs_varchar_scalar USING psql_bm25s (content)
    WITH (
        text_lowercase = true,
        text_fold_diacritics = true
    );

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    WITH advice AS (
        SELECT public.psql_bm25s_fast_path_advice(
            'bm25_it.docs_varchar_scalar_bm25_idx'::regclass
        ) AS advice
    )
    SELECT
        advice->>'access_method',
        advice->>'doc_type',
        advice->>'supports_plain_match',
        advice->>'supports_prepared_match',
        advice->>'supports_ordering',
        advice->>'supports_filtered_ranked',
        advice->>'canonical_api',
        COALESCE(advice->>'recommended_filter', '<null>'),
        COALESCE(advice->>'recommended_order', '<null>')
    FROM advice
) TO STDOUT;

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass,
        'cafe bird',
        2,
        NULL
) h
JOIN bm25_it.docs_varchar_scalar d ON d.ctid = h.ctid
ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_varchar_scalar
    WHERE public.psql_bm25s_match_query(
        content,
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass,
        'CAFE BIRD'
    )
    ORDER BY id
) TO STDOUT;

SET enable_seqscan = off;
COPY (
    SELECT id
    FROM bm25_it.docs_varchar_scalar
    WHERE content @@ '"cafe bird"'
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_varchar_scalar
    WHERE content @@@ public.psql_bm25s_prepared_query(
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass,
        'cafe bird',
        true,
        NULL,
        false,
        true
    )
    ORDER BY content <=> public.psql_bm25s_order_tokens(
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass,
        'cafe bird',
        true,
        NULL,
        false,
        true
    ) ASC, id
    LIMIT 2
) TO STDOUT;
RESET enable_seqscan;

SELECT public.psql_bm25s_match_query(
    'Café bird',
    'bm25_it.docs_text_scalar_bm25_idx'::regclass,
    'cafe bird',
    true,
    NULL::text[],
    false,
    true
);

SELECT public.psql_bm25s_score_query(
    'Café bird',
    'bm25_it.docs_text_scalar_bm25_idx'::regclass,
    'cafe bird',
    true,
    NULL::text[],
    false,
    true
) > 0;

SELECT public.psql_bm25s_match_prepared_query(
    'Café bird'::varchar,
    public.psql_bm25s_prepared_query(
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass,
        'cafe bird',
        true,
        NULL::text[],
        false,
        true
    )
);

SELECT public.psql_bm25s_score_prepared_query(
    'Café bird'::varchar,
    public.psql_bm25s_prepared_query(
        'bm25_it.docs_varchar_scalar_bm25_idx'::regclass,
        'cafe bird',
        true,
        NULL::text[],
        false,
        true
    )
) > 0;

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    '+cat -bird',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'bi* cat',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    '"cat bird"',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'cat AND bird',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'cat AND NOT bird',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'cat OR bird AND missing',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'cat AND (bird OR dog)',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'cat AND NOT (bird OR dog)',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    '"cat bird" OR (cat AND feline)',
    3,
    NULL
);

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'CAT and BIRD',
    3,
    NULL,
    true,
    ARRAY['and']::text[]
);

SELECT id, score
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'CAT and BIRD',
    3,
    NULL,
    true,
    ARRAY['and']::text[]
) h
JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
ORDER BY score DESC, id;

SELECT *
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    '"CAT BIRD"',
    3,
    NULL,
    true,
    NULL::text[]
);

COPY (
    SELECT
        (q).index_name::text,
        (q).query_text,
        (q).lowercase::text,
        COALESCE((q).stopwords::text, 'NULL'),
        (q).stem_english::text,
        (q).fold_diacritics::text
    FROM (
        SELECT public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ) AS q
    ) s
) TO STDOUT;

COPY (
    SELECT
        (q).index_name::text,
        (q).query_text,
        (q).lowercase::text,
        COALESCE((q).stopwords::text, 'NULL'),
        (q).stem_english::text,
        (q).fold_diacritics::text
    FROM (
        SELECT public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ) AS q
    ) s
) TO STDOUT;

COPY (
    SELECT id, score
    FROM public.psql_bm25s_query_prepared(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ),
        3,
        NULL
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT id, score
    FROM public.psql_bm25s_query_prepared(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ),
        3,
        NULL
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_prepared_query(
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'bird cat missing'
    ) IS NULL
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_order_tokens(
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'bird cat missing'
    )::text
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_order_tokens(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'bird cat missing'
        )
    )::text
) TO STDOUT;

COPY (
    SELECT
        (q).prepared_query IS NULL,
        (q).order_tokens::text,
        (q).k::text,
        COALESCE((q).weight_mask::text, 'NULL')
    FROM (
        SELECT public.psql_bm25s_ranked_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            3,
            NULL,
            true,
            ARRAY['and']::text[]
        ) AS q
    ) s
) TO STDOUT;

COPY (
    WITH rq AS (
        SELECT public.psql_bm25s_ranked_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            3,
            NULL,
            true,
            ARRAY['and']::text[]
        ) AS query
    )
    SELECT id, score
    FROM rq
    CROSS JOIN LATERAL public.psql_bm25s_query_prepared(
        (rq.query).prepared_query,
        (rq.query).k,
        (rq.query).weight_mask
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_order_tokens(
        public.psql_bm25s_ranked_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'bird cat missing'
        )
    )::text
) TO STDOUT;

COPY (
    SELECT id, score
    FROM public.psql_bm25s_query_prepared(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ),
        3,
        NULL
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT id, score
    FROM public.psql_bm25s_query_prepared(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[],
            false,
            false
        ),
        3,
        NULL
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_prepared_query(
            public.psql_bm25s_prepared_query(
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'CAT and BIRD',
                true,
                ARRAY['and']::text[]
            ),
            3,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    WITH qs AS (
        SELECT public.psql_bm25s_fusion_weighted_queries(
            ARRAY[
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'bm25_it.docs_tokens_bm25_idx'::regclass
            ],
            'bird',
            ARRAY[2.0, 1.0]::real[]
        ) AS qs
    )
    SELECT
        cardinality(qs)::text,
        ((qs)[1]).weight::text,
        ((qs)[2]).weight::text
    FROM qs
) TO STDOUT;

COPY (
    WITH qs AS (
        SELECT public.psql_bm25s_fusion_field_queries(
            ARRAY['title', 'body']::text[],
            ARRAY[
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'bm25_it.docs_tokens_bm25_idx'::regclass
            ],
            'bird',
            ARRAY[2.0, 1.0]::real[]
        ) AS qs
    )
    SELECT
        cardinality(qs)::text,
        ((qs)[1]).field_name,
        (((qs)[1]).weighted_query).weight::text,
        ((qs)[2]).field_name,
        (((qs)[2]).weighted_query).weight::text
    FROM qs
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_indexes(
            ARRAY['title', 'body']::text[],
            ARRAY[
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'bm25_it.docs_tokens_bm25_idx'::regclass
            ],
            'bird',
            ARRAY[2.0, 1.0]::real[],
            3,
            3,
            NULL
        ) AS r
    ) s
) TO STDOUT;

CREATE TABLE docs_tokens_multi (
    id int primary key,
    title_tokens text[],
    body_tokens text[],
    combined_tokens text[] not null
);

INSERT INTO docs_tokens_multi VALUES
    (1, ARRAY['cat'], ARRAY['feline'], ARRAY['cat', 'feline']),
    (2, ARRAY['bird'], ARRAY['cat'], ARRAY['bird', 'cat']),
    (3, ARRAY['dog'], ARRAY['bird', 'bird'], ARRAY['dog', 'bird', 'bird']),
    (4, ARRAY[]::text[], ARRAY['cat'], ARRAY['cat']),
    (5, NULL, ARRAY['cat', 'bird'], ARRAY['cat', 'bird']),
    (6, ARRAY['bird'], NULL, ARRAY['bird']);

CREATE INDEX docs_tokens_multi_fused_bm25_idx
    ON docs_tokens_multi USING psql_bm25s (title_tokens, body_tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

CREATE INDEX docs_tokens_multi_field_bm25_idx
    ON docs_tokens_multi USING psql_bm25s (title_tokens, body_tokens)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        field_aware = true
    );

CREATE INDEX docs_tokens_multi_combined_bm25_idx
    ON docs_tokens_multi USING psql_bm25s (combined_tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_tokens(
            'bm25_it.docs_tokens_multi_fused_bm25_idx'::regclass,
            ARRAY['cat', 'bird'],
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT d.id, round(h.score::numeric, 6)::text
    FROM public.psql_bm25s_field_aware_query_tokens(
        'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
        ARRAY['bird'],
        ARRAY['title_tokens', 'body_tokens'],
        ARRAY[3.0, 1.0]::real[],
        6
    ) h
    JOIN bm25_it.docs_tokens_multi d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

COPY (
    SELECT d.id, round(h.score::numeric, 6)::text
    FROM public.psql_bm25s_field_aware_query_tokens(
        'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
        ARRAY['bird'],
        ARRAY['title_tokens', 'body_tokens'],
        ARRAY[-1.0, 1.0]::real[],
        6
    ) h
    JOIN bm25_it.docs_tokens_multi d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

COPY (
    SELECT d.id, round(h.score::numeric, 6)::text
    FROM public.psql_bm25s_field_aware_query(
        'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
        'bird',
        ARRAY['title_tokens', 'body_tokens'],
        ARRAY[3.0, 1.0]::real[],
        6
    ) h
    JOIN bm25_it.docs_tokens_multi d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

COPY (
    WITH generic AS (
        SELECT d.id, round(h.score::numeric, 6)::text AS score
        FROM public.psql_bm25s_query_tokens(
            'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
            ARRAY['bird'],
            6,
            NULL
        ) h
        JOIN bm25_it.docs_tokens_multi d ON d.ctid = h.ctid
    ),
    explicit AS (
        SELECT d.id, round(h.score::numeric, 6)::text AS score
        FROM public.psql_bm25s_field_aware_query_tokens(
            'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
            ARRAY['bird'],
            ARRAY['title_tokens', 'body_tokens'],
            ARRAY[1.0, 1.0]::real[],
            6
        ) h
        JOIN bm25_it.docs_tokens_multi d ON d.ctid = h.ctid
    )
    SELECT
        generic.id,
        generic.score,
        explicit.score,
        (generic.score = explicit.score)::text AS equal_weight_match
    FROM generic
    JOIN explicit USING (id)
    ORDER BY generic.score::numeric DESC, generic.id
) TO STDOUT;

COPY (
    SELECT d.id, round(h.score::numeric, 6)::text
    FROM public.psql_bm25s_query(
        'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
        'bird',
        6,
        NULL
    ) h
    JOIN bm25_it.docs_tokens_multi d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;
SELECT bm25_it.search_query(
    'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass,
    '"bird cat"',
    3,
    NULL
);
COPY (
    SELECT
    public.psql_bm25s_fast_path_advice(
        'bm25_it.docs_tokens_multi_field_bm25_idx'::regclass
    )->>'canonical_api'
) TO STDOUT;

CREATE TABLE docs_tokens_multi_field_delta (
    id int primary key,
    title_tokens text[],
    body_tokens text[]
);

INSERT INTO docs_tokens_multi_field_delta VALUES
    (1, ARRAY['cat'], ARRAY['feline']),
    (2, ARRAY['dog'], ARRAY['bird']);

CREATE INDEX docs_tokens_multi_field_delta_bm25_idx
    ON docs_tokens_multi_field_delta USING psql_bm25s (title_tokens, body_tokens)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        field_aware = true,
        auto_rebuild_threshold = 10
    );

INSERT INTO docs_tokens_multi_field_delta VALUES
    (3, ARRAY['bird'], ARRAY['cat']);

COPY (
    SELECT d.id, round(h.score::numeric, 6)::text
    FROM public.psql_bm25s_field_aware_query_tokens(
        'bm25_it.docs_tokens_multi_field_delta_bm25_idx'::regclass,
        ARRAY['bird'],
        ARRAY['title_tokens', 'body_tokens'],
        ARRAY[3.0, 1.0]::real[],
        5
    ) h
    JOIN bm25_it.docs_tokens_multi_field_delta d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

CREATE TABLE docs_tokens_multi_eventual_field_delta (
    id int primary key,
    title_tokens text[],
    body_tokens text[]
);

INSERT INTO docs_tokens_multi_eventual_field_delta VALUES
    (1, ARRAY['cat'], ARRAY['feline']),
    (2, ARRAY['dog'], ARRAY['bird']);

CREATE INDEX docs_tokens_multi_eventual_field_delta_bm25_idx
    ON docs_tokens_multi_eventual_field_delta
    USING psql_bm25s (title_tokens, body_tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 10,
        query_overlay_max_records = 10,
        field_aware = true
    );

INSERT INTO docs_tokens_multi_eventual_field_delta VALUES
    (3, ARRAY['fresh', 'bird'], ARRAY['overlay']);

COPY (
    WITH first_run AS (
        SELECT array_agg(d.id ORDER BY h.score DESC, d.id) AS ids
        FROM public.psql_bm25s_field_aware_query_tokens(
            'bm25_it.docs_tokens_multi_eventual_field_delta_bm25_idx'::regclass,
            ARRAY['fresh', 'bird'],
            ARRAY['title_tokens', 'body_tokens'],
            ARRAY[2.0, 1.0]::real[],
            5
        ) h
        JOIN bm25_it.docs_tokens_multi_eventual_field_delta d
            ON d.ctid = h.ctid
    ),
    second_run AS (
        SELECT array_agg(d.id ORDER BY h.score DESC, d.id) AS ids
        FROM public.psql_bm25s_field_aware_query_tokens(
            'bm25_it.docs_tokens_multi_eventual_field_delta_bm25_idx'::regclass,
            ARRAY['fresh', 'bird'],
            ARRAY['title_tokens', 'body_tokens'],
            ARRAY[2.0, 1.0]::real[],
            5
        ) h
        JOIN bm25_it.docs_tokens_multi_eventual_field_delta d
            ON d.ctid = h.ctid
    )
    SELECT
        3 = ANY(first_run.ids) AS delta_doc_visible,
        first_run.ids = second_run.ids AS repeat_query_stable
    FROM first_run, second_run
) TO STDOUT;

CREATE TABLE docs_tokens_multi_varchar (
    id int primary key,
    title_tokens varchar[],
    body_tokens varchar[],
    combined_tokens text[] not null
);

INSERT INTO docs_tokens_multi_varchar VALUES
    (1, ARRAY['cat']::varchar[], ARRAY['feline']::varchar[], ARRAY['cat', 'feline']),
    (2, ARRAY['bird']::varchar[], ARRAY['cat']::varchar[], ARRAY['bird', 'cat']),
    (3, ARRAY['dog']::varchar[], ARRAY['bird', 'bird']::varchar[], ARRAY['dog', 'bird', 'bird']),
    (4, ARRAY[]::varchar[], ARRAY['cat']::varchar[], ARRAY['cat']),
    (5, NULL, ARRAY['cat', 'bird']::varchar[], ARRAY['cat', 'bird']),
    (6, ARRAY['bird']::varchar[], NULL, ARRAY['bird']);

CREATE INDEX docs_tokens_multi_varchar_fused_bm25_idx
    ON docs_tokens_multi_varchar USING psql_bm25s (title_tokens, body_tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

CREATE INDEX docs_tokens_multi_varchar_combined_bm25_idx
    ON docs_tokens_multi_varchar USING psql_bm25s (combined_tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_tokens(
            'bm25_it.docs_tokens_multi_varchar_fused_bm25_idx'::regclass,
            ARRAY['cat', 'bird'],
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        round(h.score::numeric, 6)::text,
        h.source_count::text,
        h.source_names::text,
        h.ranks::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_bm25_candidate(
                'title',
                '(0,1)'::tid,
                4.0,
                1,
                2.0
            ),
            public.psql_bm25s_hybrid_vector_candidate(
                'embedding',
                '(0,1)'::tid,
                0.4,
                2,
                1.0
            ),
            public.psql_bm25s_hybrid_vector_candidate(
                'embedding',
                '(0,2)'::tid,
                0.1,
                1,
                1.0
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        3,
        'rrf',
        60.0
    ) AS h
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        round(h.score::numeric, 6)::text,
        h.source_names::text,
        h.normalized_scores::text,
        h.weighted_scores::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_candidate(
                'title',
                '(0,1)'::tid,
                10.0,
                1,
                2.0,
                'minmax',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'title',
                '(0,2)'::tid,
                5.0,
                2,
                2.0,
                'minmax',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'embedding',
                '(0,1)'::tid,
                0.4,
                2,
                3.0,
                'minmax',
                'lower_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'embedding',
                '(0,2)'::tid,
                0.1,
                1,
                3.0,
                'minmax',
                'lower_is_better'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        2,
        'score'
    ) AS h
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        round(h.score::numeric, 6)::text,
        round((h.normalized_scores)[1]::numeric, 6)::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_candidate(
                'z',
                '(0,1)'::tid,
                1.0,
                1,
                1.0,
                'zscore',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'z',
                '(0,2)'::tid,
                2.0,
                2,
                1.0,
                'zscore',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'z',
                '(0,3)'::tid,
                3.0,
                3,
                1.0,
                'zscore',
                'higher_is_better'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        3,
        'score'
    ) AS h
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        round(h.score::numeric, 6)::text,
        h.source_names::text,
        h.raw_values::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_candidate(
                'ranked',
                '(0,1)'::tid,
                99.0,
                2,
                1.0,
                'rank',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_vector_candidate(
                'distance',
                '(0,1)'::tid,
                0.5,
                1,
                1.0,
                'inverse_distance'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        1,
        'score'
    ) AS h
) TO STDOUT;

COPY (
    SELECT count(*)::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[]::public.psql_bm25s_result_hybrid_candidate[],
        10,
        'rrf'
    )
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        round(h.score::numeric, 6)::text,
        h.normalized_scores::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_candidate(
                'single',
                '(0,1)'::tid,
                42.0,
                1,
                1.0,
                'minmax',
                'higher_is_better'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        1,
        'score'
    ) AS h
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        round(h.score::numeric, 6)::text,
        h.normalized_scores::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_candidate(
                'flat',
                '(0,1)'::tid,
                7.0,
                1,
                1.0,
                'zscore',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'flat',
                '(0,2)'::tid,
                7.0,
                2,
                1.0,
                'zscore',
                'higher_is_better'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        2,
        'score'
    ) AS h
) TO STDOUT;

COPY (
    SELECT
        h.ctid::text,
        h.raw_values::text,
        h.ranks::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_vector_candidate(
                'distance',
                '(0,1)'::tid,
                0.9,
                1,
                1.0,
                'rank'
            ),
            public.psql_bm25s_hybrid_vector_candidate(
                'distance',
                '(0,1)'::tid,
                0.1,
                1,
                1.0,
                'rank'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        1,
        'score'
    ) AS h
) TO STDOUT;

COPY (
    SELECT
        count(*)::text
    FROM public.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            public.psql_bm25s_hybrid_candidate(
                'bad',
                '(0,1)'::tid,
                'NaN'::real,
                1,
                1.0,
                'identity',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'bad',
                '(0,2)'::tid,
                'Infinity'::real,
                2,
                1.0,
                'identity',
                'higher_is_better'
            ),
            public.psql_bm25s_hybrid_candidate(
                'bad',
                '(0,3)'::tid,
                '-Infinity'::real,
                3,
                1.0,
                'identity',
                'higher_is_better'
            )
        ]::public.psql_bm25s_result_hybrid_candidate[],
        3,
        'score'
    )
) TO STDOUT;

COPY (
    WITH candidates AS (
        SELECT array_agg(c) AS arr
        FROM public.psql_bm25s_hybrid_bm25_candidates(
            'tokens',
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'bird',
            2.0,
            3
        ) AS c
    )
    SELECT
        d.id::text,
        round(h.score::numeric, 6)::text,
        h.source_names::text,
        h.ranks::text
    FROM candidates
    CROSS JOIN LATERAL public.psql_bm25s_hybrid_fuse_candidates(
        candidates.arr,
        3,
        'rrf'
    ) AS h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_tokens(
            'bm25_it.docs_tokens_multi_varchar_combined_bm25_idx'::regclass,
            ARRAY['cat', 'bird'],
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_tokens(
            'bm25_it.docs_tokens_multi_combined_bm25_idx'::regclass,
            ARRAY['cat', 'bird'],
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_tokens_multi_fused_bm25_idx'::regclass,
            '"cat bird"',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_tokens_multi_combined_bm25_idx'::regclass,
            '"cat bird"',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

SET enable_seqscan = off;
DO $$
BEGIN
    PERFORM id
    FROM docs_tokens_multi
    WHERE title_tokens @@@ public.psql_bm25s_prepared_query(
        'bm25_it.docs_tokens_multi_fused_bm25_idx'::regclass,
        'cat'
    );
EXCEPTION
    WHEN OTHERS THEN
        RAISE NOTICE '%', SQLERRM;
END
$$;
RESET enable_seqscan;

CREATE TABLE docs_text_multi_scalar (
    id int primary key,
    title_text text,
    body_text text,
    combined_text text not null
);

INSERT INTO docs_text_multi_scalar VALUES
    (1, 'Cat', 'feline', 'Cat feline'),
    (2, 'Bird', 'cat', 'Bird cat'),
    (3, 'Dog', 'bird bird', 'Dog bird bird'),
    (4, NULL, 'cat', 'cat'),
    (5, 'Café', 'bird', 'Café bird'),
    (6, 'bird', NULL, 'bird');

CREATE INDEX docs_text_multi_scalar_fused_bm25_idx
    ON docs_text_multi_scalar USING psql_bm25s (title_text, body_text)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        text_lowercase = true,
        text_fold_diacritics = true
    );

CREATE INDEX docs_text_multi_scalar_combined_bm25_idx
    ON docs_text_multi_scalar USING psql_bm25s (combined_text)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        text_lowercase = true,
        text_fold_diacritics = true
    );

CREATE INDEX docs_text_multi_scalar_field_bm25_idx
    ON docs_text_multi_scalar USING psql_bm25s (title_text, body_text)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        text_lowercase = true,
        text_fold_diacritics = true,
        field_aware = true
    );

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_text_multi_scalar_fused_bm25_idx'::regclass,
            'cat bird',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_text_multi_scalar_combined_bm25_idx'::regclass,
            'cat bird',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_text_multi_scalar_fused_bm25_idx'::regclass,
            '"cafe bird"',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_text_multi_scalar_combined_bm25_idx'::regclass,
            '"cafe bird"',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    WITH advice AS (
        SELECT public.psql_bm25s_fast_path_advice(
            'bm25_it.docs_text_multi_scalar_fused_bm25_idx'::regclass
        ) AS advice
    )
    SELECT
        advice->>'doc_type',
        advice->>'supports_plain_match',
        advice->>'supports_prepared_match',
        advice->>'supports_ordering',
        advice->>'supports_filtered_ranked',
        advice->>'canonical_api'
    FROM advice
) TO STDOUT;

COPY (
    SELECT d.id, round(h.score::numeric, 6)::text
    FROM public.psql_bm25s_field_aware_query(
        'bm25_it.docs_text_multi_scalar_field_bm25_idx'::regclass,
        'CAFE bird',
        ARRAY['title_text', 'body_text'],
        ARRAY[2.0, 1.0]::real[],
        6
    ) h
    JOIN bm25_it.docs_text_multi_scalar d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

COPY (
    WITH generic AS (
        SELECT d.id, round(h.score::numeric, 6)::text AS score
        FROM public.psql_bm25s_query(
            'bm25_it.docs_text_multi_scalar_field_bm25_idx'::regclass,
            'CAFE',
            6,
            NULL
        ) h
        JOIN bm25_it.docs_text_multi_scalar d ON d.ctid = h.ctid
    ),
    explicit AS (
        SELECT d.id, round(h.score::numeric, 6)::text AS score
        FROM public.psql_bm25s_field_aware_query(
            'bm25_it.docs_text_multi_scalar_field_bm25_idx'::regclass,
            'CAFE',
            ARRAY['title_text', 'body_text'],
            ARRAY[1.0, 1.0]::real[],
            6
        ) h
        JOIN bm25_it.docs_text_multi_scalar d ON d.ctid = h.ctid
    )
    SELECT
        generic.id,
        generic.score,
        explicit.score,
        (generic.score = explicit.score)::text AS equal_weight_match
    FROM generic
    JOIN explicit USING (id)
    ORDER BY generic.score::numeric DESC, generic.id
) TO STDOUT;

CREATE TABLE docs_varchar_multi_scalar (
    id int primary key,
    title_text varchar,
    body_text varchar,
    combined_text varchar not null
);

INSERT INTO docs_varchar_multi_scalar VALUES
    (1, 'Cat', 'feline', 'Cat feline'),
    (2, 'Bird', 'cat', 'Bird cat'),
    (3, 'Dog', 'bird bird', 'Dog bird bird'),
    (4, NULL, 'cat', 'cat'),
    (5, 'Café', 'bird', 'Café bird'),
    (6, 'bird', NULL, 'bird');

CREATE INDEX docs_varchar_multi_scalar_fused_bm25_idx
    ON docs_varchar_multi_scalar USING psql_bm25s (title_text, body_text)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        text_lowercase = true,
        text_fold_diacritics = true
    );

CREATE INDEX docs_varchar_multi_scalar_combined_bm25_idx
    ON docs_varchar_multi_scalar USING psql_bm25s (combined_text)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        text_lowercase = true,
        text_fold_diacritics = true
    );

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_varchar_multi_scalar_fused_bm25_idx'::regclass,
            'cat bird',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_query(
            'bm25_it.docs_varchar_multi_scalar_combined_bm25_idx'::regclass,
            'cat bird',
            5,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_indexes(
            ARRAY[
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'bm25_it.docs_tokens_bm25_idx'::regclass
            ],
            'bird',
            ARRAY[2.0, 1.0]::real[],
            3,
            3,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_field_queries(
            ARRAY[
                public.psql_bm25s_fusion_field_query(
                    'title',
                    'bm25_it.docs_tokens_bm25_idx'::regclass,
                    'bird',
                    2.0
                ),
                public.psql_bm25s_fusion_field_query(
                    'body',
                    'bm25_it.docs_tokens_bm25_idx'::regclass,
                    'cat',
                    1.0
                )
            ]::public.psql_bm25s_result_fusion_field_query[],
            3,
            3,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search_weighted_queries(
            ARRAY[
                public.psql_bm25s_fusion_weighted_query(
                    'bm25_it.docs_tokens_bm25_idx'::regclass,
                    'bird',
                    2.0
                ),
                public.psql_bm25s_fusion_weighted_query(
                    'bm25_it.docs_tokens_bm25_idx'::regclass,
                    'cat',
                    1.0
                )
            ]::public.psql_bm25s_result_fusion_weighted_query[],
            3,
            3,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search(
            public.psql_bm25s_prepared_query(
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'CAT and BIRD',
                true,
                ARRAY['and']::text[]
            ),
            3,
            NULL
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search(
            public.psql_bm25s_ranked_query(
                'bm25_it.docs_tokens_bm25_idx'::regclass,
                'CAT and BIRD',
                3,
                NULL,
                true,
                ARRAY['and']::text[]
            )
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.hit_arrays(ARRAY(
            SELECT h
            FROM public.psql_bm25s_fusion(
                ARRAY(
                    SELECT h
                    FROM public.psql_bm25s_query(
                        'bm25_it.docs_tokens_bm25_idx'::regclass,
                        'bird',
                        3,
                        NULL
                    ) AS h
                ),
            2.0,
                ARRAY(
                    SELECT h
                    FROM public.psql_bm25s_query(
                        'bm25_it.docs_tokens_bm25_idx'::regclass,
                        'cat',
                        3,
                        NULL
                    ) AS h
                ),
            1.0,
            3
            ) AS h
        )) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT
        (r).doc_ids::text,
        (r).scores::text
    FROM (
        SELECT bm25_it.search(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            3,
            NULL,
            true,
            ARRAY['and']::text[]
        ) AS r
    ) s
) TO STDOUT;

COPY (
    SELECT id, score
    FROM public.psql_bm25s_query_prepared(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ),
        3,
        NULL
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens
    WHERE public.psql_bm25s_match_prepared_query(
        tokens,
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        )
    )
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens
    WHERE public.psql_bm25s_match_query(
        tokens,
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'CAT and BIRD',
        true,
        ARRAY['and']::text[]
    )
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens
    WHERE tokens @@@ public.psql_bm25s_prepared_query(
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'CAT and BIRD',
        true,
        ARRAY['and']::text[]
    )
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id, score
    FROM public.psql_bm25s_query_prepared(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        ),
        3,
        NULL
    ) h
    JOIN bm25_it.docs_tokens d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT id, public.psql_bm25s_score_query(
        tokens,
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'CAT and BIRD',
        true,
        ARRAY['and']::text[]
    )
    FROM bm25_it.docs_tokens
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT id, public.psql_bm25s_score_prepared_query(
        tokens,
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'CAT and BIRD',
            true,
            ARRAY['and']::text[]
        )
    )
    FROM bm25_it.docs_tokens
    ORDER BY id
) TO STDOUT;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ '+cat -bird'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ '"cat bird"'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat AND bird'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat AND NOT bird'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat OR bird AND missing'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat AND (bird OR dog)'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat AND NOT (bird OR dog)'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ '"cat bird" OR (cat AND feline)'
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'NOT bird'
ORDER BY id;

SET enable_seqscan = off;
SET enable_indexscan = off;

EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat';

\pset format unaligned
\pset tuples_only on
EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@@ public.psql_bm25s_prepared_query(
    'bm25_it.docs_tokens_bm25_idx'::regclass,
    'CAT and BIRD',
    true,
    ARRAY['and']::text[]
);
\pset format aligned
\pset tuples_only off

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ '"cat bird"'
ORDER BY id;

RESET enable_indexscan;
RESET enable_seqscan;

SET enable_seqscan = off;
SET enable_bitmapscan = off;

EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat';

RESET enable_bitmapscan;
RESET enable_seqscan;

SET enable_seqscan = off;
SET enable_bitmapscan = off;

EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat'
ORDER BY tokens <=> ARRAY['bird','cat','missing']::text[] ASC
LIMIT 2;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ 'cat'
ORDER BY tokens <=> ARRAY['bird','cat','missing']::text[] ASC
LIMIT 2;

SELECT id
FROM bm25_it.docs_tokens
WHERE tokens @@ '"cat bird" OR (cat AND feline)'
ORDER BY tokens <=> ARRAY['cat','bird','feline']::text[] ASC
LIMIT 2;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens
    WHERE tokens @@@ public.psql_bm25s_prepared_query(
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'cat'
    )
    ORDER BY tokens <=> public.psql_bm25s_order_tokens(
        public.psql_bm25s_prepared_query(
            'bm25_it.docs_tokens_bm25_idx'::regclass,
            'bird cat missing'
        )
    ) ASC
    LIMIT 2
) TO STDOUT;

RESET enable_bitmapscan;
RESET enable_seqscan;

CREATE TABLE docs_tokens_candidate (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_tokens_candidate VALUES
    (1, ARRAY['cat']),
    (2, ARRAY['cat','bird']),
    (3, ARRAY['dog']),
    (4, ARRAY['fish']),
    (5, ARRAY['horse']),
    (6, ARRAY['goat']),
    (7, ARRAY['lizard']),
    (8, ARRAY['whale']);

CREATE INDEX docs_tokens_candidate_bm25_idx
    ON docs_tokens_candidate USING psql_bm25s (tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

SET enable_seqscan = off;
SET enable_bitmapscan = off;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens_candidate
    WHERE tokens @@ 'cat'
    ORDER BY tokens <=> ARRAY['bird']::text[] ASC
    LIMIT 2
) TO STDOUT;

RESET enable_bitmapscan;
RESET enable_seqscan;

CREATE TABLE docs_tokens_candidate_mid (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_tokens_candidate_mid VALUES
    (1, ARRAY['common','alpha']),
    (2, ARRAY['common','beta']),
    (3, ARRAY['common','gamma']),
    (4, ARRAY['common','delta']),
    (5, ARRAY['rare','epsilon']),
    (6, ARRAY['rare2','zeta']),
    (7, ARRAY['rare3','eta']),
    (8, ARRAY['rare4','theta']);

CREATE INDEX docs_tokens_candidate_mid_bm25_idx
    ON docs_tokens_candidate_mid USING psql_bm25s (tokens)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5
    );

SET enable_seqscan = off;
SET enable_bitmapscan = off;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens_candidate_mid
    WHERE tokens @@ 'common'
    ORDER BY tokens <=> ARRAY['alpha','common']::text[] ASC
    LIMIT 4
) TO STDOUT;

RESET enable_bitmapscan;
RESET enable_seqscan;

SELECT public.psql_bm25s_tokenize_text(
    'Cat, and dog!',
    true,
    ARRAY['and']::text[]
);

SELECT public.psql_bm25s_normalize_tokens(
    ARRAY['Cat','The','Bird']::text[],
    true,
    ARRAY['the']::text[]
);

SELECT public.psql_bm25s_normalize_tokens(
    ARRAY['Running','runs','THE']::text[],
    true,
    ARRAY['the']::text[],
    true
);

COPY (
    SELECT public.psql_bm25s_match_query(
        ARRAY['cat','bird','bird']::text[],
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'CAT and BIRD',
        true,
        ARRAY['and']::text[]
    )
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_highlight(
        ARRAY['cat','bird','bird']::text[],
        'bi* cat',
        '<mark>',
        '</mark>'
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_highlight(
        ARRAY['cat','bird','bird']::text[],
        'CAT and BIRD',
        '<mark>',
        '</mark>',
        true,
        ARRAY['and']::text[]
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_highlight(
        'cat bird bird',
        'bi* cat',
        '<mark>',
        '</mark>'
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_highlight(
        'Café bird',
        'CAFE and BIRD',
        '<mark>',
        '</mark>',
        true,
        ARRAY['and']::text[],
        false,
        true
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_snippet(
        ARRAY['zero','one','two','cat','bird','three','four']::text[],
        '"cat bird"',
        4,
        '<mark>',
        '</mark>'
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_snippet(
        ARRAY['zero','one','two','cat','bird','three','four']::text[],
        '"CAT BIRD"',
        4,
        '<mark>',
        '</mark>',
        true,
        NULL::text[]
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_snippet(
        'zero one two cat bird three four',
        '"cat bird"',
        4,
        '<mark>',
        '</mark>'
    ) AS value
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_snippet(
        'zero one two Café bird three four',
        '"CAFE BIRD"',
        4,
        '<mark>',
        '</mark>',
        true,
        NULL::text[],
        false,
        true
    ) AS value
) TO STDOUT;

SELECT public.psql_bm25s_tokenize_text(
    'Running, runs! THE',
    true,
    ARRAY['the']::text[],
    true
);

SELECT public.psql_bm25s_normalize_tokens(
    ARRAY['CAFÉ','Straße','Über']::text[],
    true,
    NULL::text[],
    false,
    true
);

SELECT public.psql_bm25s_tokenize_text(
    'Crème brûlée façade',
    true,
    NULL::text[],
    false,
    true
);

CREATE TABLE docs_tokens_stem (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_tokens_stem VALUES
    (
        1,
        public.psql_bm25s_tokenize_text(
            'Running with dogs',
            true,
            NULL::text[],
            true
        )
    ),
    (
        2,
        public.psql_bm25s_tokenize_text(
            'He runs daily',
            true,
            NULL::text[],
            true
        )
    ),
    (
        3,
        public.psql_bm25s_tokenize_text(
            'Bird song',
            true,
            NULL::text[],
            true
        )
    );

CREATE INDEX docs_tokens_stem_bm25_idx
    ON docs_tokens_stem USING psql_bm25s (tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

SELECT id
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_stem_bm25_idx'::regclass,
    'running',
    2,
    NULL,
    true,
    NULL::text[],
    true
) h
JOIN bm25_it.docs_tokens_stem d ON d.ctid = h.ctid
WHERE score > 0
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens_stem
WHERE public.psql_bm25s_match_query(
    tokens,
    'bm25_it.docs_tokens_stem_bm25_idx'::regclass,
    'running',
    true,
    NULL::text[],
    true
)
ORDER BY id;

CREATE TABLE docs_tokens_fold (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_tokens_fold VALUES
    (
        1,
        public.psql_bm25s_tokenize_text(
            'Café crème',
            true,
            NULL::text[],
            false,
            true
        )
    ),
    (
        2,
        public.psql_bm25s_tokenize_text(
            'Straße Berlin',
            true,
            NULL::text[],
            false,
            true
        )
    ),
    (
        3,
        public.psql_bm25s_tokenize_text(
            'Resume writing',
            true,
            NULL::text[],
            false,
            true
        )
    );

CREATE INDEX docs_tokens_fold_bm25_idx
    ON docs_tokens_fold USING psql_bm25s (tokens)
    WITH (method = 'lucene', idf_method = 'lucene', k1 = 1.5, b = 0.75, delta = 0.5);

SELECT id
FROM public.psql_bm25s_query(
    'bm25_it.docs_tokens_fold_bm25_idx'::regclass,
    'cafe',
    2,
    NULL,
    true,
    NULL::text[],
    false,
    true
) h
JOIN bm25_it.docs_tokens_fold d ON d.ctid = h.ctid
WHERE score > 0
ORDER BY id;

SELECT id
FROM bm25_it.docs_tokens_fold
WHERE public.psql_bm25s_match_query(
    tokens,
    'bm25_it.docs_tokens_fold_bm25_idx'::regclass,
    'strasse',
    true,
    NULL::text[],
    false,
    true
)
ORDER BY id;

COPY (
    SELECT public.psql_bm25s_highlight(
        ARRAY['cafe','creme']::text[],
        'CAFÉ',
        '<mark>',
        '</mark>',
        true,
        NULL::text[],
        false,
        true
    ) AS value
) TO STDOUT;

SET enable_seqscan = off;
SET enable_sort = off;

EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_tokens
ORDER BY tokens <=> ARRAY['bird','cat','missing']::text[] ASC
LIMIT 3;

SELECT id
FROM bm25_it.docs_tokens
ORDER BY tokens <=> ARRAY['bird','cat','missing']::text[] ASC
LIMIT 3;

COPY (
    SELECT id
    FROM bm25_it.docs_tokens
    ORDER BY tokens <=> public.psql_bm25s_order_tokens(
        'bm25_it.docs_tokens_bm25_idx'::regclass,
        'bird cat missing'
    ) ASC
    LIMIT 3
) TO STDOUT;

RESET enable_sort;
RESET enable_seqscan;

CREATE TABLE docs_ids (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_bm25_idx
    ON docs_ids USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true,
        consistency = 'manual'
    );

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    SELECT id::text || '|' || score::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    ) h
    JOIN bm25_it.docs_ids d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT doc_ids::text || '|' || scores::text
    FROM bm25_it.search_ids(
        'bm25_it.docs_ids_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    )
) TO STDOUT;

SET enable_seqscan = off;
SET enable_sort = off;

EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_ids
ORDER BY token_ids <=> ARRAY[0,2]::int4[] ASC
LIMIT 3;

SELECT id
FROM bm25_it.docs_ids
ORDER BY token_ids <=> ARRAY[0,2]::int4[] ASC
LIMIT 3;

RESET enable_sort;
RESET enable_seqscan;

INSERT INTO docs_ids VALUES (5, ARRAY[0,4]);
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_policy_recommend(
        'bm25_it.docs_ids_bm25_idx'::regclass,
        'query_first'
    )
) TO STDOUT;

COPY (
    SELECT id::text || '|' || score::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    ) h
    JOIN bm25_it.docs_ids d ON d.ctid = h.ctid
    ORDER BY score DESC, id
) TO STDOUT;

COPY (
    SELECT doc_ids::text || '|' || scores::text
    FROM bm25_it.search_ids(
        'bm25_it.docs_ids_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    )
) TO STDOUT;

CREATE TEMP TABLE generation_epoch_probe AS
SELECT substring(
    public.psql_bm25s_generation_cache_state(
        'bm25_it.docs_ids_bm25_idx'::regclass
    ) FROM 'cache_epoch=([0-9]+)'
)::bigint AS epoch;

COPY (
    SELECT public.psql_bm25s_index_refresh(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    SELECT substring(
        public.psql_bm25s_generation_cache_state(
            'bm25_it.docs_ids_bm25_idx'::regclass
        ) FROM 'cache_epoch=([0-9]+)'
    )::bigint > (SELECT epoch FROM generation_epoch_probe)
) TO STDOUT;

DROP TABLE generation_epoch_probe;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT id, score
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
) h
JOIN bm25_it.docs_ids d ON d.ctid = h.ctid
ORDER BY score DESC, id;

SET enable_seqscan = off;
SET enable_sort = off;

EXPLAIN (COSTS OFF)
SELECT id
FROM bm25_it.docs_ids
ORDER BY token_ids <=> ARRAY[0,2]::int4[] ASC
LIMIT 3;

SELECT id
FROM bm25_it.docs_ids
ORDER BY token_ids <=> ARRAY[0,2]::int4[] ASC
LIMIT 3;

RESET enable_sort;
RESET enable_seqscan;

CREATE TABLE docs_ids_auto (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_bm25_idx
    ON docs_ids_auto USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true
    );

INSERT INTO docs_ids_auto VALUES (5, ARRAY[0,4]);
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    WITH advice AS (
        SELECT public.psql_bm25s_fast_path_advice(
            'bm25_it.docs_ids_auto_bm25_idx'::regclass
        ) AS advice
    )
    SELECT
        advice->>'access_method',
        advice->>'doc_type',
        advice->>'supports_plain_match',
        advice->>'supports_prepared_match',
        advice->>'supports_ordering',
        advice->>'supports_filtered_ranked',
        advice->>'canonical_api',
        COALESCE(advice->>'recommended_filter', '<null>'),
        advice->>'recommended_order'
    FROM advice
) TO STDOUT;

SET enable_seqscan = off;
COPY (
    WITH explain_info AS (
        SELECT public.psql_bm25s_fast_path_explain(
            'bm25_it.docs_ids_auto_bm25_idx'::regclass,
            $q$
                SELECT id
                FROM bm25_it.docs_ids_auto
                ORDER BY token_ids <=> ARRAY[0,2] ASC
                LIMIT 3
            $q$
        ) AS explain_info
    )
    SELECT
        explain_info->>'used_psql_bm25s_index',
        explain_info->>'used_index_scan',
        explain_info->>'used_bitmap_index_scan',
        explain_info->>'used_ordered_index_scan',
        explain_info->>'uses_plain_match',
        explain_info->>'uses_ordering_operator'
    FROM explain_info
) TO STDOUT;
RESET enable_seqscan;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT id, score
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
) h
JOIN bm25_it.docs_ids_auto d ON d.ctid = h.ctid
ORDER BY score DESC, id;

COPY (
    SELECT doc_ids, scores
    FROM bm25_it.search_ids(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    )
) TO STDOUT;

COPY (
    SELECT current_setting('psql_bm25s.workspace_cache_bytes') IS NOT NULL
) TO STDOUT;
COPY (
    SELECT current_setting('psql_bm25s.workspace_idle_timeout') IS NOT NULL
) TO STDOUT;

SET psql_bm25s.workspace_cache_bytes = '0';
SET psql_bm25s.workspace_idle_timeout = '0ms';

COPY (
    SELECT count(*)
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    ) h
) TO STDOUT;

RESET psql_bm25s.workspace_cache_bytes;
RESET psql_bm25s.workspace_idle_timeout;

SET psql_bm25s.workspace_cache_bytes = '-1';
SET psql_bm25s.workspace_idle_timeout = '-1';
RESET psql_bm25s.workspace_cache_bytes;
RESET psql_bm25s.workspace_idle_timeout;

SET search_path = bm25_it;
DO $$
BEGIN
    PERFORM *
    FROM public.psql_bm25s_index_maintain_due(1);
END
$$;
COPY (
    SELECT 'maintain_due_search_path_ok'
) TO STDOUT;
SET search_path = bm25_it, public;

CREATE TABLE docs_text_eager_unchanged (
    id int primary key,
    tokens text[] not null,
    abstract text
);

INSERT INTO docs_text_eager_unchanged VALUES
    (1, ARRAY['cancer','therapy'], NULL),
    (2, ARRAY['cancer','trial'], NULL),
    (3, ARRAY['bird'], NULL);

CREATE INDEX docs_text_eager_unchanged_bm25_idx
    ON docs_text_eager_unchanged USING psql_bm25s (tokens)
    WITH (
        auto_rebuild_threshold = 0,
        auto_rebuild_delta_bytes = 0,
        auto_rebuild_churn_ratio = 0
    );

BEGIN;
UPDATE docs_text_eager_unchanged
SET abstract = 'updated'
WHERE id = 1;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_eager_unchanged_bm25_idx'::regclass
    )::text
) TO STDOUT;
COMMIT;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_eager_unchanged_bm25_idx'::regclass
    )::text
) TO STDOUT;

COPY (
    SELECT doc_ids::text || '|' || scores::text
    FROM bm25_it.search_query(
        'bm25_it.docs_text_eager_unchanged_bm25_idx'::regclass,
        'cancer',
        2,
        NULL
    )
) TO STDOUT;

CREATE TABLE docs_expr_eager_commit (
    id int primary key,
    title text not null,
    body text
);

INSERT INTO docs_expr_eager_commit VALUES
    (1, repeat('cancer therapy ', 400), NULL),
    (2, repeat('bird migration ', 400), NULL);

CREATE INDEX docs_expr_eager_commit_bm25_idx
    ON docs_expr_eager_commit USING psql_bm25s (
        psql_bm25s_tokenize_text(
            COALESCE(title, ''::text),
            true,
            NULL::text[],
            false,
            true
        )
    )
    WITH (
        auto_rebuild_threshold = 0,
        auto_rebuild_delta_bytes = 0,
        auto_rebuild_churn_ratio = 0
    );

BEGIN;
UPDATE docs_expr_eager_commit
SET body = repeat('updated abstract ', 400)
WHERE id = 1;
COMMIT;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_expr_eager_commit_bm25_idx'::regclass
    )::text
) TO STDOUT;

COPY (
    SELECT doc_ids::text || '|' || scores::text
    FROM bm25_it.search_query(
        'bm25_it.docs_expr_eager_commit_bm25_idx'::regclass,
        'cancer',
        1,
        NULL
    )
) TO STDOUT;

CREATE TABLE docs_ids_ties (
    id integer PRIMARY KEY,
    token_ids integer[] NOT NULL
);

INSERT INTO docs_ids_ties VALUES
    (1, ARRAY[0]),
    (2, ARRAY[0]),
    (3, ARRAY[0]),
    (4, ARRAY[1]);

CREATE INDEX docs_ids_ties_bm25_idx
ON docs_ids_ties USING psql_bm25s (token_ids)
WITH (
    method = 'lucene',
    idf_method = 'lucene',
    k1 = 1.5,
    b = 0.75,
    delta = 0.5,
    create_empty_token = true,
    consistency = 'manual'
);

COPY (
    SELECT doc_ids::text || '|' || scores::text AS tie_result
    FROM bm25_it.search_ids(
        'bm25_it.docs_ids_ties_bm25_idx'::regclass,
        ARRAY[0],
        2,
        NULL
    )
) TO STDOUT;

CREATE TABLE docs_ids_sparse_paths (
    id integer PRIMARY KEY,
    token_ids integer[] NOT NULL
);

INSERT INTO docs_ids_sparse_paths VALUES
    (1, ARRAY[0]),
    (2, ARRAY[1]),
    (3, ARRAY[0,1]);

CREATE INDEX docs_ids_sparse_paths_bm25_idx
ON docs_ids_sparse_paths USING psql_bm25s (token_ids)
WITH (
    method = 'lucene',
    idf_method = 'lucene',
    k1 = 1.5,
    b = 0.75,
    delta = 0.5,
    create_empty_token = true,
    consistency = 'manual'
);

COPY (
    SELECT array_agg(d.id ORDER BY h.score DESC, d.id)::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_sparse_paths_bm25_idx'::regclass,
        ARRAY[0],
        3,
        NULL
    ) h
    JOIN bm25_it.docs_ids_sparse_paths d ON d.ctid = h.ctid
) TO STDOUT;

COPY (
    SELECT array_agg(d.id ORDER BY h.score DESC, d.id)::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_sparse_paths_bm25_idx'::regclass,
        ARRAY[0],
        3,
        ARRAY[-1.0, 1.0, 1.0]::real[]
    ) h
    JOIN bm25_it.docs_ids_sparse_paths d ON d.ctid = h.ctid
) TO STDOUT;

CREATE TABLE docs_ids_zero_idf (
    id integer PRIMARY KEY,
    token_ids integer[] NOT NULL
);

INSERT INTO docs_ids_zero_idf VALUES
    (1, ARRAY[0]),
    (2, ARRAY[0]),
    (3, ARRAY[0]);

CREATE INDEX docs_ids_zero_idf_bm25_idx
ON docs_ids_zero_idf USING psql_bm25s (token_ids)
WITH (
    method = 'robertson',
    idf_method = 'robertson',
    k1 = 1.5,
    b = 0.75,
    delta = 0.5,
    create_empty_token = true,
    consistency = 'manual'
);

COPY (
    SELECT
        array_agg(d.id ORDER BY h.score DESC, d.id)::text || '|' ||
        COALESCE(sum(h.score), 0)::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_zero_idf_bm25_idx'::regclass,
        ARRAY[0,0,0],
        3,
        NULL
    ) h
    JOIN bm25_it.docs_ids_zero_idf d ON d.ctid = h.ctid
) TO STDOUT;

CREATE TABLE docs_ids_mvcc_sparse_expand (
    id integer PRIMARY KEY,
    token_ids integer[] NOT NULL
);

INSERT INTO docs_ids_mvcc_sparse_expand VALUES
    (1, ARRAY[0]),
    (2, ARRAY[0]),
    (3, ARRAY[0]),
    (4, ARRAY[0]),
    (5, ARRAY[0]),
    (6, ARRAY[1]);

CREATE INDEX docs_ids_mvcc_sparse_expand_bm25_idx
ON docs_ids_mvcc_sparse_expand USING psql_bm25s (token_ids)
WITH (
    method = 'lucene',
    idf_method = 'lucene',
    k1 = 1.5,
    b = 0.75,
    delta = 0.5,
    consistency = 'manual'
);

DELETE FROM docs_ids_mvcc_sparse_expand WHERE id = 1;

COPY (
    SELECT array_agg(d.id ORDER BY h.score DESC, d.id)::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_mvcc_sparse_expand_bm25_idx'::regclass,
        ARRAY[0],
        3,
        NULL
    ) h
    JOIN bm25_it.docs_ids_mvcc_sparse_expand d ON d.ctid = h.ctid
) TO STDOUT;

SET enable_seqscan = off;
SET enable_sort = off;

COPY (
    SELECT array_agg(id ORDER BY id)::text
    FROM (
        SELECT id
        FROM bm25_it.docs_ids_zero_idf
        ORDER BY token_ids <=> ARRAY[0,0,0]::int4[] ASC
        LIMIT 3
    ) ranked
) TO STDOUT;

RESET enable_sort;
RESET enable_seqscan;

CREATE TABLE docs_ids_auto_txn (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_txn VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_txn_bm25_idx
    ON docs_ids_auto_txn USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true
    );

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_txn_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    SELECT
        array_agg(d.id ORDER BY h.score DESC, d.id)::text || '|' ||
        array_agg(round(h.score::numeric, 6)::text
            ORDER BY h.score DESC, d.id)::text
    FROM public.psql_bm25s_query_ids(
        'bm25_it.docs_ids_auto_txn_bm25_idx'::regclass,
        ARRAY[0,2],
        4,
        ARRAY[1.0, 0.0, -1.0, 1.0]::real[]
    ) h
    JOIN bm25_it.docs_ids_auto_txn d ON d.ctid = h.ctid
) TO STDOUT;

BEGIN;
INSERT INTO docs_ids_auto_txn VALUES (5, ARRAY[0,4]);
UPDATE docs_ids_auto_txn
SET token_ids = ARRAY[0,2,2,4]
WHERE id = 2;
COMMIT;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_txn_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_txn_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT id, score
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_txn_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
) h
JOIN bm25_it.docs_ids_auto_txn d ON d.ctid = h.ctid
ORDER BY score DESC, id;

SELECT *
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_txn_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
);

CREATE TABLE docs_ids_auto_query (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_query VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_query_bm25_idx
    ON docs_ids_auto_query USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true
    );

BEGIN;
INSERT INTO docs_ids_auto_query VALUES (5, ARRAY[0,4]);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_query_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT *
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_query_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_query_bm25_idx'::regclass
    )
) TO STDOUT;
COMMIT;

CREATE TABLE docs_ids_auto_threshold (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_threshold VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_threshold_bm25_idx
    ON docs_ids_auto_threshold USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true,
        auto_rebuild_threshold = 10
    );

INSERT INTO docs_ids_auto_threshold VALUES (5, ARRAY[0,4]);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_policy_recommend(
        'bm25_it.docs_ids_auto_threshold_bm25_idx'::regclass,
        'write_tolerant_query_first'
    )
) TO STDOUT;

SELECT *
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_threshold_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TABLE docs_text_auto_threshold (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_text_auto_threshold VALUES
    (1, ARRAY['cancer','therapy']),
    (2, ARRAY['bird','migration']),
    (3, ARRAY['common','baseline']);

CREATE INDEX docs_text_auto_threshold_bm25_idx
    ON docs_text_auto_threshold USING psql_bm25s (tokens)
    WITH (
        method = 'lucene',
        idf_method = 'lucene',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        auto_rebuild_threshold = 10
    );

INSERT INTO docs_text_auto_threshold VALUES (4, ARRAY['fresh','delta']);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_auto_threshold_bm25_idx'::regclass
    )::text
) TO STDOUT;

COPY (
    SELECT d.id, h.score
    FROM public.psql_bm25s_query_tokens(
        'bm25_it.docs_text_auto_threshold_bm25_idx'::regclass,
        ARRAY['fresh'],
        3,
        NULL
    ) h
    JOIN bm25_it.docs_text_auto_threshold d ON d.ctid = h.ctid
    ORDER BY h.score DESC, d.id
) TO STDOUT;

CREATE TABLE docs_ids_auto_threshold_update (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_threshold_update VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_threshold_update_bm25_idx
    ON docs_ids_auto_threshold_update USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true,
        auto_rebuild_threshold = 10
    );

UPDATE docs_ids_auto_threshold_update
SET token_ids = ARRAY[0,4]
WHERE id = 1;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_update_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT *
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_threshold_update_bm25_idx'::regclass,
    ARRAY[0,4],
    2,
    NULL
);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_update_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TABLE docs_ids_auto_threshold_bytes (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_threshold_bytes VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_threshold_bytes_bm25_idx
    ON docs_ids_auto_threshold_bytes USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true,
        auto_rebuild_threshold = 10,
        auto_rebuild_delta_bytes = 16
    );

INSERT INTO docs_ids_auto_threshold_bytes VALUES (5, ARRAY[0,4]);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_bytes_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_policy_recommend(
        'bm25_it.docs_ids_auto_threshold_bytes_bm25_idx'::regclass,
        'heavy_mixed_churn'
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_policy_recommend(
        'bm25_it.docs_ids_auto_threshold_bytes_bm25_idx'::regclass,
        'heavy_insert_skew'
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_bytes_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT *
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_threshold_bytes_bm25_idx'::regclass,
    ARRAY[0,4],
    2,
    NULL
);

CREATE TABLE docs_ids_auto_threshold_ratio (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_threshold_ratio VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_threshold_ratio_bm25_idx
    ON docs_ids_auto_threshold_ratio USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true,
        auto_rebuild_threshold = 10,
        auto_rebuild_delta_bytes = 1000,
        auto_rebuild_churn_ratio = 0.20
    );

INSERT INTO docs_ids_auto_threshold_ratio VALUES (5, ARRAY[0,4]);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_ratio_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_policy_recommend(
        'bm25_it.docs_ids_auto_threshold_ratio_bm25_idx'::regclass,
        'small_mixed_churn'
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_policy_recommend(
        'bm25_it.docs_ids_auto_threshold_ratio_bm25_idx'::regclass,
        'longrun_mixed_churn'
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_ratio_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TABLE docs_ids_eventual_policy (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_eventual_policy VALUES
    (1, ARRAY[0,1]),
    (2, ARRAY[1,2]);

CREATE INDEX docs_ids_eventual_policy_bm25_idx
    ON docs_ids_eventual_policy USING psql_bm25s (token_ids)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 1000,
        query_overlay_max_records = 1234,
        query_overlay_max_bytes = 5678
    );

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_eventual_policy_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TABLE docs_default_policy (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_default_policy VALUES
    (1, ARRAY['default','policy']);

CREATE INDEX docs_default_policy_bm25_idx
    ON docs_default_policy USING psql_bm25s (tokens);

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_default_policy_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TABLE docs_eventual_default_policy (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_eventual_default_policy VALUES
    (1, ARRAY['eventual','default']);

CREATE INDEX docs_eventual_default_policy_bm25_idx
    ON docs_eventual_default_policy USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual'
    );

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_eventual_default_policy_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TABLE docs_manual_payload_policy (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_manual_payload_policy VALUES
    (1, ARRAY['manual','base']),
    (2, ARRAY['manual','delete']);

CREATE INDEX docs_manual_payload_policy_bm25_idx
    ON docs_manual_payload_policy USING psql_bm25s (tokens)
    WITH (
        consistency = 'manual'
    );

INSERT INTO docs_manual_payload_policy VALUES
    (3, ARRAY['manual','insert']);

COPY (
    WITH state AS (
        SELECT *
        FROM public.psql_bm25s_index_details(
            'bm25_it.docs_manual_payload_policy_bm25_idx'::regclass
        )
    )
    SELECT
        pending_writes = 1 AND
        pending_deletes = 0 AND
        delta_records = 0 AND
        delta_bytes = 0 AND
        stale AS manual_insert_is_lightweight
    FROM state
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_index_refresh(
        'bm25_it.docs_manual_payload_policy_bm25_idx'::regclass
    ) IS NOT NULL AS manual_refresh_ok
) TO STDOUT;

DELETE FROM docs_manual_payload_policy
WHERE id = 2;

VACUUM (INDEX_CLEANUP ON) docs_manual_payload_policy;

COPY (
    WITH state AS (
        SELECT *
        FROM public.psql_bm25s_index_details(
            'bm25_it.docs_manual_payload_policy_bm25_idx'::regclass
        )
    )
    SELECT
        pending_writes = 0 AND
        pending_deletes = 1 AND
        delta_records = 0 AND
        delta_bytes = 0 AND
        stale AS manual_delete_is_lightweight
    FROM state
) TO STDOUT;

CREATE TABLE docs_eventual_large_delta_policy (
    id int primary key,
    body text not null
);

INSERT INTO docs_eventual_large_delta_policy VALUES
    (1, 'eventual base');

CREATE INDEX docs_eventual_large_delta_policy_bm25_idx
    ON docs_eventual_large_delta_policy USING psql_bm25s (body)
    WITH (
        consistency = 'eventual'
    );

INSERT INTO docs_eventual_large_delta_policy VALUES
    (2, repeat('eventual oversized payload ', 1200));

COPY (
    WITH state AS (
        SELECT *
        FROM public.psql_bm25s_index_details(
            'bm25_it.docs_eventual_large_delta_policy_bm25_idx'::regclass
        )
    )
    SELECT
        docs = 1 AND
        pending_writes = 1 AND
        pending_deletes = 0 AND
        delta_records = 0 AND
        delta_bytes = 0 AND
        NOT stale AS eventual_large_delta_is_freshness_debt
    FROM state
) TO STDOUT;

CREATE TABLE docs_realtime_large_delta_policy (
    id int primary key,
    body text not null
);

INSERT INTO docs_realtime_large_delta_policy VALUES
    (1, 'realtime base');

CREATE INDEX docs_realtime_large_delta_policy_bm25_idx
    ON docs_realtime_large_delta_policy USING psql_bm25s (body)
    WITH (
        consistency = 'realtime',
        auto_rebuild_threshold = 10
    );

INSERT INTO docs_realtime_large_delta_policy VALUES
    (2, repeat('realtime oversized payload ', 1200));

COPY (
    WITH state AS (
        SELECT *
        FROM public.psql_bm25s_index_details(
            'bm25_it.docs_realtime_large_delta_policy_bm25_idx'::regclass
        )
    )
    SELECT
        docs = 2 AND
        pending_writes = 0 AND
        pending_deletes = 0 AND
        delta_records = 0 AND
        delta_bytes = 0 AND
        NOT stale AS realtime_large_delta_refreshes
    FROM state
) TO STDOUT;

CREATE TABLE docs_manual_large_delta_policy (
    id int primary key,
    body text not null
);

INSERT INTO docs_manual_large_delta_policy VALUES
    (1, 'manual base');

CREATE INDEX docs_manual_large_delta_policy_bm25_idx
    ON docs_manual_large_delta_policy USING psql_bm25s (body)
    WITH (
        consistency = 'manual'
    );

INSERT INTO docs_manual_large_delta_policy VALUES
    (2, repeat('manual oversized payload ', 1200));

COPY (
    WITH state AS (
        SELECT *
        FROM public.psql_bm25s_index_details(
            'bm25_it.docs_manual_large_delta_policy_bm25_idx'::regclass
        )
    )
    SELECT
        docs = 1 AND
        pending_writes = 1 AND
        pending_deletes = 0 AND
        delta_records = 0 AND
        delta_bytes = 0 AND
        stale AS manual_large_delta_is_lightweight
    FROM state
) TO STDOUT;

CREATE TABLE docs_text_eventual_budget (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_text_eventual_budget VALUES
    (1, ARRAY['base','alpha']),
    (2, ARRAY['base','beta']);

CREATE INDEX docs_text_eventual_budget_bm25_idx
    ON docs_text_eventual_budget USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 10,
        query_overlay_max_records = 1
    );

INSERT INTO docs_text_eventual_budget VALUES
    (3, ARRAY['eventual','first']);

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_eventual_budget_bm25_idx'::regclass,
        'eventual',
        10,
        NULL,
        true,
        NULL,
        false,
        false
    ) h
    JOIN bm25_it.docs_text_eventual_budget d ON d.ctid = h.ctid
    WHERE h.score > 0
    ORDER BY id
) TO STDOUT;

INSERT INTO docs_text_eventual_budget VALUES
    (4, ARRAY['eventual','second']);

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_eventual_budget_bm25_idx'::regclass,
        'eventual',
        10,
        NULL,
        true,
        NULL,
        false,
        false
    ) h
    JOIN bm25_it.docs_text_eventual_budget d ON d.ctid = h.ctid
    WHERE h.score > 0
    ORDER BY id
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_index_refresh(
        'bm25_it.docs_text_eventual_budget_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_eventual_budget_bm25_idx'::regclass,
        'eventual',
        10,
        NULL,
        true,
        NULL,
        false,
        false
    ) h
    JOIN bm25_it.docs_text_eventual_budget d ON d.ctid = h.ctid
    WHERE h.score > 0
    ORDER BY id
) TO STDOUT;

CREATE TABLE docs_text_exact_budget (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_text_exact_budget VALUES
    (1, ARRAY['base','alpha']),
    (2, ARRAY['base','beta']);

CREATE INDEX docs_text_exact_budget_bm25_idx
    ON docs_text_exact_budget USING psql_bm25s (tokens)
    WITH (
        auto_rebuild_threshold = 10
    );

INSERT INTO docs_text_exact_budget VALUES
    (3, ARRAY['exact','first']),
    (4, ARRAY['exact','second']);

COPY (
    SELECT id
    FROM public.psql_bm25s_query(
        'bm25_it.docs_text_exact_budget_bm25_idx'::regclass,
        'exact',
        10,
        NULL,
        true,
        NULL,
        false,
        false
    ) h
    JOIN bm25_it.docs_text_exact_budget d ON d.ctid = h.ctid
    WHERE h.score > 0
    ORDER BY id
) TO STDOUT;

CREATE TABLE docs_text_exact_delete_budget (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_text_exact_delete_budget VALUES
    (1, ARRAY['deleteprobe','keep']),
    (2, ARRAY['deleteprobe','gone']),
    (3, ARRAY['other','term']);

CREATE INDEX docs_text_exact_delete_budget_bm25_idx
    ON docs_text_exact_delete_budget USING psql_bm25s (tokens)
    WITH (
        auto_rebuild_threshold = 10
    );

DELETE FROM docs_text_exact_delete_budget
WHERE id = 2;

VACUUM (INDEX_CLEANUP ON) docs_text_exact_delete_budget;

CREATE TEMP TABLE docs_text_exact_delete_state AS
SELECT *
FROM public.psql_bm25s_index_details(
    'bm25_it.docs_text_exact_delete_budget_bm25_idx'::regclass
);

CREATE TEMP TABLE docs_text_exact_delete_before AS
SELECT d.id, h.score
FROM public.psql_bm25s_query(
    'bm25_it.docs_text_exact_delete_budget_bm25_idx'::regclass,
    'deleteprobe',
    10,
    NULL,
    true,
    NULL,
    false,
    false
) h
JOIN bm25_it.docs_text_exact_delete_budget d ON d.ctid = h.ctid
WHERE h.score > 0;

COPY (
    SELECT public.psql_bm25s_index_refresh(
        'bm25_it.docs_text_exact_delete_budget_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TEMP TABLE docs_text_exact_delete_after AS
SELECT d.id, h.score
FROM public.psql_bm25s_query(
    'bm25_it.docs_text_exact_delete_budget_bm25_idx'::regclass,
    'deleteprobe',
    10,
    NULL,
    true,
    NULL,
    false,
    false
) h
JOIN bm25_it.docs_text_exact_delete_budget d ON d.ctid = h.ctid
WHERE h.score > 0;

COPY (
    SELECT
        count(*) = 1 AS ids_match,
        CASE
            WHEN bool_or(
                s.pending_deletes = 1 AND
                s.delta_records = 1
            )
            THEN coalesce(bool_and(abs(b.score - a.score) < 0.000001), false)
            ELSE true
        END AS scores_match
    FROM docs_text_exact_delete_before b
    JOIN docs_text_exact_delete_after a USING (id)
    CROSS JOIN docs_text_exact_delete_state s
) TO STDOUT;

CREATE TABLE docs_invalid_policy_options (
    id int primary key,
    tokens text[] not null
);

CREATE INDEX docs_invalid_realtime_overlay_bm25_idx
    ON docs_invalid_policy_options USING psql_bm25s (tokens)
    WITH (
        query_overlay_max_records = 1
    );

CREATE INDEX docs_invalid_manual_threshold_bm25_idx
    ON docs_invalid_policy_options USING psql_bm25s (tokens)
    WITH (
        consistency = 'manual',
        auto_rebuild_threshold = 10
    );

CREATE TABLE docs_eventual_overlay_budget (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_eventual_overlay_budget VALUES
    (1, ARRAY['overlay','alpha']);

CREATE INDEX docs_eventual_overlay_budget_bm25_idx
    ON docs_eventual_overlay_budget USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 1000,
        query_overlay_max_records = 1,
        query_overlay_max_bytes = 1
    );

INSERT INTO docs_eventual_overlay_budget VALUES
    (2, ARRAY['overlay','beta']),
    (3, ARRAY['overlay','gamma']);

COPY (
    SELECT public.psql_bm25s_index_try_maintain(
        'bm25_it.docs_eventual_overlay_budget_bm25_idx'::regclass
    ) NOT LIKE '%maintained=true%' AS overlay_budget_does_not_rebuild
) TO STDOUT;

CREATE TABLE docs_eventual_delta_bytes_due (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_eventual_delta_bytes_due VALUES
    (1, ARRAY['delta','alpha']);

CREATE INDEX docs_eventual_delta_bytes_due_bm25_idx
    ON docs_eventual_delta_bytes_due USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 0,
        auto_rebuild_delta_bytes = 1
    );

INSERT INTO docs_eventual_delta_bytes_due VALUES
    (2, ARRAY['delta','beta']);

COPY (
    SELECT public.psql_bm25s_index_try_maintain(
        'bm25_it.docs_eventual_delta_bytes_due_bm25_idx'::regclass
    ) LIKE '%maintained=true%' AS delta_bytes_triggers_rebuild
) TO STDOUT;

CREATE TABLE docs_eventual_delta_bytes_oversized (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_eventual_delta_bytes_oversized VALUES
    (1, ARRAY['oversized','base']);

CREATE INDEX docs_eventual_delta_bytes_oversized_bm25_idx
    ON docs_eventual_delta_bytes_oversized USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 0,
        auto_rebuild_delta_bytes = 1
    );

INSERT INTO docs_eventual_delta_bytes_oversized VALUES
    (2, ARRAY[repeat('oversized ', 1200)]);

COPY (
    SELECT public.psql_bm25s_index_try_maintain(
        'bm25_it.docs_eventual_delta_bytes_oversized_bm25_idx'::regclass
    ) LIKE '%maintained=true%' AS oversized_byte_policy_triggers_rebuild
) TO STDOUT;

CREATE TABLE docs_eventual_delta_bytes_delete (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_eventual_delta_bytes_delete VALUES
    (1, ARRAY['delete','alpha']),
    (2, ARRAY['delete','beta']);

CREATE INDEX docs_eventual_delta_bytes_delete_bm25_idx
    ON docs_eventual_delta_bytes_delete USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 0,
        auto_rebuild_delta_bytes = 1
    );

DELETE FROM docs_eventual_delta_bytes_delete WHERE id = 1;
VACUUM docs_eventual_delta_bytes_delete;

COPY (
    SELECT public.psql_bm25s_index_try_maintain(
        'bm25_it.docs_eventual_delta_bytes_delete_bm25_idx'::regclass
    ) LIKE '%maintained=true%' AS delete_byte_policy_triggers_rebuild
) TO STDOUT;

CREATE TABLE docs_eventual_writer_lock (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_eventual_writer_lock VALUES
    (1, ARRAY['writer','base']);

CREATE INDEX docs_eventual_writer_lock_bm25_idx
    ON docs_eventual_writer_lock USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 1000
    );

BEGIN;
INSERT INTO docs_eventual_writer_lock VALUES
    (2, ARRAY['writer','delta']);
COPY (
    SELECT NOT EXISTS (
        SELECT 1
        FROM pg_locks
        WHERE pid = pg_backend_pid()
            AND relation =
                'bm25_it.docs_eventual_writer_lock_bm25_idx'::regclass
            AND mode = 'ShareUpdateExclusiveLock'
            AND granted
    ) AS eventual_writer_does_not_pin_share_update_exclusive,
    EXISTS (
        SELECT 1
        FROM pg_locks
        WHERE pid = pg_backend_pid()
            AND relation =
                'bm25_it.docs_eventual_writer_lock_bm25_idx'::regclass
            AND mode = 'RowExclusiveLock'
            AND granted
    ) AS eventual_writer_marks_uncommitted_delta
) TO STDOUT;
ROLLBACK;

CREATE TABLE docs_text_eventual_unchanged (
    id int primary key,
    marker int not null,
    tokens text[] not null
);

INSERT INTO docs_text_eventual_unchanged VALUES
    (1, 1, ARRAY['stable','alpha']),
    (2, 2, ARRAY['stable','beta']);

CREATE INDEX docs_text_eventual_unchanged_marker_idx
    ON docs_text_eventual_unchanged (marker);

CREATE INDEX docs_text_eventual_unchanged_bm25_idx
    ON docs_text_eventual_unchanged USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 10
    );

UPDATE docs_text_eventual_unchanged
SET marker = marker + 10
WHERE id = 1;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TEMP TABLE maintain_epoch_probe AS
SELECT substring(
    public.psql_bm25s_generation_cache_state(
        'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
    ) FROM 'cache_epoch=([0-9]+)'
)::bigint AS epoch;

COPY (
    SELECT public.psql_bm25s_index_maintain(
        'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    SELECT substring(
        public.psql_bm25s_generation_cache_state(
            'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
        ) FROM 'cache_epoch=([0-9]+)'
    )::bigint > (SELECT epoch FROM maintain_epoch_probe)
) TO STDOUT;

DROP TABLE maintain_epoch_probe;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
    )
) TO STDOUT;

COPY (
    SELECT public.psql_bm25s_index_maintain(
        'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
    )
) TO STDOUT;

CREATE TEMP TABLE reindex_generation_probe AS
SELECT public.psql_bm25s_generation_cache_state(
    'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
) AS state;

REINDEX INDEX bm25_it.docs_text_eventual_unchanged_bm25_idx;

COPY (
    SELECT public.psql_bm25s_generation_cache_state(
        'bm25_it.docs_text_eventual_unchanged_bm25_idx'::regclass
    ) <> (SELECT state FROM reindex_generation_probe)
) TO STDOUT;

DROP TABLE reindex_generation_probe;

CREATE TABLE docs_text_eventual_online (
    id int primary key,
    tokens text[] not null
);

INSERT INTO docs_text_eventual_online VALUES
    (1, ARRAY['online','base']),
    (2, ARRAY['online','stable']);

CREATE INDEX docs_text_eventual_online_bm25_idx
    ON docs_text_eventual_online USING psql_bm25s (tokens)
    WITH (
        consistency = 'eventual',
        auto_rebuild_threshold = 10
    );

INSERT INTO docs_text_eventual_online VALUES
    (3, ARRAY['online','delta']);

CREATE TEMP TABLE online_epoch_probe AS
SELECT substring(
    public.psql_bm25s_generation_cache_state(
        'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
    ) FROM 'cache_epoch=([0-9]+)'
)::bigint AS epoch;

COPY (
    SELECT regexp_replace(
        public.psql_bm25s_index_try_maintain(
            'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
        )::text,
        'shared_preload_published=(true|false)',
        'shared_preload_published=<bool>'
    )
) TO STDOUT;

COPY (
    SELECT substring(
        public.psql_bm25s_generation_cache_state(
            'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
        ) FROM 'cache_epoch=([0-9]+)'
    )::bigint > (SELECT epoch FROM online_epoch_probe)
) TO STDOUT;

DROP TABLE online_epoch_probe;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
    )
) TO STDOUT;

ALTER INDEX docs_text_eventual_online_bm25_idx
    SET (auto_rebuild_threshold = 1);

INSERT INTO docs_text_eventual_online VALUES
    (4, ARRAY['online','cron']);

CREATE TEMP TABLE due_epoch_probe AS
SELECT substring(
    public.psql_bm25s_generation_cache_state(
        'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
    ) FROM 'cache_epoch=([0-9]+)'
)::bigint AS epoch;

COPY (
    SELECT index_oid::text || '|' || regexp_replace(
        result::text,
        'shared_preload_published=(true|false)',
        'shared_preload_published=<bool>'
    )
    FROM public.psql_bm25s_index_maintain_due(10)
    WHERE index_oid = 'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
) TO STDOUT;

COPY (
    SELECT substring(
        public.psql_bm25s_generation_cache_state(
            'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
        ) FROM 'cache_epoch=([0-9]+)'
    )::bigint > (SELECT epoch FROM due_epoch_probe)
) TO STDOUT;

DROP TABLE due_epoch_probe;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_text_eventual_online_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT *
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_threshold_ratio_bm25_idx'::regclass,
    ARRAY[0,4],
    2,
    NULL
);

CREATE TABLE docs_ids_auto_threshold_delete (
    id int primary key,
    token_ids int4[] not null
);

INSERT INTO docs_ids_auto_threshold_delete VALUES
    (1, ARRAY[0,0,1]),
    (2, ARRAY[1,2]),
    (3, ARRAY[0,2,2]),
    (4, ARRAY[3]);

CREATE INDEX docs_ids_auto_threshold_delete_bm25_idx
    ON docs_ids_auto_threshold_delete USING psql_bm25s (token_ids)
    WITH (
        method = 'bm25+',
        idf_method = 'bm25+',
        k1 = 1.5,
        b = 0.75,
        delta = 0.5,
        create_empty_token = true,
        auto_rebuild_threshold = 10
    );

DELETE FROM docs_ids_auto_threshold_delete
WHERE id = 2;

VACUUM (INDEX_CLEANUP ON) docs_ids_auto_threshold_delete;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_delete_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT d.id, h.score
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_threshold_delete_bm25_idx'::regclass,
    ARRAY[1,2],
    3,
    NULL
) h
JOIN bm25_it.docs_ids_auto_threshold_delete d ON d.ctid = h.ctid
ORDER BY h.score DESC, d.id;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_threshold_delete_bm25_idx'::regclass
    )
) TO STDOUT;

DELETE FROM docs_ids_auto
WHERE id = 5;

VACUUM (INDEX_CLEANUP ON) docs_ids_auto;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT id, score
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
) h
JOIN bm25_it.docs_ids_auto d ON d.ctid = h.ctid
ORDER BY score DESC, id;

COPY (
    SELECT doc_ids, scores
    FROM bm25_it.search_ids(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    )
) TO STDOUT;

UPDATE docs_ids_auto
SET token_ids = ARRAY[0,2,2,4]
WHERE id = 2;

COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass
    )
) TO STDOUT;
COPY (
    SELECT public.psql_bm25s_index_details(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass
    )
) TO STDOUT;

SELECT id, score
FROM public.psql_bm25s_query_ids(
    'bm25_it.docs_ids_auto_bm25_idx'::regclass,
    ARRAY[0,2],
    3,
    NULL
) h
JOIN bm25_it.docs_ids_auto d ON d.ctid = h.ctid
ORDER BY score DESC, id;

COPY (
    SELECT doc_ids, scores
    FROM bm25_it.search_ids(
        'bm25_it.docs_ids_auto_bm25_idx'::regclass,
        ARRAY[0,2],
        3,
        NULL
    )
) TO STDOUT;
