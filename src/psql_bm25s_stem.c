#include "psql_bm25s_stem.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool
psql_bm25s_stem_is_lower_ascii_letter(char c)
{
    return c >= 'a' && c <= 'z';
}

static bool
psql_bm25s_stem_is_consonant(const char *token, int i)
{
    char c;

    c = token[i];
    switch (c)
    {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
            return false;
        case 'y':
            if (i == 0)
            {
                return true;
            }
            return !psql_bm25s_stem_is_consonant(token, i - 1);
        default:
            return true;
    }
}

static int
psql_bm25s_stem_measure(const char *token, int len)
{
    int i;
    int measure;

    i = 0;
    measure = 0;
    while (i < len)
    {
        while (i < len && psql_bm25s_stem_is_consonant(token, i))
        {
            i++;
        }
        while (i < len && !psql_bm25s_stem_is_consonant(token, i))
        {
            i++;
        }
        if (i < len)
        {
            measure++;
        }
    }

    return measure;
}

static bool
psql_bm25s_stem_contains_vowel(const char *token, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        if (!psql_bm25s_stem_is_consonant(token, i))
        {
            return true;
        }
    }
    return false;
}

static bool
psql_bm25s_stem_ends_with_double_consonant(const char *token, int len)
{
    if (len < 2)
    {
        return false;
    }

    return token[len - 1] == token[len - 2] &&
           psql_bm25s_stem_is_consonant(token, len - 1);
}

static bool
psql_bm25s_stem_is_cvc(const char *token, int len)
{
    char c;

    if (len < 3)
    {
        return false;
    }
    if (!psql_bm25s_stem_is_consonant(token, len - 1) ||
        psql_bm25s_stem_is_consonant(token, len - 2) ||
        !psql_bm25s_stem_is_consonant(token, len - 3))
    {
        return false;
    }

    c = token[len - 1];
    return c != 'w' && c != 'x' && c != 'y';
}

static bool
psql_bm25s_stem_ends_with(
    const char *token,
    int len,
    const char *suffix,
    size_t suffix_len
)
{
    if (len < (int) suffix_len)
    {
        return false;
    }

    return memcmp(token + len - (int) suffix_len, suffix, suffix_len) == 0;
}

static void
psql_bm25s_stem_replace_suffix(
    char *token,
    int *len,
    size_t suffix_len,
    const char *replacement
)
{
    size_t repl_len;

    repl_len = strlen(replacement);
    memcpy(token + *len - (int) suffix_len, replacement, repl_len);
    *len = *len - (int) suffix_len + (int) repl_len;
    token[*len] = '\0';
}

static void
psql_bm25s_stem_step_1a(char *token, int *len)
{
    if (psql_bm25s_stem_ends_with(token, *len, "sses", 4))
    {
        psql_bm25s_stem_replace_suffix(token, len, 4, "ss");
    }
    else if (psql_bm25s_stem_ends_with(token, *len, "ies", 3))
    {
        psql_bm25s_stem_replace_suffix(token, len, 3, "i");
    }
    else if (psql_bm25s_stem_ends_with(token, *len, "ss", 2))
    {
        return;
    }
    else if (psql_bm25s_stem_ends_with(token, *len, "s", 1))
    {
        psql_bm25s_stem_replace_suffix(token, len, 1, "");
    }
}

static void
psql_bm25s_stem_step_1b_postprocess(char *token, int *len)
{
    if (psql_bm25s_stem_ends_with(token, *len, "at", 2) ||
        psql_bm25s_stem_ends_with(token, *len, "bl", 2) ||
        psql_bm25s_stem_ends_with(token, *len, "iz", 2))
    {
        token[*len] = 'e';
        *len += 1;
        token[*len] = '\0';
    }
    else if (psql_bm25s_stem_ends_with_double_consonant(token, *len) &&
             token[*len - 1] != 'l' &&
             token[*len - 1] != 's' &&
             token[*len - 1] != 'z')
    {
        *len -= 1;
        token[*len] = '\0';
    }
    else if (psql_bm25s_stem_measure(token, *len) == 1 &&
             psql_bm25s_stem_is_cvc(token, *len))
    {
        token[*len] = 'e';
        *len += 1;
        token[*len] = '\0';
    }
}

static void
psql_bm25s_stem_step_1b(char *token, int *len)
{
    int stem_len;

    if (psql_bm25s_stem_ends_with(token, *len, "eed", 3))
    {
        stem_len = *len - 3;
        if (psql_bm25s_stem_measure(token, stem_len) > 0)
        {
            psql_bm25s_stem_replace_suffix(token, len, 3, "ee");
        }
        return;
    }

    if (psql_bm25s_stem_ends_with(token, *len, "ed", 2))
    {
        stem_len = *len - 2;
        if (psql_bm25s_stem_contains_vowel(token, stem_len))
        {
            psql_bm25s_stem_replace_suffix(token, len, 2, "");
            psql_bm25s_stem_step_1b_postprocess(token, len);
        }
        return;
    }

    if (psql_bm25s_stem_ends_with(token, *len, "ing", 3))
    {
        stem_len = *len - 3;
        if (psql_bm25s_stem_contains_vowel(token, stem_len))
        {
            psql_bm25s_stem_replace_suffix(token, len, 3, "");
            psql_bm25s_stem_step_1b_postprocess(token, len);
        }
    }
}

