CREATE FUNCTION psql_bm25s_generation_cache_clear()
RETURNS int4
AS 'MODULE_PATHNAME', 'psql_bm25s_generation_cache_clear'
LANGUAGE C VOLATILE PARALLEL UNSAFE;

CREATE FUNCTION psql_bm25s_generation_cache_state(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_generation_cache_state'
LANGUAGE C STABLE PARALLEL SAFE;

CREATE FUNCTION psql_bm25s_generation_cache_preload(index_name regclass)
RETURNS text
AS 'MODULE_PATHNAME', 'psql_bm25s_generation_cache_preload'
LANGUAGE C VOLATILE PARALLEL UNSAFE;
