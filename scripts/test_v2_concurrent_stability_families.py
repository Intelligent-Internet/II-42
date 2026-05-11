from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
MATRIX_STRESS_SCRIPT = (
    REPO_ROOT / 'scripts' / 'test_v2_concurrent_stability_stress.py'
)
LONGRUN_SCRIPT = REPO_ROOT / 'scripts' / 'benchmark_v2_longrun_profiles.py'


@dataclass(frozen=True)
class FamilyConfig:
    name: str
    kind: str
    rounds: int
    repeat_count: int
    pairs: tuple[tuple[str, str], ...] = ()
    longrun_workers: int = 0


FAMILIES = [
    FamilyConfig(
        name='matrix_default_soak',
        kind='matrix',
        rounds=6,
        repeat_count=4,
    ),
    FamilyConfig(
        name='matrix_cross_skew',
        kind='matrix',
        rounds=3,
        repeat_count=3,
        pairs=(
            ('heavy_delete_skew', 'heavy_insert_skew'),
            ('heavy_update_skew', 'heavy_mixed'),
        ),
    ),
    FamilyConfig(
        name='longrun_pair',
        kind='longrun',
        rounds=1,
        repeat_count=2,
        longrun_workers=2,
    ),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run broader concurrent v2 stability families.'
    )
    parser.add_argument(
        '--family',
        action='append',
        default=[],
        help='Optional family name filter.',
    )
    return parser.parse_args()


def selected_families(names: list[str]) -> list[FamilyConfig]:
    if not names:
        return FAMILIES

    selected: list[FamilyConfig] = []
    wanted = set(names)
    for family in FAMILIES:
        if family.name in wanted:
            selected.append(family)

    if len(selected) != len(wanted):
        known = ', '.join(family.name for family in FAMILIES)
        missing = sorted(wanted - {family.name for family in selected})
        raise ValueError(
            f'unknown family(s): {", ".join(missing)}; known: {known}'
        )

    return selected


def matrix_command(family: FamilyConfig) -> list[str]:
    cmd = [
        sys.executable,
        str(MATRIX_STRESS_SCRIPT),
        '--rounds',
        str(family.rounds),
        '--repeat-count',
        str(family.repeat_count),
    ]

    for left, right in family.pairs:
        cmd.extend(['--pair', f'{left},{right}'])

    return cmd


def longrun_commands(tmpdir: Path, family: FamilyConfig) -> list[list[str]]:
    commands: list[list[str]] = []
    for worker_no in range(1, family.longrun_workers + 1):
        run_id = f'w{worker_no}_p{os.getpid()}'
        commands.append(
            [
                sys.executable,
                str(LONGRUN_SCRIPT),
                '--repeat-count',
                str(family.repeat_count),
                '--base-seed',
                str(20260324 + (worker_no * 1000)),
                '--run-id',
                run_id,
                '--output',
                str(tmpdir / f'{family.name}_w{worker_no}.json'),
            ]
        )
    return commands


def run_commands(commands: list[list[str]], label: str) -> None:
    procs = [
        subprocess.Popen(cmd, cwd=REPO_ROOT)
        for cmd in commands
    ]

    failures: list[str] = []
    for idx, proc in enumerate(procs, start=1):
        rc = proc.wait()
        if rc != 0:
            failures.append(f'{label}[{idx}] exit={rc}')

    if failures:
        raise RuntimeError('; '.join(failures))


def run_family(family: FamilyConfig) -> None:
    if family.kind == 'matrix':
        run_commands([matrix_command(family)], family.name)
        return

    if family.kind == 'longrun':
        with tempfile.TemporaryDirectory(
            prefix=f'psql_bm25s_{family.name}_'
        ) as td:
            run_commands(longrun_commands(Path(td), family), family.name)
        return

    raise ValueError(f'unsupported family kind: {family.kind}')


def main() -> None:
    args = parse_args()
    families = selected_families(args.family)

    with tempfile.TemporaryDirectory(
        prefix='psql_bm25s_v2_concurrent_families_'
    ):
        for family in families:
            run_family(family)

    names = ', '.join(family.name for family in families)
    print(f'concurrent stability families passed ({names})')


if __name__ == '__main__':
    main()
