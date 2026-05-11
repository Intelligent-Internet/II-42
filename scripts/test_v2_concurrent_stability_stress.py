from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
MATRIX_SCRIPT = REPO_ROOT / 'scripts' / 'benchmark_v2_policy_matrix.py'

DEFAULT_PAIRS = [
    ('heavy_delete_skew', 'heavy_update_skew'),
    ('heavy_mixed', 'heavy_insert_skew'),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a broader concurrent v2 maintenance stress test.'
    )
    parser.add_argument('--rounds', type=int, default=2)
    parser.add_argument('--repeat-count', type=int, default=3)
    parser.add_argument(
        '--pair',
        action='append',
        default=[],
        help='Optional scenario pair formatted as left,right',
    )
    return parser.parse_args()


def parse_pairs(raw_pairs: list[str]) -> list[tuple[str, str]]:
    if not raw_pairs:
        return DEFAULT_PAIRS

    pairs: list[tuple[str, str]] = []
    for raw_pair in raw_pairs:
        left, sep, right = raw_pair.partition(',')
        left = left.strip()
        right = right.strip()
        if sep != ',' or not left or not right:
            raise ValueError(
                'pair must be formatted as left,right'
            )
        pairs.append((left, right))
    return pairs


def build_command(
    tmpdir: Path,
    scenario: str,
    repeat_count: int,
    round_no: int,
) -> list[str]:
    run_id = f'p{os.getpid()}_{scenario}_r{round_no}'
    return [
        sys.executable,
        str(MATRIX_SCRIPT),
        '--repeat-count',
        str(repeat_count),
        '--scenario',
        scenario,
        '--run-id',
        run_id,
        '--base-seed',
        str(20260324 + (round_no * 1000)),
        '--output',
        str(tmpdir / f'{scenario}_round{round_no}.json'),
    ]


def run_pair(
    tmpdir: Path,
    left: str,
    right: str,
    repeat_count: int,
    round_no: int,
) -> list[str]:
    commands = [
        build_command(tmpdir, left, repeat_count, round_no),
        build_command(tmpdir, right, repeat_count, round_no),
    ]
    procs = [
        subprocess.Popen(cmd, cwd=REPO_ROOT)
        for cmd in commands
    ]
    failures: list[str] = []

    for proc, label in zip(procs, [left, right]):
        rc = proc.wait()
        if rc != 0:
            failures.append(
                f'round={round_no} scenario={label} exit={rc}'
            )

    return failures


def main() -> None:
    args = parse_args()
    pairs = parse_pairs(args.pair)
    failures: list[str] = []

    with tempfile.TemporaryDirectory(
        prefix='psql_bm25s_v2_concurrent_stress_'
    ) as td:
        tmpdir = Path(td)
        for round_no in range(1, args.rounds + 1):
            for left, right in pairs:
                failures.extend(
                    run_pair(
                        tmpdir,
                        left,
                        right,
                        args.repeat_count,
                        round_no,
                    )
                )

    if failures:
        raise SystemExit(
            'concurrent stability stress failed: ' + '; '.join(failures)
        )

    print(
        'concurrent stability stress passed '
        f'(rounds={args.rounds}, repeat_count={args.repeat_count})'
    )


if __name__ == '__main__':
    main()
