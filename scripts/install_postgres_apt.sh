#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo 'usage: install_postgres_apt.sh <pg-major>' >&2
    exit 1
fi

pg_major="$1"

if command -v sudo >/dev/null 2>&1; then
    sudo_cmd='sudo'
else
    sudo_cmd=''
fi

. /etc/os-release
codename="${VERSION_CODENAME:-}"

if [ -z "$codename" ]; then
    echo 'could not determine OS codename' >&2
    exit 1
fi

${sudo_cmd} apt-get update
${sudo_cmd} apt-get install -y \
    ca-certificates \
    curl \
    gnupg \
    lsb-release \
    zip \
    build-essential \
    cmake \
    pkg-config \
    python3-numpy \
    python3-onnx \
    python3-psycopg \
    python3-pytest \
    python3-scipy

${sudo_cmd} install -d -m 0755 /etc/apt/keyrings

if [ ! -f /etc/apt/keyrings/postgresql.gpg ]; then
    curl -fsSL https://www.postgresql.org/media/keys/ACCC4CF8.asc \
        | ${sudo_cmd} gpg --dearmor -o /etc/apt/keyrings/postgresql.gpg
fi

echo \
    "deb [signed-by=/etc/apt/keyrings/postgresql.gpg] "\
"http://apt.postgresql.org/pub/repos/apt ${codename}-pgdg main" \
    | ${sudo_cmd} tee /etc/apt/sources.list.d/pgdg.list >/dev/null

${sudo_cmd} apt-get update
${sudo_cmd} apt-get install -y \
    "postgresql-${pg_major}" \
    "postgresql-server-dev-${pg_major}"