static void
psql_bm25s_stem_step_1c(char *token, int *len)
{
    if (*len > 1 &&
        token[*len - 1] == 'y' &&
        psql_bm25s_stem_contains_vowel(token, *len - 1))
    {
        token[*len - 1] = 'i';
    }
}

typedef struct psql_bm25s_stem_rule
{
    const char *suffix;
    const char *replacement;
} psql_bm25s_stem_rule;

static bool
psql_bm25s_stem_apply_rules(
    char *token,
    int *len,
    const psql_bm25s_stem_rule *rules,
    size_t num_rules,
    int min_measure
)
{
    size_t i;

    for (i = 0; i < num_rules; i++)
    {
        size_t suffix_len;
        int stem_len;

        suffix_len = strlen(rules[i].suffix);
        if (!psql_bm25s_stem_ends_with(
                token,
                *len,
                rules[i].suffix,
                suffix_len))
        {
            continue;
        }

        stem_len = *len - (int) suffix_len;
        if (psql_bm25s_stem_measure(token, stem_len) > min_measure)
        {
            psql_bm25s_stem_replace_suffix(
                token,
                len,
                suffix_len,
                rules[i].replacement
            );
        }
        return true;
    }

    return false;
}

static void
psql_bm25s_stem_step_2(char *token, int *len)
{
    static const psql_bm25s_stem_rule rules[] = {
        {"ational", "ate"},
        {"tional", "tion"},
        {"enci", "ence"},
        {"anci", "ance"},
        {"izer", "ize"},
        {"abli", "able"},
        {"alli", "al"},
        {"entli", "ent"},
        {"eli", "e"},
        {"ousli", "ous"},
        {"ization", "ize"},
        {"ation", "ate"},
        {"ator", "ate"},
        {"alism", "al"},
        {"iveness", "ive"},
        {"fulness", "ful"},
        {"ousness", "ous"},
        {"aliti", "al"},
        {"iviti", "ive"},
        {"biliti", "ble"},
        {"logi", "log"}
    };

    (void) psql_bm25s_stem_apply_rules(
        token,
        len,
        rules,
        sizeof(rules) / sizeof(rules[0]),
        0
    );
}

static void
psql_bm25s_stem_step_3(char *token, int *len)
{
    static const psql_bm25s_stem_rule rules[] = {
        {"icate", "ic"},
        {"ative", ""},
        {"alize", "al"},
        {"iciti", "ic"},
        {"ical", "ic"},
        {"ful", ""},
        {"ness", ""}
    };

    (void) psql_bm25s_stem_apply_rules(
        token,
        len,
        rules,
        sizeof(rules) / sizeof(rules[0]),
        0
    );
}

static void
psql_bm25s_stem_step_4(char *token, int *len)
{
    static const char *suffixes[] = {
        "ement",
        "ance",
        "ence",
        "able",
        "ible",
        "ment",
        "ant",
        "ent",
        "ism",
        "ate",
        "iti",
        "ous",
        "ive",
        "ize",
        "al",
        "er",
        "ic",
        "ou"
    };
    size_t i;

    if (psql_bm25s_stem_ends_with(token, *len, "ion", 3))
    {
        int stem_len;

        stem_len = *len - 3;
        if (stem_len > 0 &&
            psql_bm25s_stem_measure(token, stem_len) > 1 &&
            (token[stem_len - 1] == 's' || token[stem_len - 1] == 't'))
        {
            psql_bm25s_stem_replace_suffix(token, len, 3, "");
        }
        return;
    }

    for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++)
    {
        size_t suffix_len;
        int stem_len;

        suffix_len = strlen(suffixes[i]);
        if (!psql_bm25s_stem_ends_with(token, *len, suffixes[i], suffix_len))
        {
            continue;
        }
        stem_len = *len - (int) suffix_len;
        if (psql_bm25s_stem_measure(token, stem_len) > 1)
        {
            psql_bm25s_stem_replace_suffix(token, len, suffix_len, "");
        }
        return;
    }
}

static void
psql_bm25s_stem_step_5(char *token, int *len)
{
    if (*len > 0 && token[*len - 1] == 'e')
    {
        int stem_len;
        int measure;

        stem_len = *len - 1;
        measure = psql_bm25s_stem_measure(token, stem_len);
        if (measure > 1 ||
            (measure == 1 && !psql_bm25s_stem_is_cvc(token, stem_len)))
        {
            *len -= 1;
            token[*len] = '\0';
        }
    }

    if (*len > 1 &&
        token[*len - 1] == 'l' &&
        token[*len - 2] == 'l' &&
        psql_bm25s_stem_measure(token, *len) > 1)
    {
        *len -= 1;
        token[*len] = '\0';
    }
}

void
psql_bm25s_stem_english_porter(char *token)
{
    int len;
    int i;

    if (token == NULL)
    {
        return;
    }

    len = (int) strlen(token);
    if (len < 3)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        if (!psql_bm25s_stem_is_lower_ascii_letter(token[i]))
        {
            return;
        }
    }

    psql_bm25s_stem_step_1a(token, &len);
    psql_bm25s_stem_step_1b(token, &len);
    psql_bm25s_stem_step_1c(token, &len);
    psql_bm25s_stem_step_2(token, &len);
    psql_bm25s_stem_step_3(token, &len);
    psql_bm25s_stem_step_4(token, &len);
    psql_bm25s_stem_step_5(token, &len);
}
