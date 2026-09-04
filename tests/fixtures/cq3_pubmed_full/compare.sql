\set ON_ERROR_STOP on

SELECT route, checkpoint, identity
FROM bench.cq3_full_root_identity
WHERE route IN (:'baseline_route', :'candidate_route')
ORDER BY route, checkpoint;

SELECT
    count(*) = 4 AND
    count(DISTINCT identity) = 1 AND
    bool_and(coalesce((identity->>'valid')::boolean, false)) AND
    bool_and(coalesce(
        (identity->>'runtime_contract_matches')::boolean,
        false
    )) AND
    bool_and(identity->>'health' = 'ok')
        AS root_identity_stable
FROM bench.cq3_full_root_identity
WHERE route IN (:'baseline_route', :'candidate_route')
  AND checkpoint IN ('before', 'after')
\gset
\if :root_identity_stable
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 qualification failed: root identity changed during A/B';
    END
    $failure$;
\endif

WITH expected(filter_name, expected_hits) AS (
    VALUES
        ('p006', 50),
        ('p100', 50),
        ('p500', 50),
        ('unfiltered', 50)
), expected_rank AS (
    SELECT expected.*, rank
    FROM expected
    CROSS JOIN LATERAL generate_series(1, expected.expected_hits) AS rank
), compared AS (
    SELECT
        expected_rank.filter_name,
        expected_rank.expected_hits,
        count(baseline.rank) AS baseline_hits,
        count(candidate.rank) AS candidate_hits,
        count(*) FILTER (
            WHERE baseline.doc_ord IS DISTINCT FROM candidate.doc_ord
               OR float8send(baseline.score) IS DISTINCT FROM
                    float8send(candidate.score)
        ) AS mismatched_ranks
    FROM expected_rank
    LEFT JOIN bench.cq3_full_hits AS baseline
      ON baseline.route = :'baseline_route'
     AND baseline.filter_name = expected_rank.filter_name
     AND baseline.rank = expected_rank.rank
    LEFT JOIN bench.cq3_full_hits AS candidate
      ON candidate.route = :'candidate_route'
     AND candidate.filter_name = expected_rank.filter_name
     AND candidate.rank = expected_rank.rank
    GROUP BY expected_rank.filter_name, expected_rank.expected_hits
)
SELECT *
FROM compared
ORDER BY filter_name;

SELECT
    route,
    filter_name,
    count(*) AS attempts,
    bool_and(hit_count = 50) AS topk_complete,
    round(
        percentile_cont(0.5) WITHIN GROUP (ORDER BY elapsed_ms)::numeric,
        3
    ) AS p50_ms,
    round(
        percentile_cont(0.95) WITHIN GROUP (ORDER BY elapsed_ms)::numeric,
        3
    ) AS p95_ms,
    min(backend_memory_bytes) AS minimum_backend_memory_bytes,
    max(backend_memory_bytes) AS maximum_backend_memory_bytes,
    min(backend_rss_bytes) AS minimum_backend_rss_bytes,
    max(backend_rss_bytes) AS maximum_backend_rss_bytes,
    max((trace->>'memory_bytes')::int8) AS scorer_memory_bytes,
    max((trace->>'filtered_bmp_allowed_blocks')::int8)
        AS allowed_blocks,
    max((trace->>'filtered_bmp_allowed_superblocks')::int8)
        AS allowed_superblocks,
    max((trace->>'filtered_bmp_matching_ref_count')::int8)
        AS matching_refs,
    max((trace->>'filtered_bmp_matching_super_ref_count')::int8)
        AS matching_super_refs,
    max((trace->>'semantic_bmp_ref_reads')::int8) AS bound_refs,
    max((trace->>'semantic_bmp_record_reads')::int8) AS record_reads,
    max((trace->>'blocks_considered')::int8) AS blocks_considered,
    max((trace->>'blocks_scored')::int8) AS blocks_scored,
    max((trace->>'blocks_skipped')::int8) AS blocks_skipped,
    max((trace->>'semantic_bmp_postings_examined')::int8)
        AS semantic_postings_examined,
    max((trace->>'postings_examined')::int8)
        AS page_native_postings_examined,
    max((trace->>'documents_examined')::int8) AS documents_examined,
    max((trace->>'semantic_bmp_query_bytes')::int8) AS query_bytes,
    array_agg(DISTINCT trace->>'query_route') AS query_routes,
    bool_or((trace->>'accelerator_fallback')::boolean)
        AS accelerator_fallback,
    bool_and((trace->>'semantic_bmp_direct_flat_filtered')::boolean)
        AS direct_flat_filtered
FROM bench.cq3_full_results
WHERE route IN (:'baseline_route', :'candidate_route')
GROUP BY route, filter_name
ORDER BY filter_name, route;

