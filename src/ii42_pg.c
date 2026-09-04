#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "varatt.h"

#include "ii42_core.h"
#include "ii42_storage.h"

PG_MODULE_MAGIC;

static void
ii42_validate_serialized_bytes(
    const uint8_t *bytes,
    size_t len
)
{
    ii42_index index;
    ii42_status status;

    ii42_index_init(&index);
    status = ii42_deserialize_index(bytes, len, &index);
    if (status != II42_OK)
    {
        ereport(ERROR, (errmsg("invalid ii42_index value: %s",
                               ii42_strerror(status))));
    }
    ii42_index_free(&index);
}

PG_FUNCTION_INFO_V1(ii42_in);
Datum
ii42_in(PG_FUNCTION_ARGS)
{
    bytea *result;

    result = DatumGetByteaPP(
        DirectFunctionCall1(byteain, CStringGetDatum(PG_GETARG_CSTRING(0)))
    );

    ii42_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(result),
        (size_t) VARSIZE_ANY_EXHDR(result)
    );
    PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(ii42_out);
Datum
ii42_out(PG_FUNCTION_ARGS)
{
    bytea *raw = PG_GETARG_BYTEA_PP(0);

    ii42_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(raw),
        (size_t) VARSIZE_ANY_EXHDR(raw)
    );
    PG_RETURN_CSTRING(
        DatumGetCString(DirectFunctionCall1(byteaout, PG_GETARG_DATUM(0)))
    );
}

PG_FUNCTION_INFO_V1(ii42_recv);
Datum
ii42_recv(PG_FUNCTION_ARGS)
{
    bytea *result;

    result = DatumGetByteaPP(DirectFunctionCall1(bytearecv, PG_GETARG_DATUM(0)));

    ii42_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(result),
        (size_t) VARSIZE_ANY_EXHDR(result)
    );
    PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(ii42_send);
Datum
ii42_send(PG_FUNCTION_ARGS)
{
    bytea *raw = PG_GETARG_BYTEA_PP(0);

    ii42_validate_serialized_bytes(
        (const uint8_t *) VARDATA_ANY(raw),
        (size_t) VARSIZE_ANY_EXHDR(raw)
    );
    PG_RETURN_BYTEA_P(DatumGetByteaPP(DirectFunctionCall1(
        byteasend,
        PG_GETARG_DATUM(0)
    )));
}
