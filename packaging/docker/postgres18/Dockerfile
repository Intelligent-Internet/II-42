FROM postgres:18 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        libicu-dev \
        make \
        pkg-config \
        postgresql-server-dev-18 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY Makefile CMakeLists.txt psql_bm25s.control README.md ./
COPY src ./src
COPY sql ./sql

RUN make clean PG_CONFIG=/usr/bin/pg_config \
    && make PG_CONFIG=/usr/bin/pg_config \
    && make install DESTDIR=/tmp/psql_bm25s-stage \
        PG_CONFIG=/usr/bin/pg_config

FROM postgres:18

ARG PSQL_BM25S_VERSION

LABEL org.opencontainers.image.title='psql_bm25s PostgreSQL 18'
LABEL org.opencontainers.image.description='PostgreSQL 18 image with psql_bm25s preinstalled'
LABEL org.opencontainers.image.version="${PSQL_BM25S_VERSION}"
LABEL org.opencontainers.image.source='https://github.com/Intelligent-Internet/psql_bm25s'

COPY --from=build /tmp/psql_bm25s-stage/ /
COPY packaging/docker/postgres18/initdb/010-create-psql_bm25s.sh \
    /docker-entrypoint-initdb.d/010-create-psql_bm25s.sh

RUN mkdir -p /var/run/postgresql \
    && chown postgres:postgres /var/run/postgresql \
    && chmod 3775 /var/run/postgresql \
    && chmod 0755 /docker-entrypoint-initdb.d/010-create-psql_bm25s.sh
