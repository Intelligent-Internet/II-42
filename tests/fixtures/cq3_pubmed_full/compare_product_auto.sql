\set ON_ERROR_STOP on

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
SELECT
    filter_name,
    baseline_hits,
    candidate_hits,
    mismatched_ranks
FROM compared
ORDER BY filter_name;

WITH expected_rank AS (
    SELECT filter_name, rank
    FROM (
        VALUES ('p006'), ('p100'), ('p500'), ('unfiltered')
    ) AS expected(filter_name)
    CROSS JOIN LATERAL generate_series(1, 50) AS rank
), compared AS (
    SELECT
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
)
SELECT
    baseline_hits = 200 AND
    candidate_hits = 200 AND
    mismatched_ranks = 0 AS parity_ok
FROM compared
\gset
\if :parity_ok
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 product-route qualification failed: rank mismatch';
    END
    $failure$;
\endif

WITH attempts AS (
    SELECT
        route,
        filter_name,
        count(*) AS attempts,
        bool_and(hit_count = 50) AS complete
    FROM bench.cq3_full_results
    WHERE route IN (:'baseline_route', :'candidate_route')
    GROUP BY route, filter_name
)
SELECT count(*) = 8 AND bool_and(attempts = 5 AND complete) AS runs_complete
FROM attempts
\gset
\if :runs_complete
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 product-route qualification failed: incomplete attempts';
    END
    $failure$;
\endif

WITH identities AS (
    SELECT route, min(identity::text) = max(identity::text) AS stable
    FROM bench.cq3_full_root_identity
    WHERE route IN (:'baseline_route', :'candidate_route')
    GROUP BY route
)
SELECT count(*) = 2 AND bool_and(stable) AS root_stable
FROM identities
\gset
\if :root_stable
\else
    DO $failure$
    BEGIN
        RAISE EXCEPTION
            'CQ-3 product-route qualification changed persistent root';
    END
    $failure$;
\endif

SELECT
    route,
    filter_name,
    round(
        percentile_cont(0.5) WITHIN GROUP (ORDER BY elapsed_ms)::numeric,
        3
    ) AS p50_ms,
    round(
        percentile_cont(0.95) WITHIN GROUP (ORDER BY elapsed_ms)::numeric,
        3
    ) AS p95_ms,
    array_agg(DISTINCT trace->>'query_route') AS query_routes,
    max((trace->>'accelerator_forward_bytes')::int8) AS forward_bytes,
    max((trace->>'ranked_prefix_probe_documents_examined')::int8)
        AS prefix_documents,
    max((trace->>'ranked_prefix_probe_postings_examined')::int8)
        AS prefix_postings,
    max((trace->>'memory_bytes')::int8) AS memory_bytes
FROM bench.cq3_full_results
WHERE route IN (:'baseline_route', :'candidate_route')
GROUP BY route, filter_name
ORDER BY filter_name, route;
