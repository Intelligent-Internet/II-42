#!/usr/bin/env python3

from __future__ import annotations

import argparse
import gc
import json
import sys
import time
import traceback
from pathlib import Path
from typing import Any

from benchmark_beir_official import (
    DEFAULT_DATASETS_DIR,
    DEFAULT_RESULTS_DIR,
    OFFICIAL_ORDER,
    OFFICIAL_QPS,
    TOP_K,
    benchmark_python_reference,
    dataset_stats,
    load_dataset,
    load_existing_results,
    make_output_path,
    persist_results,
    result_is_success,
    tokenize_dataset,
)


def run_dataset(
    dataset: str,
    datasets_dir: Path,
    top_k: int,
) -> dict[str, Any]:
    corpus_ids, corpus_texts, query_texts = load_dataset(dataset, datasets_dir)
    corpus_tokenized, query_ids, _vocab_by_id = tokenize_dataset(
        corpus_texts,
        query_texts,
    )

    result = {
        'dataset': dataset,
        'official_qps': OFFICIAL_QPS[dataset],
        'stats': dataset_stats(corpus_tokenized, query_ids),
        'python_reference_bm25s': benchmark_python_reference(
            corpus_ids,
            corpus_tokenized,
            query_ids,
            top_k,
        ),
    }

    del corpus_ids
    del corpus_texts
    del query_texts
    del corpus_tokenized
    del query_ids
    gc.collect()

    return result


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Benchmark local Python reference implementation on official BEIR datasets.'
    )
    parser.add_argument(
        '--datasets',
        nargs='*',
        default=OFFICIAL_ORDER,
        choices=OFFICIAL_ORDER,
        help='Datasets to benchmark. Defaults to the full official list.',
    )
    parser.add_argument(
        '--datasets-dir',
        type=Path,
        default=DEFAULT_DATASETS_DIR,
        help='Directory used to cache BEIR dataset downloads.',
    )
    parser.add_argument(
        '--output',
        type=Path,
        default=None,
        help='JSON file for progressive benchmark results.',
    )
    parser.add_argument(
        '--top-k',
        type=int,
        default=TOP_K,
        help='Top-k used for retrieval benchmarking.',
    )
    parser.add_argument(
        '--resume',
        action='store_true',
        help='Skip datasets already present in the output file.',
    )
    parser.add_argument(
        '--stop-on-error',
        action='store_true',
        help='Stop immediately when a dataset benchmark fails.',
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    output_path = args.output or make_output_path(DEFAULT_RESULTS_DIR)
    payload = load_existing_results(output_path)
    payload['paths'] = ['python_reference']
    payload['top_k'] = args.top_k

    for dataset in args.datasets:
        if args.resume and dataset in payload['results']:
            if result_is_success(payload['results'][dataset]):
                print(
                    f'Skipping completed dataset: {dataset}',
                    file=sys.stderr,
                )
                continue
            print(
                f'Rerunning failed dataset: {dataset}',
                file=sys.stderr,
            )

        print(f'=== {dataset} ===', file=sys.stderr)
        started = time.perf_counter()
        try:
            payload['results'][dataset] = run_dataset(
                dataset,
                args.datasets_dir,
                args.top_k,
            )
            payload['results'][dataset]['wall_time_s'] = (
                time.perf_counter() - started
            )
        except Exception as exc:  # pragma: no cover - benchmark failure path
            payload['results'][dataset] = {
                'dataset': dataset,
                'official_qps': OFFICIAL_QPS[dataset],
                'error': str(exc),
                'traceback': traceback.format_exc(),
                'wall_time_s': time.perf_counter() - started,
            }
            persist_results(output_path, payload)
            print(
                f'Benchmark failed for {dataset}: {exc}',
                file=sys.stderr,
            )
            if args.stop_on_error:
                raise

        persist_results(output_path, payload)
        print(
            json.dumps(payload['results'][dataset], indent=2, sort_keys=True),
            file=sys.stderr,
        )

    print(json.dumps(payload, indent=2, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
