#include "psql_bm25s_pg_common.h"

#include "catalog/pg_type_d.h"
#include "utils/builtins.h"

static Oid
psql_bm25s_array_textlike_element_type(
    ArrayType *array,
    const char *context
)
{
    Oid element_type = ARR_ELEMTYPE(array);

    if (element_type != TEXTOID && element_type != VARCHAROID)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "%s must use text[] or varchar[] input",
                    context
                )
            )
        );
    }

    return element_type;
}

float *
psql_bm25s_array_read_weight_mask(ArrayType *array, uint32_t expected_len)
{
    float *weights;
    Datum *datums;
    bool *nulls;
    int nelems;
    int i;

    if (array == NULL)
    {
        return NULL;
    }

    if (ARR_NDIM(array) != 1)
    {
        ereport(ERROR, (errmsg("weight_mask must be a one-dimensional real[]")));
    }

    deconstruct_array(
        array,
        FLOAT4OID,
        4,
        true,
        TYPALIGN_INT,
        &datums,
        &nulls,
        &nelems
    );

    if (nelems != (int) expected_len)
    {
        ereport(ERROR, (errmsg("weight_mask length must equal number of indexed documents")));
    }

    weights = palloc(sizeof(*weights) * expected_len);
    for (i = 0; i < nelems; i++)
    {
        if (nulls[i])
        {
            ereport(ERROR, (errmsg("weight_mask cannot contain NULL elements")));
        }
        weights[i] = DatumGetFloat4(datums[i]);
    }

    if (datums != NULL)
    {
        pfree(datums);
    }
    if (nulls != NULL)
    {
        pfree(nulls);
    }

    return weights;
}

uint32_t *
psql_bm25s_array_read_query_ids(ArrayType *array, size_t *query_len_out)
{
    Datum *datums;
    bool *nulls;
    int nelems;
    int i;
    uint32_t *query_ids;

    if (ARR_NDIM(array) != 1)
    {
        ereport(ERROR, (errmsg("query ids must be a one-dimensional int4[]")));
    }

    deconstruct_array(
        array,
        INT4OID,
        4,
        true,
        TYPALIGN_INT,
        &datums,
        &nulls,
        &nelems
    );

    query_ids = palloc(sizeof(*query_ids) * Max(nelems, 1));
    for (i = 0; i < nelems; i++)
    {
        if (nulls[i])
        {
            ereport(ERROR, (errmsg("query ids cannot contain NULL elements")));
        }
        query_ids[i] = (uint32_t) DatumGetInt32(datums[i]);
    }

    if (datums != NULL)
    {
        pfree(datums);
    }
    if (nulls != NULL)
    {
        pfree(nulls);
    }

    *query_len_out = (size_t) nelems;
    return query_ids;
}

char **
psql_bm25s_array_read_query_tokens(ArrayType *array, size_t *query_len_out)
{
    Datum *datums;
    bool *nulls;
    int nelems;
    int i;
    char **tokens;
    Oid element_type;

    if (ARR_NDIM(array) != 1)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "query tokens must be a one-dimensional text[] or varchar[]"
                )
            )
        );
    }

    element_type = psql_bm25s_array_textlike_element_type(
        array,
        "query tokens"
    );

    deconstruct_array(
        array,
        element_type,
        -1,
        false,
        TYPALIGN_INT,
        &datums,
        &nulls,
        &nelems
    );

    tokens = palloc(sizeof(*tokens) * Max(nelems, 1));
    for (i = 0; i < nelems; i++)
    {
        if (nulls[i])
        {
            ereport(ERROR, (errmsg("query tokens cannot contain NULL elements")));
        }
        tokens[i] = TextDatumGetCString(datums[i]);
    }

    if (datums != NULL)
    {
        pfree(datums);
    }
    if (nulls != NULL)
    {
        pfree(nulls);
    }

    *query_len_out = (size_t) nelems;
    return tokens;
}

char **
psql_bm25s_array_read_doc_tokens(ArrayType *array, size_t *doc_len_out)
{
    Datum *datums;
    bool *nulls;
    int nelems;
    int i;
    char **tokens;
    Oid element_type;

    if (ARR_NDIM(array) != 0 && ARR_NDIM(array) != 1)
    {
        ereport(
            ERROR,
            (
                errmsg(
                    "document tokens must be a one-dimensional text[] or varchar[]"
                )
            )
        );
    }

    element_type = psql_bm25s_array_textlike_element_type(
        array,
        "document tokens"
    );

    deconstruct_array(
        array,
        element_type,
        -1,
        false,
        TYPALIGN_INT,
        &datums,
        &nulls,
        &nelems
    );

    tokens = palloc(sizeof(*tokens) * Max(nelems, 1));
    for (i = 0; i < nelems; i++)
    {
        if (nulls[i])
        {
            ereport(ERROR, (errmsg("document tokens cannot contain NULL elements")));
        }
        tokens[i] = TextDatumGetCString(datums[i]);
    }

    if (datums != NULL)
    {
        pfree(datums);
    }
    if (nulls != NULL)
    {
        pfree(nulls);
    }

    *doc_len_out = (size_t) nelems;
    return tokens;
}
