#!/usr/bin/env bash

set -euo pipefail

source_tag=''
target_image=''
target_tags=()

while [ "$#" -gt 0 ]; do
    case "$1" in
        --source-tag)
            source_tag="$2"
            shift 2
            ;;
        --target-image)
            target_image="$2"
            shift 2
            ;;
        --tag)
            target_tags+=("$2")
            shift 2
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

if [ -z "${source_tag}" ]; then
    echo '--source-tag is required' >&2
    exit 1
fi

if [ -z "${target_image}" ]; then
    echo '--target-image is required' >&2
    exit 1
fi

if [ "${#target_tags[@]}" -eq 0 ]; then
    echo 'at least one --tag is required' >&2
    exit 1
fi

for target_tag in "${target_tags[@]}"; do
    full_target="${target_image}:${target_tag}"
    docker tag "${source_tag}" "${full_target}"
    docker push "${full_target}"
done
