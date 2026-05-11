#ifndef PSQL_BM25S_QUERY_H
#define PSQL_BM25S_QUERY_H

#include <stdbool.h>
#include <stddef.h>

#include "psql_bm25s_core.h"

typedef enum psql_bm25s_query_occur
{
    PSQL_BM25S_QUERY_SHOULD = 0,
    PSQL_BM25S_QUERY_MUST = 1,
    PSQL_BM25S_QUERY_MUST_NOT = 2
} psql_bm25s_query_occur;

typedef enum psql_bm25s_query_term_kind
{
    PSQL_BM25S_QUERY_TERM = 0,
    PSQL_BM25S_QUERY_PREFIX = 1,
    PSQL_BM25S_QUERY_PHRASE = 2
} psql_bm25s_query_term_kind;

typedef enum psql_bm25s_query_mode
{
    PSQL_BM25S_QUERY_MODE_LEGACY = 0,
    PSQL_BM25S_QUERY_MODE_BOOLEAN_AST = 1
} psql_bm25s_query_mode;

typedef struct psql_bm25s_query_term
{
    psql_bm25s_query_term_kind kind;
    psql_bm25s_query_occur occur;
    char **tokens;
    size_t *token_lens;
    size_t len;
} psql_bm25s_query_term;

typedef enum psql_bm25s_query_node_kind
{
    PSQL_BM25S_QUERY_NODE_TERM = 0,
    PSQL_BM25S_QUERY_NODE_AND = 1,
    PSQL_BM25S_QUERY_NODE_OR = 2,
    PSQL_BM25S_QUERY_NODE_NOT = 3
} psql_bm25s_query_node_kind;

typedef struct psql_bm25s_query_node
{
    psql_bm25s_query_node_kind kind;
    size_t term_index;
    struct psql_bm25s_query_node *left;
    struct psql_bm25s_query_node *right;
} psql_bm25s_query_node;

typedef struct psql_bm25s_query
{
    psql_bm25s_query_mode mode;
    psql_bm25s_query_term *terms;
    size_t len;
    psql_bm25s_query_node *root;
} psql_bm25s_query;

void psql_bm25s_query_init(psql_bm25s_query *query);
void psql_bm25s_query_free(psql_bm25s_query *query);

bool psql_bm25s_query_is_simple_term_query(const psql_bm25s_query *query);
bool psql_bm25s_query_uses_boolean_ast(const psql_bm25s_query *query);
bool psql_bm25s_query_has_phrase(const psql_bm25s_query *query);

void psql_bm25s_query_rewrite_terms(
    psql_bm25s_query *query,
    const size_t *old_to_new,
    size_t old_len
);

psql_bm25s_status psql_bm25s_parse_query_string(
    const char *input,
    psql_bm25s_query *query_out
);

#endif
