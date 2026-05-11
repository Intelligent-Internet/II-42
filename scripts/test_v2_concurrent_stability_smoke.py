from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
MATRIX_SCRIPT = REPO_ROOT / 'scripts' / 'benchmark_v2_policy_matrix.py'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a concurrent v2 maintenance stability smoke test.'
    )
    parser.add_argument('--repeat-count', type=int, default=3)
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    with tempfile.TemporaryDirectory(
        prefix='psql_bm25s_v2_concurrent_smoke_'
    ) as td:
        tmpdir = Path(td)
        commands = [
            [
                sys.executable,
                str(MATRIX_SCRIPT),
                '--repeat-count',
                str(args.repeat_count),
                '--scenario',
                'heavy_delete_skew',
                '--output',
                str(tmpdir / 'heavy_delete.json'),
            ],
            [
                sys.executable,
                str(MATRIX_SCRIPT),
                '--repeat-count',
                str(args.repeat_count),
                '--scenario',
                'heavy_update_skew',
                '--output',
                str(tmpdir / 'heavy_update.json'),
            ],
        ]

        procs = [
            subprocess.Popen(cmd, cwd=REPO_ROOT)
            for cmd in commands
        ]
        failures: list[str] = []

        for proc, label in zip(procs, ['heavy_delete_skew', 'heavy_update_skew']):
            rc = proc.wait()
            if rc != 0:
                failures.append(f'{label}: exit {rc}')

        if failures:
            raise SystemExit(
                'concurrent stability smoke failed: ' + ', '.join(failures)
            )

    print('concurrent stability smoke passed')


if __name__ == '__main__':
    main()
