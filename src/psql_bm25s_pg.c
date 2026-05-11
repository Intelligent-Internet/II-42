#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "varatt.h"

#include "psql_bm25s_core.h"
#include "psql_bm25s_storage.h"

PG_MODULE_MAGIC;

static void
psql_bm25s_validate_serialized_bytes(
    const uint8_t *bytes,
    size_t len
)
{
    psql_bm25s_index index;
    psql_bm25s_status status;

    psql_bm25s_index_init(&index);
    status = psql_bm25s_deserialize_index(bytes, len, &index);
    if (status != PSQL_BM25S_OK)
    {
        ereport(ERROR, (errmsg("invalid psql_bm25s_index value: %s",
                               psql_bm25s_strerror(status))));
    }
    psql_bm25s_index_free(&index);
}

PG_FUNCTION_INFO_V1(psql_bm25s_in);
Datum
psql_bm25s_in(PG_FUNCTION_ARGS)
{
    bytea *result;

    result = DatumGetByteaPP(
        DirectFunctionCall1(byteain, CStringGetDatum(PG_GETARG_CSTRING(0)))
    );

    psql_bm25s_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(result),
        (size_t) VARSIZE_ANY_EXHDR(result)
    );
    PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_out);
Datum
psql_bm25s_out(PG_FUNCTION_ARGS)
{
    bytea *raw = PG_GETARG_BYTEA_PP(0);

    psql_bm25s_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(raw),
        (size_t) VARSIZE_ANY_EXHDR(raw)
    );
    PG_RETURN_CSTRING(
        DatumGetCString(DirectFunctionCall1(byteaout, PG_GETARG_DATUM(0)))
    );
}

PG_FUNCTION_INFO_V1(psql_bm25s_recv);
Datum
psql_bm25s_recv(PG_FUNCTION_ARGS)
{
    bytea *result;

    result = DatumGetByteaPP(DirectFunctionCall1(bytearecv, PG_GETARG_DATUM(0)));

    psql_bm25s_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(result),
        (size_t) VARSIZE_ANY_EXHDR(result)
    );
    PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(psql_bm25s_send);
Datum
psql_bm25s_send(PG_FUNCTION_ARGS)
{
    bytea *raw = PG_GETARG_BYTEA_PP(0);

    psql_bm25s_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(raw),
        (size_t) VARSIZE_ANY_EXHDR(raw)
    );
    PG_RETURN_BYTEA_P(DatumGetByteaPP(DirectFunctionCall1(
        byteasend,
        PG_GETARG_DATUM(0)
    )));
}
