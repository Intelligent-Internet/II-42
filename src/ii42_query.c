#include "ii42_query.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum ii42_query_token_kind
{
    II42_QUERY_TOKEN_TERM = 0,
    II42_QUERY_TOKEN_PREFIX = 1,
    II42_QUERY_TOKEN_PHRASE = 2,
    II42_QUERY_TOKEN_PLUS = 3,
    II42_QUERY_TOKEN_MINUS = 4,
    II42_QUERY_TOKEN_AND = 5,
    II42_QUERY_TOKEN_OR = 6,
    II42_QUERY_TOKEN_NOT = 7,
    II42_QUERY_TOKEN_LPAREN = 8,
    II42_QUERY_TOKEN_RPAREN = 9,
    II42_QUERY_TOKEN_EOF = 10
} ii42_query_token_kind;

typedef struct ii42_query_token
{
    ii42_query_token_kind kind;
    ii42_query_term term;
} ii42_query_token;

typedef struct ii42_query_parser
{
    const ii42_query_token *tokens;
    size_t len;
    size_t pos;
    size_t depth;
    ii42_query *query;
} ii42_query_parser;

static void
ii42_query_term_free(ii42_query_term *term)
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
ii42_query_node_free(ii42_query_node *node)
{
    if (node == NULL)
    {
        return;
    }

    ii42_query_node_free(node->left);
    ii42_query_node_free(node->right);
    free(node);
}

static ii42_status
ii42_query_term_clone(
    const ii42_query_term *term,
    ii42_query_term *clone_out
)
{
    size_t i;

    memset(clone_out, 0, sizeof(*clone_out));
    if (term == NULL)
    {
        return II42_ERR_INVALID;
    }

    clone_out->kind = term->kind;
    clone_out->occur = term->occur;
    clone_out->len = term->len;
    if (term->len == 0)
    {
        return II42_OK;
    }

    clone_out->tokens = malloc(sizeof(*clone_out->tokens) * term->len);
    clone_out->token_lens = malloc(sizeof(*clone_out->token_lens) * term->len);
    if (clone_out->tokens == NULL || clone_out->token_lens == NULL)
    {
        ii42_query_term_free(clone_out);
        return II42_ERR_NOMEM;
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
            ii42_query_term_free(clone_out);
            return II42_ERR_NOMEM;
        }
        memcpy(clone_out->tokens[i], term->tokens[i], token_len + 1);
        clone_out->token_lens[i] = token_len;
    }

    return II42_OK;
}

void
ii42_query_init(ii42_query *query)
{
    if (query == NULL)
    {
        return;
    }

    memset(query, 0, sizeof(*query));
    query->mode = II42_QUERY_MODE_CLASSIC;
}

void
ii42_query_free(ii42_query *query)
{
    size_t i;

    if (query == NULL)
    {
        return;
    }

    for (i = 0; i < query->len; i++)
    {
        ii42_query_term_free(&query->terms[i]);
    }
    free(query->terms);
    ii42_query_node_free(query->root);
    memset(query, 0, sizeof(*query));
}

static bool
ii42_query_node_is_simple_term_disjunction(
    const ii42_query *query,
    const ii42_query_node *node
)
{
    const ii42_query_term *term;

    if (query == NULL || node == NULL)
    {
        return false;
    }

    if (node->kind == II42_QUERY_NODE_OR)
    {
        return
            ii42_query_node_is_simple_term_disjunction(
                query,
                node->left
            ) &&
            ii42_query_node_is_simple_term_disjunction(
                query,
                node->right
            );
    }

    if (node->kind != II42_QUERY_NODE_TERM ||
        node->term_index >= query->len)
    {
        return false;
    }

    term = &query->terms[node->term_index];
    return term->kind == II42_QUERY_TERM &&
           term->len == 1 &&
           term->tokens != NULL &&
           term->tokens[0] != NULL;
}

