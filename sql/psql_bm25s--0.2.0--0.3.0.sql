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
