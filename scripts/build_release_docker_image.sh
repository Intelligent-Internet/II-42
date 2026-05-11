#!/usr/bin/env bash

set -euo pipefail

version=''
image_tag=''

while [ "$#" -gt 0 ]; do
    case "$1" in
        --version)
            version="$2"
            shift 2
            ;;
        --image-tag)
            image_tag="$2"
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

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
arch="$(uname -m)"
os_name="$(uname -s | tr '[:upper:]' '[:lower:]')"

if [ -z "$image_tag" ]; then
    image_tag="psql_bm25s:pg18-v${version}"
fi

package_name="psql_bm25s-v${version}-docker-pg18-${os_name}-${arch}"
dist_dir="${repo_root}/dist"
archive_path="${dist_dir}/${package_name}.tar.gz"
checksum_path="${archive_path}.sha256"

rm -rf "$archive_path" "$checksum_path"
mkdir -p "$dist_dir"

docker build \
    --pull \
    --build-arg "PSQL_BM25S_VERSION=${version}" \
    --file "${repo_root}/packaging/docker/postgres18/Dockerfile" \
    --tag "${image_tag}" \
    "${repo_root}"

docker save "${image_tag}" | gzip -c >"${archive_path}"

if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$archive_path" >"$checksum_path"
elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$archive_path" >"$checksum_path"
else
    echo 'no sha256 checksum tool available' >&2
    exit 1
fi

echo "$archive_path"
