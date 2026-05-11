#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def load_payload(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding='utf-8'))


def merge_entry(
    merged: dict[str, Any],
    source: dict[str, Any],
    dataset: str,
) -> None:
    entry = source['results'].get(dataset)
    if entry is None:
        return

    target = merged['results'].setdefault(
        dataset,
        {
            'dataset': dataset,
            'official_qps': entry.get('official_qps'),
        },
    )

    if 'official_qps' not in target or target['official_qps'] is None:
        target['official_qps'] = entry.get('official_qps')
    if 'stats' in entry and 'stats' not in target:
        target['stats'] = entry['stats']
    if 'wall_time_s' in entry:
        target['wall_time_s'] = entry['wall_time_s']

    for key in ('python_reference_bm25s', 'psql_bm25s_ids', 'psql_bm25s_text'):
        if key in entry:
            target[key] = entry[key]

    if 'error' in entry:
        target['error'] = entry['error']
    if 'traceback' in entry:
        target['traceback'] = entry['traceback']
    if 'retry_attempts' in entry:
        target['retry_attempts'] = entry['retry_attempts']


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Merge Python reference, ids, and text BEIR benchmark results.'
    )
    parser.add_argument('--ids', type=Path, required=True)
    parser.add_argument('--text', type=Path, required=True)
    parser.add_argument('--python-reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    ids = load_payload(args.ids)
    text = load_payload(args.text)
    python_reference = load_payload(args.python_reference)

    datasets = sorted(
        set(ids.get('results', {}))
        | set(text.get('results', {}))
        | set(python_reference.get('results', {}))
    )

    merged: dict[str, Any] = {
        'created_at': python_reference.get('created_at')
        or text.get('created_at')
        or ids.get('created_at'),
        'official_source': python_reference.get('official_source')
        or text.get('official_source')
        or ids.get('official_source'),
        'top_k': python_reference.get('top_k')
        or text.get('top_k')
        or ids.get('top_k'),
        'paths': ['ids', 'text', 'python_reference'],
        'results': {},
    }

    for dataset in datasets:
        merge_entry(merged, python_reference, dataset)
        merge_entry(merged, ids, dataset)
        merge_entry(merged, text, dataset)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(merged, indent=2, sort_keys=True),
        encoding='utf-8',
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
