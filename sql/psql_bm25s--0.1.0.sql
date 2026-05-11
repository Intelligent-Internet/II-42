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

CREATE FUNCTION psql_bm25s_match_query_tokens_op(doc_tokens text[], query_text text)
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

CREATE OPERATOR @@ (
    LEFTARG = text[],
    RIGHTARG = text,
    PROCEDURE = psql_bm25s_match_query_tokens_op
);

CREATE OPERATOR CLASS psql_bm25s_text_array_ops
DEFAULT FOR TYPE text[] USING psql_bm25s AS
    OPERATOR 1 @@ (text[], text),
    OPERATOR 1 <=> (text[], text[]) FOR ORDER BY pg_catalog.float_ops,
    STORAGE text[];

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
