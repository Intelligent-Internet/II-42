#!/usr/bin/env bash

set -euo pipefail

version=''
pg_config=''

while [ "$#" -gt 0 ]; do
    case "$1" in
        --version)
            version="$2"
            shift 2
            ;;
        --pg-config)
            pg_config="$2"
            shift 2
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [ -z "$version" ]; then
    echo '--version is required' >&2
    exit 1
fi

if [ -z "$pg_config" ]; then
    pg_config="$(command -v pg_config || true)"
fi

if [ -z "$pg_config" ] || [ ! -x "$pg_config" ]; then
    echo 'pg_config not found' >&2
    exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
arch="$(uname -m)"
os_name="$(uname -s | tr '[:upper:]' '[:lower:]')"
pg_version="$("$pg_config" --version | awk '{print $2}')"
pg_major="${pg_version%%.*}"
package_name="psql_bm25s-v${version}-${os_name}-${arch}-pg${pg_major}"
dist_dir="${repo_root}/dist"
stage_dir="${dist_dir}/${package_name}"
zip_path="${dist_dir}/${package_name}.zip"
checksum_path="${zip_path}.sha256"

rm -rf "$stage_dir" "$zip_path" "$checksum_path"
mkdir -p "$stage_dir"

make -C "$repo_root" clean PG_CONFIG="$pg_config"
make -C "$repo_root" PG_CONFIG="$pg_config"
make -C "$repo_root" install DESTDIR="$stage_dir" PG_CONFIG="$pg_config"

cat >"${stage_dir}/BUILD-INFO.txt" <<EOF
Package: ${package_name}
Version: ${version}
PostgreSQL major: ${pg_major}
PostgreSQL version: ${pg_version}
Git commit: $(git -C "$repo_root" rev-parse HEAD)
Built at: $(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

cp "${repo_root}/README.md" "${stage_dir}/README.md"

(
    cd "$dist_dir"
    zip -rq "$(basename "$zip_path")" "$(basename "$stage_dir")"
)

if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$zip_path" >"$checksum_path"
elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$zip_path" >"$checksum_path"
else
    echo 'no sha256 checksum tool available' >&2
    exit 1
fi

echo "$zip_path"
