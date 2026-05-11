#include "psql_bm25s_query.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum psql_bm25s_query_token_kind
{
    PSQL_BM25S_QUERY_TOKEN_TERM = 0,
    PSQL_BM25S_QUERY_TOKEN_PREFIX = 1,
    PSQL_BM25S_QUERY_TOKEN_PHRASE = 2,
    PSQL_BM25S_QUERY_TOKEN_PLUS = 3,
    PSQL_BM25S_QUERY_TOKEN_MINUS = 4,
    PSQL_BM25S_QUERY_TOKEN_AND = 5,
    PSQL_BM25S_QUERY_TOKEN_OR = 6,
    PSQL_BM25S_QUERY_TOKEN_NOT = 7,
    PSQL_BM25S_QUERY_TOKEN_LPAREN = 8,
    PSQL_BM25S_QUERY_TOKEN_RPAREN = 9,
    PSQL_BM25S_QUERY_TOKEN_EOF = 10
} psql_bm25s_query_token_kind;

typedef struct psql_bm25s_query_token
{
    psql_bm25s_query_token_kind kind;
    psql_bm25s_query_term term;
} psql_bm25s_query_token;

typedef struct psql_bm25s_query_parser
{
    const psql_bm25s_query_token *tokens;
    size_t len;
    size_t pos;
    psql_bm25s_query *query;
} psql_bm25s_query_parser;

static void
psql_bm25s_query_term_free(psql_bm25s_query_term *term)
{
    size_t i;

    if (term == NULL)
    {
        return;
    }

    if (term->tokens != NULL)
    {
        for (i = 0; i < term->len; i++)
        {
            free(term->tokens[i]);
        }
        free(term->tokens);
    }
    free(term->token_lens);

    memset(term, 0, sizeof(*term));
}

static void
psql_bm25s_query_node_free(psql_bm25s_query_node *node)
{
    if (node == NULL)
    {
        return;
    }

    psql_bm25s_query_node_free(node->left);
    psql_bm25s_query_node_free(node->right);
    free(node);
}