WITH expected(filter_name, expected_hits) AS (
    VALUES
        ('p006', 50),
        ('p100', 50),
        ('p500', 50),
        ('unfiltered', 50)
), expected_rank AS (
    SELECT expected.*, rank
    FROM expected
    CROSS JOIN LATERAL generate_series(1, expected.expected_hits) AS rank
), compared AS (
    SELECT
        expected_rank.filter_name,
        count(baseline.rank) AS baseline_hits,
        count(candidate.rank) AS candidate_hits,
        count(*) FILTER (
            WHERE baseline.doc_ord IS DISTINCT FROM candidate.doc_ord
               OR float8send(baseline.score) IS DISTINCT FROM
                    float8send(candidate.score)
        ) AS mismatched_ranks
    FROM expected_rank
    LEFT JOIN bench.cq3_full_hits AS baseline
      ON baseline.route = :'baseline_route'
     AND baseline.filter_name = expected_rank.filter_name
     AND baseline.rank = expected_rank.rank
    LEFT JOIN bench.cq3_full_hits AS candidate
      ON candidate.route = :'candidate_route'
     AND candidate.filter_name = expected_rank.filter_name
     AND candidate.rank = expected_rank.rank
    GROUP BY expected_rank.filter_name
)
SELECT bool_and(
    baseline_hits = 50 AND
    candidate_hits = 50 AND
    mismatched_ranks = 0
) AS parity_ok
FROM compared
\gset
\if :parity_ok
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 qualification failed: baseline/candidate parity mismatch';
    END
    $failure$;
\endif

SELECT
    count(*) = 80 AND bool_and(hit_count = 50) AS runs_complete
FROM bench.cq3_full_results
WHERE route IN (:'baseline_route', :'candidate_route')
  AND filter_name IN ('p006', 'p100', 'p500', 'unfiltered')
\gset
\if :runs_complete
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 qualification failed: repeated top-k runs are incomplete';
    END
    $failure$;
\endif

WITH bounded AS (
    SELECT
        results.filter_name,
        max(cardinality(filter_set.tids))::int8 AS allowed_documents,
        bool_and(results.trace->>'query_route' = 'semantic_bmp')
            AS query_path,
        bool_or((results.trace->>'accelerator_fallback')::boolean)
            AS accelerator_fallback,
        bool_and(
            (results.trace->>'semantic_bmp_direct_flat_filtered')::boolean
        ) AS direct_flat_filtered,
        max((results.trace->>'semantic_bmp_query_bytes')::int8)
            AS query_bytes,
        max((results.trace->>'filtered_bmp_allowed_blocks')::int8)
            AS allowed_blocks,
        max((results.trace->>'blocks_considered')::int8)
            AS blocks_considered,
        max((results.trace->>'blocks_scored')::int8)
            AS blocks_scored,
        max((results.trace->>'documents_examined')::int8)
            AS documents_examined
    FROM bench.cq3_full_results AS results
    JOIN bench.cq3_full_filter_sets AS filter_set
      ON filter_set.name = results.filter_name
    WHERE results.route = :'candidate_route'
      AND results.filter_name IN ('p006', 'p100', 'p500')
    GROUP BY results.filter_name
)
SELECT
    count(*) = 3 AND bool_and(
        query_path AND
        NOT accelerator_fallback AND
        direct_flat_filtered AND
        query_bytes <= 64 * 1024 * 1024 AND
        allowed_blocks > 0 AND
        blocks_considered <= allowed_blocks AND
        blocks_scored <= allowed_blocks AND
        documents_examined <= allowed_documents
    ) AS filtered_work_bounded
FROM bounded
\gset
\if :filtered_work_bounded
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 qualification failed: filtered exact work was not bounded';
    END
    $failure$;
\endif

WITH work AS (
    SELECT
        filter_name,
        max((trace->>'semantic_bmp_ref_reads')::int8) AS bound_refs,
        max((trace->>'semantic_bmp_postings_examined')::int8)
            AS semantic_postings,
        max((trace->>'postings_examined')::int8)
            AS page_native_postings
    FROM bench.cq3_full_results
    WHERE route = :'candidate_route'
    GROUP BY filter_name
)
SELECT
    selective.bound_refs < unfiltered.bound_refs AND
    selective.semantic_postings + selective.page_native_postings <
        unfiltered.semantic_postings + unfiltered.page_native_postings
        AS selective_work_reduced
FROM work AS selective
JOIN work AS unfiltered ON unfiltered.filter_name = 'unfiltered'
WHERE selective.filter_name = 'p006'
\gset
\if :selective_work_reduced
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 qualification failed: selective filter did not reduce work';
    END
    $failure$;
\endif

WITH tail AS (
    SELECT
        filter_name,
        max(backend_memory_bytes) - min(backend_memory_bytes)
            AS memory_range,
        max(backend_rss_bytes) - min(backend_rss_bytes)
            AS rss_range
    FROM bench.cq3_full_results
    WHERE route = :'candidate_route'
      AND attempt >= 6
    GROUP BY filter_name
)
SELECT bool_and(
    memory_range <= 16 * 1024 * 1024 AND
    rss_range <= 64 * 1024 * 1024
) AS memory_plateau
FROM tail
\gset
\if :memory_plateau
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 qualification failed: backend memory did not plateau';
    END
    $failure$;
\endif
