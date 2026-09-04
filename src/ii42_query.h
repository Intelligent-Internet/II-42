#ifndef II42_QUERY_H
#define II42_QUERY_H

#include <stdbool.h>
#include <stddef.h>

#include "ii42_core.h"

#define II42_QUERY_MAX_TOKENS 1024U
#define II42_QUERY_MAX_NESTING 128U

typedef enum ii42_query_occur
{
    II42_QUERY_SHOULD = 0,
    II42_QUERY_MUST = 1,
    II42_QUERY_MUST_NOT = 2
} ii42_query_occur;

typedef enum ii42_query_term_kind
{
    II42_QUERY_TERM = 0,
    II42_QUERY_PREFIX = 1,
    II42_QUERY_PHRASE = 2
} ii42_query_term_kind;

typedef enum ii42_query_mode
{
    II42_QUERY_MODE_CLASSIC = 0,
    II42_QUERY_MODE_BOOLEAN_AST = 1
} ii42_query_mode;

typedef struct ii42_query_term
{
    ii42_query_term_kind kind;
    ii42_query_occur occur;
    char **tokens;
    size_t *token_lens;
    size_t len;
} ii42_query_term;

typedef enum ii42_query_node_kind
{
    II42_QUERY_NODE_TERM = 0,
    II42_QUERY_NODE_AND = 1,
    II42_QUERY_NODE_OR = 2,
    II42_QUERY_NODE_NOT = 3
} ii42_query_node_kind;

typedef struct ii42_query_node
{
    ii42_query_node_kind kind;
    size_t term_index;
    struct ii42_query_node *left;
    struct ii42_query_node *right;
} ii42_query_node;

typedef struct ii42_query
{
    ii42_query_mode mode;
    ii42_query_term *terms;
    size_t len;
    ii42_query_node *root;
} ii42_query;

void ii42_query_init(ii42_query *query);
void ii42_query_free(ii42_query *query);

bool ii42_query_is_simple_term_query(const ii42_query *query);
bool ii42_query_uses_boolean_ast(const ii42_query *query);
bool ii42_query_has_phrase(const ii42_query *query);

void ii42_query_rewrite_terms(
    ii42_query *query,
    const size_t *old_to_new,
    size_t old_len
);

ii42_status ii42_parse_query_string(
    const char *input,
    ii42_query *query_out
);

#endif
