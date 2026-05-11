#!/usr/bin/env bash

set -euo pipefail

create_extension() {
    local db_name="$1"
    psql \
        --username "$POSTGRES_USER" \
        --dbname "$db_name" \
        --set ON_ERROR_STOP=1 \
        --command 'CREATE EXTENSION IF NOT EXISTS psql_bm25s;'
}

create_extension postgres
create_extension template1

if [ "${POSTGRES_DB:-postgres}" != 'postgres' ]; then
    create_extension "${POSTGRES_DB}"
fi

