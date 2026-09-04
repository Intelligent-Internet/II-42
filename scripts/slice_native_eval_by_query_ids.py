#!/usr/bin/env python3
"""Slice native per-query eval JSON to a fixed query-id set."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Slice native baseline/eval JSON by query ids.',
    )
    parser.add_argument('--input-json', type=Path, required=True)
    parser.add_argument('--query-ids-file', type=Path, required=True)
    parser.add_argument('--output-json', type=Path, required=True)
    return parser.parse_args()


def read_query_ids(path: Path) -> set[str]:
    query_ids = set()
    for line in path.read_text(encoding='utf-8').splitlines():
        stripped = line.strip()
        if stripped and not stripped.startswith('#'):
            query_ids.add(stripped)
    return query_ids


def mean(values: list[float]) -> float | None:
    return sum(values) / len(values) if values else None


def query_set_identity(query_rows: list[dict[str, Any]]) -> dict[str, Any]:
    query_ids = sorted(str(row.get('query_id')) for row in query_rows)
    digest = hashlib.sha256()
    for query_id in query_ids:
        digest.update(query_id.encode('utf-8'))
        digest.update(b'\0')
    return {
        'query_count': len(query_ids),
        'query_ids_sha256': digest.hexdigest(),
    }


def recompute_summary(query_rows: list[dict[str, Any]]) -> dict[str, Any]:
    metric_keys = sorted({
        key
        for row in query_rows
        for key, value in row.get('metrics', {}).items()
        if isinstance(value, int | float)
    })
    summary = {
        key: mean([
            float(row['metrics'][key])
            for row in query_rows
            if isinstance(row.get('metrics', {}).get(key), int | float)
        ])
        for key in metric_keys
    }
    identity = query_set_identity(query_rows)
    summary['query_count'] = identity['query_count']
    summary['query_ids_sha256'] = identity['query_ids_sha256']
    return summary


def filter_query_rows(
    query_rows: list[dict[str, Any]],
    *,
    query_ids: set[str],
) -> list[dict[str, Any]]:
    return [
        row
        for row in query_rows
        if str(row.get('query_id')) in query_ids
    ]


def slice_source_row(
    row: dict[str, Any],
    *,
    query_ids: set[str],
) -> dict[str, Any]:
    query_rows = filter_query_rows(
        list(row.get('query_rows') or []),
        query_ids=query_ids,
    )
    sliced = dict(row)
    sliced['query_rows'] = query_rows
    sliced['summary'] = recompute_summary(query_rows)
    return sliced


def slice_payload(payload: dict[str, Any], *, query_ids: set[str]) -> dict[str, Any]:
    sliced = dict(payload)
    if isinstance(payload.get('rows'), list):
        sliced['rows'] = [
            slice_source_row(row, query_ids=query_ids)
            for row in payload['rows']
        ]
    elif isinstance(payload.get('query_rows'), list):
        query_rows = filter_query_rows(
            list(payload['query_rows']),
            query_ids=query_ids,
        )
        sliced['query_rows'] = query_rows
        sliced['summary'] = recompute_summary(query_rows)
        sliced['query_set'] = query_set_identity(query_rows)
    else:
        raise ValueError('input JSON has neither rows nor query_rows')
    sliced['query_ids_file'] = None
    return sliced


def run(args: argparse.Namespace) -> dict[str, Any]:
    payload = json.loads(args.input_json.read_text(encoding='utf-8'))
    query_ids = read_query_ids(args.query_ids_file)
    sliced = slice_payload(payload, query_ids=query_ids)
    sliced['query_ids_file'] = str(args.query_ids_file)
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(
        json.dumps(sliced, indent=2, sort_keys=True) + '\n',
        encoding='utf-8',
    )
    return sliced


def main() -> int:
    args = parse_args()
    payload = run(args)
    if isinstance(payload.get('rows'), list):
        result = {
            str(row.get('source')): row.get('summary')
            for row in payload['rows']
        }
    else:
        result = payload.get('summary', {})
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