bool
ii42_query_is_simple_term_query(const ii42_query *query)
{
    size_t i;

    if (query == NULL || query->len == 0)
    {
        return false;
    }

    if (query->mode == II42_QUERY_MODE_BOOLEAN_AST)
    {
        return ii42_query_node_is_simple_term_disjunction(
            query,
            query->root
        );
    }

    for (i = 0; i < query->len; i++)
    {
        const ii42_query_term *term = &query->terms[i];

        if (term->kind != II42_QUERY_TERM ||
            term->occur != II42_QUERY_SHOULD ||
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
ii42_query_uses_boolean_ast(const ii42_query *query)
{
    return query != NULL &&
           query->mode == II42_QUERY_MODE_BOOLEAN_AST &&
           query->root != NULL;
}

bool
ii42_query_has_phrase(const ii42_query *query)
{
    size_t i;

    if (query == NULL)
    {
        return false;
    }

    for (i = 0; i < query->len; i++)
    {
        if (query->terms[i].kind == II42_QUERY_PHRASE)
        {
            return true;
        }
    }

    return false;
}

static ii42_query_node *
ii42_query_node_new(ii42_query_node_kind kind)
{
    ii42_query_node *node;

    node = calloc(1, sizeof(*node));
    if (node == NULL)
    {
        return NULL;
    }

    node->kind = kind;
    node->term_index = SIZE_MAX;
    return node;
}

static ii42_query_node *
ii42_query_node_term(size_t term_index)
{
    ii42_query_node *node;

    node = ii42_query_node_new(II42_QUERY_NODE_TERM);
    if (node == NULL)
    {
        return NULL;
    }

    node->term_index = term_index;
    return node;
}

static ii42_query_node *
ii42_query_node_unary(
    ii42_query_node_kind kind,
    ii42_query_node *child
)
{
    ii42_query_node *node;

    if (child == NULL)
    {
        return NULL;
    }

    node = ii42_query_node_new(kind);
    if (node == NULL)
    {
        ii42_query_node_free(child);
        return NULL;
    }

    node->left = child;
    return node;
}

static ii42_query_node *
ii42_query_node_binary(
    ii42_query_node_kind kind,
    ii42_query_node *left,
    ii42_query_node *right
)
{
    ii42_query_node *node;

    if (left == NULL)
    {
        return right;
    }
    if (right == NULL)
    {
        return left;
    }

    node = ii42_query_node_new(kind);
    if (node == NULL)
    {
        ii42_query_node_free(left);
        ii42_query_node_free(right);
        return NULL;
    }

    node->left = left;
    node->right = right;
    return node;
}

static const char *
ii42_skip_space(const char *cursor)
{
    while (*cursor != '\0' && isspace((unsigned char) *cursor))
    {
        cursor++;
    }

    return cursor;
}

static char *
ii42_copy_span(const char *start, size_t len)
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
ii42_query_token_free(ii42_query_token *token)
{
    if (token == NULL)
    {
        return;
    }

    ii42_query_term_free(&token->term);
}

static void
ii42_query_tokens_free(
    ii42_query_token *tokens,
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
        ii42_query_token_free(&tokens[i]);
    }
    free(tokens);
}

static ii42_status
ii42_append_token(
    ii42_query_token **tokens,
    size_t *len,
    const ii42_query_token *token
)
{
    ii42_query_token *resized;

    resized = realloc(*tokens, sizeof(*resized) * (*len + 1));
    if (resized == NULL)
    {
        return II42_ERR_NOMEM;
    }

    *tokens = resized;
    (*tokens)[*len] = *token;
    (*len)++;
    return II42_OK;
}

static ii42_status
ii42_split_phrase(
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
        token = ii42_copy_span(term_start, span_len);
        if (token == NULL)
        {
            size_t i;

            for (i = 0; i < token_len; i++)
            {
                free(tokens[i]);
            }
            free(tokens);
            free(token_lens);
            return II42_ERR_NOMEM;
        }
        if (token_len >= II42_QUERY_MAX_TOKENS)
        {
            size_t i;

            free(token);
            for (i = 0; i < token_len; i++)
            {
                free(tokens[i]);
            }
            free(tokens);
            free(token_lens);
            return II42_ERR_RANGE;
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
            return II42_ERR_NOMEM;
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
        return II42_ERR_INVALID;
    }

    *tokens_out = tokens;
    *token_lens_out = token_lens;
    *len_out = token_len;
    return II42_OK;
}

static ii42_status
ii42_tokenize_query_string(
    const char *input,
    ii42_query_token **tokens_out,
    size_t *len_out,
    bool *has_boolean_syntax_out
)
{
    const char *cursor;
    ii42_query_token *tokens = NULL;
    size_t len = 0;
    bool has_boolean_syntax = false;

    *tokens_out = NULL;
    *len_out = 0;
    *has_boolean_syntax_out = false;
    if (input == NULL)
    {
        return II42_ERR_INVALID;
    }

    cursor = input;
    while (true)
    {
        ii42_query_token token = {0};
        const char *start;
        size_t span_len;
        ii42_status status;

        cursor = ii42_skip_space(cursor);
        if (*cursor == '\0')
        {
            break;
        }

        if (*cursor == '+')
        {
            token.kind = II42_QUERY_TOKEN_PLUS;
            cursor++;
        }
        else if (*cursor == '-')
        {
            token.kind = II42_QUERY_TOKEN_MINUS;
            cursor++;
        }
        else if (*cursor == '(')
        {
            token.kind = II42_QUERY_TOKEN_LPAREN;
            has_boolean_syntax = true;
            cursor++;
        }
        else if (*cursor == ')')
        {
            token.kind = II42_QUERY_TOKEN_RPAREN;
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
                ii42_query_tokens_free(tokens, len);
                return II42_ERR_INVALID;
            }

            token.kind = II42_QUERY_TOKEN_PHRASE;
            token.term.kind = II42_QUERY_PHRASE;
            status = ii42_split_phrase(
                start,
                (size_t) (cursor - start),
                &token.term.tokens,
                &token.term.token_lens,
                &token.term.len
            );
            if (status != II42_OK)
            {
                ii42_query_tokens_free(tokens, len);
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
                ii42_query_tokens_free(tokens, len);
                return II42_ERR_INVALID;
            }

            if (span_len == 3 && memcmp(start, "AND", 3) == 0)
            {
                token.kind = II42_QUERY_TOKEN_AND;
                has_boolean_syntax = true;
            }
            else if (span_len == 2 && memcmp(start, "OR", 2) == 0)
            {
                token.kind = II42_QUERY_TOKEN_OR;
                has_boolean_syntax = true;
            }
            else if (span_len == 3 && memcmp(start, "NOT", 3) == 0)
            {
                token.kind = II42_QUERY_TOKEN_NOT;
                has_boolean_syntax = true;
            }
            else
            {
                token.kind = II42_QUERY_TOKEN_TERM;
                token.term.kind = II42_QUERY_TERM;
                if (start[span_len - 1] == '*')
                {
                    if (span_len == 1)
                    {
                        ii42_query_tokens_free(tokens, len);
                        return II42_ERR_INVALID;
                    }
                    token.kind = II42_QUERY_TOKEN_PREFIX;
                    token.term.kind = II42_QUERY_PREFIX;
                    span_len--;
                }

                token.term.tokens = malloc(sizeof(*token.term.tokens));
                token.term.token_lens = malloc(sizeof(*token.term.token_lens));
                if (token.term.tokens == NULL || token.term.token_lens == NULL)
                {
                    ii42_query_token_free(&token);
                    ii42_query_tokens_free(tokens, len);
                    return II42_ERR_NOMEM;
                }
                token.term.tokens[0] = ii42_copy_span(start, span_len);
                if (token.term.tokens[0] == NULL)
                {
                    ii42_query_token_free(&token);
                    ii42_query_tokens_free(tokens, len);
                    return II42_ERR_NOMEM;
                }
                token.term.token_lens[0] = span_len;
                token.term.len = 1;
            }
        }

        if (len >= II42_QUERY_MAX_TOKENS)
        {
            ii42_query_token_free(&token);
            ii42_query_tokens_free(tokens, len);
            return II42_ERR_RANGE;
        }
        status = ii42_append_token(&tokens, &len, &token);
        if (status != II42_OK)
        {
            ii42_query_token_free(&token);
            ii42_query_tokens_free(tokens, len);
            return status;
        }
    }

    *tokens_out = tokens;
    *len_out = len;
    *has_boolean_syntax_out = has_boolean_syntax;
    return II42_OK;
}

static ii42_status
ii42_append_term(
    ii42_query *query,
    const ii42_query_term *term,
    size_t *index_out
)
{
    ii42_query_term *terms;
    ii42_query_term cloned;
    size_t new_len;
    ii42_status status;

    if (query == NULL || term == NULL)
    {
        return II42_ERR_INVALID;
    }

    status = ii42_query_term_clone(term, &cloned);
    if (status != II42_OK)
    {
        return status;
    }

    new_len = query->len + 1;
    terms = realloc(query->terms, sizeof(*terms) * new_len);
    if (terms == NULL)
    {
        ii42_query_term_free(&cloned);
        return II42_ERR_NOMEM;
    }

    query->terms = terms;
    query->terms[query->len] = cloned;
    if (index_out != NULL)
    {
        *index_out = query->len;
    }
    query->len = new_len;
    return II42_OK;
}

static ii42_query_node *
ii42_query_build_classic_root(const ii42_query *query)
{
    ii42_query_node *must_root = NULL;
    ii42_query_node *should_root = NULL;
    ii42_query_node *negative_root = NULL;
    size_t i;

    if (query == NULL)
    {
        return NULL;
    }

    for (i = 0; i < query->len; i++)
    {
        const ii42_query_term *term = &query->terms[i];
        ii42_query_node *leaf;

        leaf = ii42_query_node_term(i);
        if (leaf == NULL)
        {
            ii42_query_node_free(must_root);
            ii42_query_node_free(should_root);
            ii42_query_node_free(negative_root);
            return NULL;
        }

        if (term->occur == II42_QUERY_MUST)
        {
            must_root = ii42_query_node_binary(
                II42_QUERY_NODE_AND,
                must_root,
                leaf
            );
        }
        else if (term->occur == II42_QUERY_MUST_NOT)
        {
            negative_root = ii42_query_node_binary(
                II42_QUERY_NODE_AND,
                negative_root,
                ii42_query_node_unary(II42_QUERY_NODE_NOT, leaf)
            );
        }
        else
        {
            should_root = ii42_query_node_binary(
                II42_QUERY_NODE_OR,
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

    /*
     * Classic SHOULD terms contribute scores but do not constrain a query that
     * already has MUST terms. Their temporary AST is therefore not attached
     * to the match predicate and must be released here.
     */
    ii42_query_node_free(should_root);

    return ii42_query_node_binary(
        II42_QUERY_NODE_AND,
        must_root,
        negative_root
    );
}

static bool
ii42_parser_token_can_start_unary(
    ii42_query_token_kind kind
)
{
    return kind == II42_QUERY_TOKEN_TERM ||
           kind == II42_QUERY_TOKEN_PREFIX ||
           kind == II42_QUERY_TOKEN_PHRASE ||
           kind == II42_QUERY_TOKEN_LPAREN ||
           kind == II42_QUERY_TOKEN_PLUS ||
           kind == II42_QUERY_TOKEN_MINUS ||
           kind == II42_QUERY_TOKEN_NOT;
}

static const ii42_query_token *
ii42_query_parser_peek(const ii42_query_parser *parser)
{
    static const ii42_query_token eof = {
        .kind = II42_QUERY_TOKEN_EOF
    };

    if (parser->pos >= parser->len)
    {
        return &eof;
    }

    return &parser->tokens[parser->pos];
}

static void
ii42_query_parser_consume(ii42_query_parser *parser)
{
    if (parser->pos < parser->len)
    {
        parser->pos++;
    }
}

static ii42_status
ii42_parse_boolean_or(
    ii42_query_parser *parser,
    ii42_query_node **node_out
);

static ii42_status
ii42_parse_boolean_primary(
    ii42_query_parser *parser,
    ii42_query_node **node_out
)
{
    const ii42_query_token *token;
    ii42_status status;

    token = ii42_query_parser_peek(parser);
    if (token->kind == II42_QUERY_TOKEN_LPAREN)
    {
        ii42_query_node *node;

        if (parser->depth >= II42_QUERY_MAX_NESTING)
        {
            return II42_ERR_RANGE;
        }
        ii42_query_parser_consume(parser);
        parser->depth++;
        status = ii42_parse_boolean_or(parser, &node);
        parser->depth--;
        if (status != II42_OK)
        {
            return status;
        }
        if (ii42_query_parser_peek(parser)->kind !=
            II42_QUERY_TOKEN_RPAREN)
        {
            ii42_query_node_free(node);
            return II42_ERR_INVALID;
        }
        ii42_query_parser_consume(parser);
        *node_out = node;
        return II42_OK;
    }

    if (token->kind == II42_QUERY_TOKEN_TERM ||
        token->kind == II42_QUERY_TOKEN_PREFIX ||
        token->kind == II42_QUERY_TOKEN_PHRASE)
    {
        ii42_query_node *node;
        size_t term_index = SIZE_MAX;

        status = ii42_append_term(parser->query, &token->term, &term_index);
        if (status != II42_OK)
        {
            return status;
        }

        node = ii42_query_node_term(term_index);
        if (node == NULL)
        {
            return II42_ERR_NOMEM;
        }
        ii42_query_parser_consume(parser);
        *node_out = node;
        return II42_OK;
    }

    return II42_ERR_INVALID;
}

static ii42_status
ii42_parse_boolean_unary(
    ii42_query_parser *parser,
    ii42_query_node **node_out
)
{
    const ii42_query_token *token;
    ii42_query_node *child;
    ii42_status status;

    token = ii42_query_parser_peek(parser);
    if (token->kind == II42_QUERY_TOKEN_PLUS)
    {
        if (parser->depth >= II42_QUERY_MAX_NESTING)
        {
            return II42_ERR_RANGE;
        }
        ii42_query_parser_consume(parser);
        parser->depth++;
        status = ii42_parse_boolean_unary(parser, node_out);
        parser->depth--;
        return status;
    }
    if (token->kind == II42_QUERY_TOKEN_MINUS ||
        token->kind == II42_QUERY_TOKEN_NOT)
    {
        if (parser->depth >= II42_QUERY_MAX_NESTING)
        {
            return II42_ERR_RANGE;
        }
        ii42_query_parser_consume(parser);
        parser->depth++;
        status = ii42_parse_boolean_unary(parser, &child);
        parser->depth--;
        if (status != II42_OK)
        {
            return status;
        }

        *node_out = ii42_query_node_unary(
            II42_QUERY_NODE_NOT,
            child
        );
        if (*node_out == NULL)
        {
            return II42_ERR_NOMEM;
        }
        return II42_OK;
    }

    return ii42_parse_boolean_primary(parser, node_out);
}

static ii42_status
ii42_parse_boolean_and(
    ii42_query_parser *parser,
    ii42_query_node **node_out
)
{
    ii42_query_node *node;
    ii42_status status;

    status = ii42_parse_boolean_unary(parser, &node);
    if (status != II42_OK)
    {
        return status;
    }

    while (ii42_query_parser_peek(parser)->kind ==
           II42_QUERY_TOKEN_AND)
    {
        ii42_query_node *rhs;

        ii42_query_parser_consume(parser);
        status = ii42_parse_boolean_unary(parser, &rhs);
        if (status != II42_OK)
        {
            ii42_query_node_free(node);
            return status;
        }

        node = ii42_query_node_binary(
            II42_QUERY_NODE_AND,
            node,
            rhs
        );
        if (node == NULL)
        {
            return II42_ERR_NOMEM;
        }
    }

    *node_out = node;
    return II42_OK;
}

static ii42_status
ii42_parse_boolean_or(
    ii42_query_parser *parser,
    ii42_query_node **node_out
)
{
    ii42_query_node *node;
    ii42_status status;

    status = ii42_parse_boolean_and(parser, &node);
    if (status != II42_OK)
    {
        return status;
    }

    while (true)
    {
        const ii42_query_token *token;
        ii42_query_node *rhs;

        token = ii42_query_parser_peek(parser);
        if (token->kind == II42_QUERY_TOKEN_OR)
        {
            ii42_query_parser_consume(parser);
        }
        else if (!ii42_parser_token_can_start_unary(token->kind))
        {
            break;
        }

        status = ii42_parse_boolean_and(parser, &rhs);
        if (status != II42_OK)
        {
            ii42_query_node_free(node);
            return status;
        }

        node = ii42_query_node_binary(
            II42_QUERY_NODE_OR,
            node,
            rhs
        );
        if (node == NULL)
        {
            return II42_ERR_NOMEM;
        }
    }

    *node_out = node;
    return II42_OK;
}

static ii42_status
ii42_parse_boolean_tokens(
    const ii42_query_token *tokens,
    size_t len,
    ii42_query *query_out
)
{
    ii42_query query = {0};
    ii42_query_parser parser;
    ii42_status status;

    ii42_query_init(&query);
    query.mode = II42_QUERY_MODE_BOOLEAN_AST;

    parser.tokens = tokens;
    parser.len = len;
    parser.pos = 0;
    parser.depth = 0;
    parser.query = &query;

    status = ii42_parse_boolean_or(&parser, &query.root);
    if (status != II42_OK)
    {
        ii42_query_free(&query);
        return status;
    }
    if (query.root == NULL || parser.pos != len)
    {
        ii42_query_free(&query);
        return II42_ERR_INVALID;
    }

    *query_out = query;
    return II42_OK;
}

static ii42_status
ii42_parse_classic_tokens(
    const ii42_query_token *tokens,
    size_t len,
    ii42_query *query_out
)
{
    ii42_query query = {0};
    ii42_query_occur next_occur = II42_QUERY_SHOULD;
    bool expect_term = true;
    size_t i;

    ii42_query_init(&query);
    query.mode = II42_QUERY_MODE_CLASSIC;

    for (i = 0; i < len; i++)
    {
        ii42_query_term term;
        ii42_status status;

        if (tokens[i].kind == II42_QUERY_TOKEN_PLUS)
        {
            next_occur = II42_QUERY_MUST;
            expect_term = true;
            continue;
        }
        if (tokens[i].kind == II42_QUERY_TOKEN_MINUS)
        {
            next_occur = II42_QUERY_MUST_NOT;
            expect_term = true;
            continue;
        }
        if (tokens[i].kind != II42_QUERY_TOKEN_TERM &&
            tokens[i].kind != II42_QUERY_TOKEN_PREFIX &&
            tokens[i].kind != II42_QUERY_TOKEN_PHRASE)
        {
            ii42_query_free(&query);
            return II42_ERR_INVALID;
        }

        term = tokens[i].term;
        term.occur = next_occur;
        status = ii42_append_term(&query, &term, NULL);
        if (status != II42_OK)
        {
            ii42_query_free(&query);
            return status;
        }
        next_occur = II42_QUERY_SHOULD;
        expect_term = false;
    }

    if (expect_term && len > 0)
    {
        ii42_query_free(&query);
        return II42_ERR_INVALID;
    }

    query.root = ii42_query_build_classic_root(&query);
    if (query.len > 0 && query.root == NULL)
    {
        ii42_query_free(&query);
        return II42_ERR_NOMEM;
    }

    *query_out = query;
    return II42_OK;
}

static ii42_query_node *
ii42_query_rewrite_node(
    ii42_query_node *node,
    const size_t *old_to_new,
    size_t old_len
)
{
    if (node == NULL)
    {
        return NULL;
    }

    if (node->kind == II42_QUERY_NODE_TERM)
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

    node->left = ii42_query_rewrite_node(
        node->left,
        old_to_new,
        old_len
    );
    node->right = ii42_query_rewrite_node(
        node->right,
        old_to_new,
        old_len
    );

    if (node->kind == II42_QUERY_NODE_NOT)
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
        ii42_query_node *right = node->right;

        free(node);
        return right;
    }
    if (node->right == NULL)
    {
        ii42_query_node *left = node->left;

        free(node);
        return left;
    }

    return node;
}

void
ii42_query_rewrite_terms(
    ii42_query *query,
    const size_t *old_to_new,
    size_t old_len
)
{
    if (query == NULL || old_to_new == NULL)
    {
        return;
    }

    query->root = ii42_query_rewrite_node(
        query->root,
        old_to_new,
        old_len
    );
}

ii42_status
ii42_parse_query_string(
    const char *input,
    ii42_query *query_out
)
{
    ii42_query_token *tokens = NULL;
    size_t len = 0;
    bool has_boolean_syntax = false;
    ii42_status status;

    if (input == NULL || query_out == NULL)
    {
        return II42_ERR_INVALID;
    }

    status = ii42_tokenize_query_string(
        input,
        &tokens,
        &len,
        &has_boolean_syntax
    );
    if (status != II42_OK)
    {
        return status;
    }

    if (has_boolean_syntax)
    {
        status = ii42_parse_boolean_tokens(tokens, len, query_out);
    }
    else
    {
        status = ii42_parse_classic_tokens(tokens, len, query_out);
    }

    ii42_query_tokens_free(tokens, len);
    return status;
}
