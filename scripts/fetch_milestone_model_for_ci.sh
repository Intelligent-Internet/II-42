#!/usr/bin/env bash

set -euo pipefail

output="${1:-}"
if [ -z "$output" ]; then
    echo 'usage: fetch_milestone_model_for_ci.sh OUTPUT' >&2
    exit 1
fi
if [ -z "${II42_MILESTONE_MODEL_URL:-}" ]; then
    echo 'II42_MILESTONE_MODEL_URL repository variable is required' >&2
    exit 1
fi

args=(
    --url "$II42_MILESTONE_MODEL_URL"
    --output "$output"
    --overwrite
)
if [ -n "${II42_MILESTONE_MODEL_ARCHIVE_SHA256:-}" ]; then
    args+=(
        --archive-sha256 "$II42_MILESTONE_MODEL_ARCHIVE_SHA256"
    )
fi
python3 scripts/fetch_milestone_model.py "${args[@]}"

if [ -n "${GITHUB_ENV:-}" ]; then
    echo "II42_MILESTONE_MODEL_CHECKOUT=${output}" >>"$GITHUB_ENV"
fi