static psql_bm25s_status
psql_bm25s_query_term_clone(
    const psql_bm25s_query_term *term,
    psql_bm25s_query_term *clone_out
)
{
    size_t i;

    memset(clone_out, 0, sizeof(*clone_out));
    if (term == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    clone_out->kind = term->kind;
    clone_out->occur = term->occur;
    clone_out->len = term->len;
    if (term->len == 0)
    {
        return PSQL_BM25S_OK;
    }

    clone_out->tokens = malloc(sizeof(*clone_out->tokens) * term->len);
    clone_out->token_lens = malloc(sizeof(*clone_out->token_lens) * term->len);
    if (clone_out->tokens == NULL || clone_out->token_lens == NULL)
    {
        psql_bm25s_query_term_free(clone_out);
        return PSQL_BM25S_ERR_NOMEM;
    }

    for (i = 0; i < term->len; i++)
    {
        size_t token_len;

        token_len = term->token_lens != NULL ?
            term->token_lens[i] :
            strlen(term->tokens[i]);
        clone_out->tokens[i] = malloc(token_len + 1);
        if (clone_out->tokens[i] == NULL)
        {
            psql_bm25s_query_term_free(clone_out);
            return PSQL_BM25S_ERR_NOMEM;
        }
        memcpy(clone_out->tokens[i], term->tokens[i], token_len + 1);
        clone_out->token_lens[i] = token_len;
    }

    return PSQL_BM25S_OK;
}

void
psql_bm25s_query_init(psql_bm25s_query *query)
{
    if (query == NULL)
    {
        return;
    }

    memset(query, 0, sizeof(*query));
    query->mode = PSQL_BM25S_QUERY_MODE_LEGACY;
}

void
psql_bm25s_query_free(psql_bm25s_query *query)
{
    size_t i;

    if (query == NULL)
    {
        return;
    }

    for (i = 0; i < query->len; i++)
    {
        psql_bm25s_query_term_free(&query->terms[i]);
    }
    free(query->terms);
    psql_bm25s_query_node_free(query->root);
    memset(query, 0, sizeof(*query));
}

static bool
psql_bm25s_query_node_is_simple_term_disjunction(
    const psql_bm25s_query *query,
    const psql_bm25s_query_node *node
)
{
    const psql_bm25s_query_term *term;

    if (query == NULL || node == NULL)
    {
        return false;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_OR)
    {
        return
            psql_bm25s_query_node_is_simple_term_disjunction(
                query,
                node->left
            ) &&
            psql_bm25s_query_node_is_simple_term_disjunction(
                query,
                node->right
            );
    }

    if (node->kind != PSQL_BM25S_QUERY_NODE_TERM ||
        node->term_index >= query->len)
    {
        return false;
    }

    term = &query->terms[node->term_index];
    return term->kind == PSQL_BM25S_QUERY_TERM &&
           term->len == 1 &&
           term->tokens != NULL &&
           term->tokens[0] != NULL;
}

bool
psql_bm25s_query_is_simple_term_query(const psql_bm25s_query *query)
{
    size_t i;

    if (query == NULL || query->len == 0)
    {
        return false;
    }

    if (query->mode == PSQL_BM25S_QUERY_MODE_BOOLEAN_AST)
    {
        return psql_bm25s_query_node_is_simple_term_disjunction(
            query,
            query->root
        );
    }

    for (i = 0; i < query->len; i++)
    {
        const psql_bm25s_query_term *term = &query->terms[i];

        if (term->kind != PSQL_BM25S_QUERY_TERM ||
            term->occur != PSQL_BM25S_QUERY_SHOULD ||
            term->len != 1 ||
            term->tokens == NULL ||
            term->tokens[0] == NULL)
        {
            return false;
        }
    }

    return true;
}

bool
psql_bm25s_query_uses_boolean_ast(const psql_bm25s_query *query)
{
    return query != NULL &&
           query->mode == PSQL_BM25S_QUERY_MODE_BOOLEAN_AST &&
           query->root != NULL;
}

bool
psql_bm25s_query_has_phrase(const psql_bm25s_query *query)
{
    size_t i;

    if (query == NULL)
    {
        return false;
    }

    for (i = 0; i < query->len; i++)
    {
        if (query->terms[i].kind == PSQL_BM25S_QUERY_PHRASE)
        {
            return true;
        }
    }

    return false;
}

static psql_bm25s_query_node *
psql_bm25s_query_node_new(psql_bm25s_query_node_kind kind)
{
    psql_bm25s_query_node *node;

    node = calloc(1, sizeof(*node));
    if (node == NULL)
    {
        return NULL;
    }

    node->kind = kind;
    node->term_index = SIZE_MAX;
    return node;
}

static psql_bm25s_query_node *
psql_bm25s_query_node_term(size_t term_index)
{
    psql_bm25s_query_node *node;

    node = psql_bm25s_query_node_new(PSQL_BM25S_QUERY_NODE_TERM);
    if (node == NULL)
    {
        return NULL;
    }

    node->term_index = term_index;
    return node;
}

static psql_bm25s_query_node *
psql_bm25s_query_node_unary(
    psql_bm25s_query_node_kind kind,
    psql_bm25s_query_node *child
)
{
    psql_bm25s_query_node *node;

    if (child == NULL)
    {
        return NULL;
    }

    node = psql_bm25s_query_node_new(kind);
    if (node == NULL)
    {
        psql_bm25s_query_node_free(child);
        return NULL;
    }

    node->left = child;
    return node;
}

static psql_bm25s_query_node *
psql_bm25s_query_node_binary(
    psql_bm25s_query_node_kind kind,
    psql_bm25s_query_node *left,
    psql_bm25s_query_node *right
)
{
    psql_bm25s_query_node *node;

    if (left == NULL)
    {
        return right;
    }
    if (right == NULL)
    {
        return left;
    }

    node = psql_bm25s_query_node_new(kind);
    if (node == NULL)
    {
        psql_bm25s_query_node_free(left);
        psql_bm25s_query_node_free(right);
        return NULL;
    }

    node->left = left;
    node->right = right;
    return node;
}

static const char *
psql_bm25s_skip_space(const char *cursor)
{
    while (*cursor != '\0' && isspace((unsigned char) *cursor))
    {
        cursor++;
    }

    return cursor;
}

static char *
psql_bm25s_copy_span(const char *start, size_t len)
{
    char *copy;

    copy = malloc(len + 1);
    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

static void
psql_bm25s_query_token_free(psql_bm25s_query_token *token)
{
    if (token == NULL)
    {
        return;
    }

    psql_bm25s_query_term_free(&token->term);
}

static void
psql_bm25s_query_tokens_free(
    psql_bm25s_query_token *tokens,
    size_t len
)
{
    size_t i;

    if (tokens == NULL)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        psql_bm25s_query_token_free(&tokens[i]);
    }
    free(tokens);
}

static psql_bm25s_status
psql_bm25s_append_token(
    psql_bm25s_query_token **tokens,
    size_t *len,
    const psql_bm25s_query_token *token
)
{
    psql_bm25s_query_token *resized;

    resized = realloc(*tokens, sizeof(*resized) * (*len + 1));
    if (resized == NULL)
    {
        return PSQL_BM25S_ERR_NOMEM;
    }

    *tokens = resized;
    (*tokens)[*len] = *token;
    (*len)++;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_split_phrase(
    const char *start,
    size_t len,
    char ***tokens_out,
    size_t **token_lens_out,
    size_t *len_out
)
{
    const char *cursor = start;
    const char *end = start + len;
    char **tokens = NULL;
    size_t *token_lens = NULL;
    size_t token_len = 0;

    *tokens_out = NULL;
    *token_lens_out = NULL;
    *len_out = 0;

    while (cursor < end)
    {
        const char *term_start;
        size_t span_len;
        char **new_tokens;
        size_t *new_token_lens;
        char *token;

        while (cursor < end && isspace((unsigned char) *cursor))
        {
            cursor++;
        }
        if (cursor >= end)
        {
            break;
        }

        term_start = cursor;
        while (cursor < end && !isspace((unsigned char) *cursor))
        {
            cursor++;
        }
        span_len = (size_t) (cursor - term_start);
        token = psql_bm25s_copy_span(term_start, span_len);
        if (token == NULL)
        {
            size_t i;

            for (i = 0; i < token_len; i++)
            {
                free(tokens[i]);
            }
            free(tokens);
            free(token_lens);
            return PSQL_BM25S_ERR_NOMEM;
        }

        new_tokens = realloc(tokens, sizeof(*tokens) * (token_len + 1));
        new_token_lens = realloc(
            token_lens,
            sizeof(*token_lens) * (token_len + 1)
        );
        if (new_tokens == NULL || new_token_lens == NULL)
        {
            size_t i;

            free(token);
            if (new_tokens != NULL)
            {
                tokens = new_tokens;
            }
            if (new_token_lens != NULL)
            {
                token_lens = new_token_lens;
            }
            for (i = 0; i < token_len; i++)
            {
                free(tokens[i]);
            }
            free(tokens);
            free(token_lens);
            return PSQL_BM25S_ERR_NOMEM;
        }

        tokens = new_tokens;
        token_lens = new_token_lens;
        tokens[token_len++] = token;
        token_lens[token_len - 1] = span_len;
    }

    if (token_len == 0)
    {
        free(tokens);
        free(token_lens);
        return PSQL_BM25S_ERR_INVALID;
    }

    *tokens_out = tokens;
    *token_lens_out = token_lens;
    *len_out = token_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_tokenize_query_string(
    const char *input,
    psql_bm25s_query_token **tokens_out,
    size_t *len_out,
    bool *has_boolean_syntax_out
)
{
    const char *cursor;
    psql_bm25s_query_token *tokens = NULL;
    size_t len = 0;
    bool has_boolean_syntax = false;

    *tokens_out = NULL;
    *len_out = 0;
    *has_boolean_syntax_out = false;
    if (input == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    cursor = input;
    while (true)
    {
        psql_bm25s_query_token token = {0};
        const char *start;
        size_t span_len;
        psql_bm25s_status status;

        cursor = psql_bm25s_skip_space(cursor);
        if (*cursor == '\0')
        {
            break;
        }

        if (*cursor == '+')
        {
            token.kind = PSQL_BM25S_QUERY_TOKEN_PLUS;
            cursor++;
        }
        else if (*cursor == '-')
        {
            token.kind = PSQL_BM25S_QUERY_TOKEN_MINUS;
            cursor++;
        }
        else if (*cursor == '(')
        {
            token.kind = PSQL_BM25S_QUERY_TOKEN_LPAREN;
            has_boolean_syntax = true;
            cursor++;
        }
        else if (*cursor == ')')
        {
            token.kind = PSQL_BM25S_QUERY_TOKEN_RPAREN;
            has_boolean_syntax = true;
            cursor++;
        }
        else if (*cursor == '"')
        {
            cursor++;
            start = cursor;
            while (*cursor != '\0' && *cursor != '"')
            {
                cursor++;
            }
            if (*cursor != '"')
            {
                psql_bm25s_query_tokens_free(tokens, len);
                return PSQL_BM25S_ERR_INVALID;
            }

            token.kind = PSQL_BM25S_QUERY_TOKEN_PHRASE;
            token.term.kind = PSQL_BM25S_QUERY_PHRASE;
            status = psql_bm25s_split_phrase(
                start,
                (size_t) (cursor - start),
                &token.term.tokens,
                &token.term.token_lens,
                &token.term.len
            );
            if (status != PSQL_BM25S_OK)
            {
                psql_bm25s_query_tokens_free(tokens, len);
                return status;
            }
            cursor++;
        }
        else
        {
            start = cursor;
            while (*cursor != '\0' &&
                   !isspace((unsigned char) *cursor) &&
                   *cursor != '(' &&
                   *cursor != ')')
            {
                cursor++;
            }
            span_len = (size_t) (cursor - start);
            if (span_len == 0)
            {
                psql_bm25s_query_tokens_free(tokens, len);
                return PSQL_BM25S_ERR_INVALID;
            }

            if (span_len == 3 && memcmp(start, "AND", 3) == 0)
            {
                token.kind = PSQL_BM25S_QUERY_TOKEN_AND;
                has_boolean_syntax = true;
            }
            else if (span_len == 2 && memcmp(start, "OR", 2) == 0)
            {
                token.kind = PSQL_BM25S_QUERY_TOKEN_OR;
                has_boolean_syntax = true;
            }
            else if (span_len == 3 && memcmp(start, "NOT", 3) == 0)
            {
                token.kind = PSQL_BM25S_QUERY_TOKEN_NOT;
                has_boolean_syntax = true;
            }
            else
            {
                token.kind = PSQL_BM25S_QUERY_TOKEN_TERM;
                token.term.kind = PSQL_BM25S_QUERY_TERM;
                if (start[span_len - 1] == '*')
                {
                    if (span_len == 1)
                    {
                        psql_bm25s_query_tokens_free(tokens, len);
                        return PSQL_BM25S_ERR_INVALID;
                    }
                    token.kind = PSQL_BM25S_QUERY_TOKEN_PREFIX;
                    token.term.kind = PSQL_BM25S_QUERY_PREFIX;
                    span_len--;
                }

                token.term.tokens = malloc(sizeof(*token.term.tokens));
                token.term.token_lens = malloc(sizeof(*token.term.token_lens));
                if (token.term.tokens == NULL || token.term.token_lens == NULL)
                {
                    psql_bm25s_query_token_free(&token);
                    psql_bm25s_query_tokens_free(tokens, len);
                    return PSQL_BM25S_ERR_NOMEM;
                }
                token.term.tokens[0] = psql_bm25s_copy_span(start, span_len);
                if (token.term.tokens[0] == NULL)
                {
                    psql_bm25s_query_token_free(&token);
                    psql_bm25s_query_tokens_free(tokens, len);
                    return PSQL_BM25S_ERR_NOMEM;
                }
                token.term.token_lens[0] = span_len;
                token.term.len = 1;
            }
        }

        status = psql_bm25s_append_token(&tokens, &len, &token);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_token_free(&token);
            psql_bm25s_query_tokens_free(tokens, len);
            return status;
        }
    }

    *tokens_out = tokens;
    *len_out = len;
    *has_boolean_syntax_out = has_boolean_syntax;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_append_term(
    psql_bm25s_query *query,
    const psql_bm25s_query_term *term,
    size_t *index_out
)
{
    psql_bm25s_query_term *terms;
    psql_bm25s_query_term cloned;
    size_t new_len;
    psql_bm25s_status status;

    if (query == NULL || term == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = psql_bm25s_query_term_clone(term, &cloned);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    new_len = query->len + 1;
    terms = realloc(query->terms, sizeof(*terms) * new_len);
    if (terms == NULL)
    {
        psql_bm25s_query_term_free(&cloned);
        return PSQL_BM25S_ERR_NOMEM;
    }

    query->terms = terms;
    query->terms[query->len] = cloned;
    if (index_out != NULL)
    {
        *index_out = query->len;
    }
    query->len = new_len;
    return PSQL_BM25S_OK;
}

static psql_bm25s_query_node *
psql_bm25s_query_build_legacy_root(const psql_bm25s_query *query)
{
    psql_bm25s_query_node *must_root = NULL;
    psql_bm25s_query_node *should_root = NULL;
    psql_bm25s_query_node *negative_root = NULL;
    size_t i;

    if (query == NULL)
    {
        return NULL;
    }

    for (i = 0; i < query->len; i++)
    {
        const psql_bm25s_query_term *term = &query->terms[i];
        psql_bm25s_query_node *leaf;

        leaf = psql_bm25s_query_node_term(i);
        if (leaf == NULL)
        {
            psql_bm25s_query_node_free(must_root);
            psql_bm25s_query_node_free(should_root);
            psql_bm25s_query_node_free(negative_root);
            return NULL;
        }

        if (term->occur == PSQL_BM25S_QUERY_MUST)
        {
            must_root = psql_bm25s_query_node_binary(
                PSQL_BM25S_QUERY_NODE_AND,
                must_root,
                leaf
            );
        }
        else if (term->occur == PSQL_BM25S_QUERY_MUST_NOT)
        {
            negative_root = psql_bm25s_query_node_binary(
                PSQL_BM25S_QUERY_NODE_AND,
                negative_root,
                psql_bm25s_query_node_unary(PSQL_BM25S_QUERY_NODE_NOT, leaf)
            );
        }
        else
        {
            should_root = psql_bm25s_query_node_binary(
                PSQL_BM25S_QUERY_NODE_OR,
                should_root,
                leaf
            );
        }
    }

    if (must_root == NULL)
    {
        must_root = should_root;
        should_root = NULL;
    }

    if (must_root == NULL)
    {
        return negative_root;
    }

    return psql_bm25s_query_node_binary(
        PSQL_BM25S_QUERY_NODE_AND,
        must_root,
        negative_root
    );
}

static bool
psql_bm25s_parser_token_can_start_unary(
    psql_bm25s_query_token_kind kind
)
{
    return kind == PSQL_BM25S_QUERY_TOKEN_TERM ||
           kind == PSQL_BM25S_QUERY_TOKEN_PREFIX ||
           kind == PSQL_BM25S_QUERY_TOKEN_PHRASE ||
           kind == PSQL_BM25S_QUERY_TOKEN_LPAREN ||
           kind == PSQL_BM25S_QUERY_TOKEN_PLUS ||
           kind == PSQL_BM25S_QUERY_TOKEN_MINUS ||
           kind == PSQL_BM25S_QUERY_TOKEN_NOT;
}

static const psql_bm25s_query_token *
psql_bm25s_query_parser_peek(const psql_bm25s_query_parser *parser)
{
    static const psql_bm25s_query_token eof = {
        .kind = PSQL_BM25S_QUERY_TOKEN_EOF
    };

    if (parser->pos >= parser->len)
    {
        return &eof;
    }

    return &parser->tokens[parser->pos];
}

static void
psql_bm25s_query_parser_consume(psql_bm25s_query_parser *parser)
{
    if (parser->pos < parser->len)
    {
        parser->pos++;
    }
}

static psql_bm25s_status
psql_bm25s_parse_boolean_or(
    psql_bm25s_query_parser *parser,
    psql_bm25s_query_node **node_out
);

static psql_bm25s_status
psql_bm25s_parse_boolean_primary(
    psql_bm25s_query_parser *parser,
    psql_bm25s_query_node **node_out
)
{
    const psql_bm25s_query_token *token;
    psql_bm25s_status status;

    token = psql_bm25s_query_parser_peek(parser);
    if (token->kind == PSQL_BM25S_QUERY_TOKEN_LPAREN)
    {
        psql_bm25s_query_node *node;

        psql_bm25s_query_parser_consume(parser);
        status = psql_bm25s_parse_boolean_or(parser, &node);
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }
        if (psql_bm25s_query_parser_peek(parser)->kind !=
            PSQL_BM25S_QUERY_TOKEN_RPAREN)
        {
            psql_bm25s_query_node_free(node);
            return PSQL_BM25S_ERR_INVALID;
        }
        psql_bm25s_query_parser_consume(parser);
        *node_out = node;
        return PSQL_BM25S_OK;
    }

    if (token->kind == PSQL_BM25S_QUERY_TOKEN_TERM ||
        token->kind == PSQL_BM25S_QUERY_TOKEN_PREFIX ||
        token->kind == PSQL_BM25S_QUERY_TOKEN_PHRASE)
    {
        psql_bm25s_query_node *node;
        size_t term_index = SIZE_MAX;

        status = psql_bm25s_append_term(parser->query, &token->term, &term_index);
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }

        node = psql_bm25s_query_node_term(term_index);
        if (node == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
        psql_bm25s_query_parser_consume(parser);
        *node_out = node;
        return PSQL_BM25S_OK;
    }

    return PSQL_BM25S_ERR_INVALID;
}

static psql_bm25s_status
psql_bm25s_parse_boolean_unary(
    psql_bm25s_query_parser *parser,
    psql_bm25s_query_node **node_out
)
{
    const psql_bm25s_query_token *token;
    psql_bm25s_query_node *child;
    psql_bm25s_status status;

    token = psql_bm25s_query_parser_peek(parser);
    if (token->kind == PSQL_BM25S_QUERY_TOKEN_PLUS)
    {
        psql_bm25s_query_parser_consume(parser);
        return psql_bm25s_parse_boolean_unary(parser, node_out);
    }
    if (token->kind == PSQL_BM25S_QUERY_TOKEN_MINUS ||
        token->kind == PSQL_BM25S_QUERY_TOKEN_NOT)
    {
        psql_bm25s_query_parser_consume(parser);
        status = psql_bm25s_parse_boolean_unary(parser, &child);
        if (status != PSQL_BM25S_OK)
        {
            return status;
        }

        *node_out = psql_bm25s_query_node_unary(
            PSQL_BM25S_QUERY_NODE_NOT,
            child
        );
        if (*node_out == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
        return PSQL_BM25S_OK;
    }

    return psql_bm25s_parse_boolean_primary(parser, node_out);
}

static psql_bm25s_status
psql_bm25s_parse_boolean_and(
    psql_bm25s_query_parser *parser,
    psql_bm25s_query_node **node_out
)
{
    psql_bm25s_query_node *node;
    psql_bm25s_status status;

    status = psql_bm25s_parse_boolean_unary(parser, &node);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    while (psql_bm25s_query_parser_peek(parser)->kind ==
           PSQL_BM25S_QUERY_TOKEN_AND)
    {
        psql_bm25s_query_node *rhs;

        psql_bm25s_query_parser_consume(parser);
        status = psql_bm25s_parse_boolean_unary(parser, &rhs);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_node_free(node);
            return status;
        }

        node = psql_bm25s_query_node_binary(
            PSQL_BM25S_QUERY_NODE_AND,
            node,
            rhs
        );
        if (node == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    *node_out = node;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_parse_boolean_or(
    psql_bm25s_query_parser *parser,
    psql_bm25s_query_node **node_out
)
{
    psql_bm25s_query_node *node;
    psql_bm25s_status status;

    status = psql_bm25s_parse_boolean_and(parser, &node);
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    while (true)
    {
        const psql_bm25s_query_token *token;
        psql_bm25s_query_node *rhs;

        token = psql_bm25s_query_parser_peek(parser);
        if (token->kind == PSQL_BM25S_QUERY_TOKEN_OR)
        {
            psql_bm25s_query_parser_consume(parser);
        }
        else if (!psql_bm25s_parser_token_can_start_unary(token->kind))
        {
            break;
        }

        status = psql_bm25s_parse_boolean_and(parser, &rhs);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_node_free(node);
            return status;
        }

        node = psql_bm25s_query_node_binary(
            PSQL_BM25S_QUERY_NODE_OR,
            node,
            rhs
        );
        if (node == NULL)
        {
            return PSQL_BM25S_ERR_NOMEM;
        }
    }

    *node_out = node;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_parse_boolean_tokens(
    const psql_bm25s_query_token *tokens,
    size_t len,
    psql_bm25s_query *query_out
)
{
    psql_bm25s_query query = {0};
    psql_bm25s_query_parser parser;
    psql_bm25s_status status;

    psql_bm25s_query_init(&query);
    query.mode = PSQL_BM25S_QUERY_MODE_BOOLEAN_AST;

    parser.tokens = tokens;
    parser.len = len;
    parser.pos = 0;
    parser.query = &query;

    status = psql_bm25s_parse_boolean_or(&parser, &query.root);
    if (status != PSQL_BM25S_OK)
    {
        psql_bm25s_query_free(&query);
        return status;
    }
    if (query.root == NULL || parser.pos != len)
    {
        psql_bm25s_query_free(&query);
        return PSQL_BM25S_ERR_INVALID;
    }

    *query_out = query;
    return PSQL_BM25S_OK;
}

static psql_bm25s_status
psql_bm25s_parse_legacy_tokens(
    const psql_bm25s_query_token *tokens,
    size_t len,
    psql_bm25s_query *query_out
)
{
    psql_bm25s_query query = {0};
    psql_bm25s_query_occur next_occur = PSQL_BM25S_QUERY_SHOULD;
    bool expect_term = true;
    size_t i;

    psql_bm25s_query_init(&query);
    query.mode = PSQL_BM25S_QUERY_MODE_LEGACY;

    for (i = 0; i < len; i++)
    {
        psql_bm25s_query_term term;
        psql_bm25s_status status;

        if (tokens[i].kind == PSQL_BM25S_QUERY_TOKEN_PLUS)
        {
            next_occur = PSQL_BM25S_QUERY_MUST;
            expect_term = true;
            continue;
        }
        if (tokens[i].kind == PSQL_BM25S_QUERY_TOKEN_MINUS)
        {
            next_occur = PSQL_BM25S_QUERY_MUST_NOT;
            expect_term = true;
            continue;
        }
        if (tokens[i].kind != PSQL_BM25S_QUERY_TOKEN_TERM &&
            tokens[i].kind != PSQL_BM25S_QUERY_TOKEN_PREFIX &&
            tokens[i].kind != PSQL_BM25S_QUERY_TOKEN_PHRASE)
        {
            psql_bm25s_query_free(&query);
            return PSQL_BM25S_ERR_INVALID;
        }

        term = tokens[i].term;
        term.occur = next_occur;
        status = psql_bm25s_append_term(&query, &term, NULL);
        if (status != PSQL_BM25S_OK)
        {
            psql_bm25s_query_free(&query);
            return status;
        }
        next_occur = PSQL_BM25S_QUERY_SHOULD;
        expect_term = false;
    }

    if (expect_term && len > 0)
    {
        psql_bm25s_query_free(&query);
        return PSQL_BM25S_ERR_INVALID;
    }

    query.root = psql_bm25s_query_build_legacy_root(&query);
    if (query.len > 0 && query.root == NULL)
    {
        psql_bm25s_query_free(&query);
        return PSQL_BM25S_ERR_NOMEM;
    }

    *query_out = query;
    return PSQL_BM25S_OK;
}

static psql_bm25s_query_node *
psql_bm25s_query_rewrite_node(
    psql_bm25s_query_node *node,
    const size_t *old_to_new,
    size_t old_len
)
{
    if (node == NULL)
    {
        return NULL;
    }

    if (node->kind == PSQL_BM25S_QUERY_NODE_TERM)
    {
        if (node->term_index >= old_len ||
            old_to_new[node->term_index] == SIZE_MAX)
        {
            free(node);
            return NULL;
        }

        node->term_index = old_to_new[node->term_index];
        return node;
    }

    node->left = psql_bm25s_query_rewrite_node(
        node->left,
        old_to_new,
        old_len
    );
    node->right = psql_bm25s_query_rewrite_node(
        node->right,
        old_to_new,
        old_len
    );

    if (node->kind == PSQL_BM25S_QUERY_NODE_NOT)
    {
        if (node->left == NULL)
        {
            free(node);
            return NULL;
        }
        return node;
    }

    if (node->left == NULL && node->right == NULL)
    {
        free(node);
        return NULL;
    }
    if (node->left == NULL)
    {
        psql_bm25s_query_node *right = node->right;

        free(node);
        return right;
    }
    if (node->right == NULL)
    {
        psql_bm25s_query_node *left = node->left;

        free(node);
        return left;
    }

    return node;
}

void
psql_bm25s_query_rewrite_terms(
    psql_bm25s_query *query,
    const size_t *old_to_new,
    size_t old_len
)
{
    if (query == NULL || old_to_new == NULL)
    {
        return;
    }

    query->root = psql_bm25s_query_rewrite_node(
        query->root,
        old_to_new,
        old_len
    );
}

psql_bm25s_status
psql_bm25s_parse_query_string(
    const char *input,
    psql_bm25s_query *query_out
)
{
    psql_bm25s_query_token *tokens = NULL;
    size_t len = 0;
    bool has_boolean_syntax = false;
    psql_bm25s_status status;

    if (input == NULL || query_out == NULL)
    {
        return PSQL_BM25S_ERR_INVALID;
    }

    status = psql_bm25s_tokenize_query_string(
        input,
        &tokens,
        &len,
        &has_boolean_syntax
    );
    if (status != PSQL_BM25S_OK)
    {
        return status;
    }

    if (has_boolean_syntax)
    {
        status = psql_bm25s_parse_boolean_tokens(tokens, len, query_out);
    }
    else
    {
        status = psql_bm25s_parse_legacy_tokens(tokens, len, query_out);
    }

    psql_bm25s_query_tokens_free(tokens, len);
    return status;
}
