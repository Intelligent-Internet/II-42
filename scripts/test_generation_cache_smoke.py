from __future__ import annotations

import argparse
import os
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCHEMA = 'bm25_generation_cache_smoke'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run generation cache lifecycle smoke checks.'
    )
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--dbname', default='contrib_regression')
    return parser.parse_args()


def psql_cmd(args: argparse.Namespace, raw: bool = False) -> list[str]:
    cmd = [
        args.psql,
        '-X',
        '-d',
        args.dbname,
        '-v',
        'ON_ERROR_STOP=1',
    ]
    if raw:
        cmd.extend(['-Atq'])
    return cmd


def run_psql(
    args: argparse.Namespace,
    sql: str,
    raw: bool = False,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        psql_cmd(args, raw=raw),
        input=sql,
        text=True,
        cwd=REPO_ROOT,
        check=check,
        capture_output=True,
    )


def scalar_sql(args: argparse.Namespace, sql: str) -> str:
    return run_psql(args, sql, raw=True).stdout.strip()


def setup_schema(args: argparse.Namespace) -> None:
    run_psql(
        args,
        f'''
        DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;
        CREATE SCHEMA {SCHEMA};
        CREATE EXTENSION IF NOT EXISTS psql_bm25s;

        CREATE TABLE {SCHEMA}.docs (
            id int primary key,
            tokens text[] not null
        );

        INSERT INTO {SCHEMA}.docs VALUES
            (1, ARRAY['generation','base']),
            (2, ARRAY['generation','stable']);

        CREATE INDEX docs_bm25_idx
            ON {SCHEMA}.docs USING psql_bm25s (tokens)
            WITH (consistency = 'eventual');
        ''',
    )


def drop_schema(args: argparse.Namespace) -> None:
    run_psql(args, f'DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;', check=False)


def data_directory(args: argparse.Namespace) -> Path:
    return Path(scalar_sql(args, 'SHOW data_directory;'))


def generation_cache_dir(args: argparse.Namespace) -> Path:
    cache_dir = data_directory(args) / 'psql_bm25s_generation_cache'
    cache_dir.mkdir(mode=0o700, exist_ok=True)
    return cache_dir


def seed_broken_descriptor_files(args: argparse.Namespace) -> list[Path]:
    cache_dir = generation_cache_dir(args)
    pid = os.getpid()
    files = [
        cache_dir / f'broken_{pid}.desc',
        cache_dir / f'broken_{pid}.desc.{pid}.tmp',
        cache_dir / f'broken_{pid}.lock',
        cache_dir / f'broken_{pid}.fail',
    ]
    files[0].write_bytes(b'not a valid descriptor')
    files[1].write_bytes(b'partial descriptor')
    files[2].write_bytes(b'')
    files[3].write_bytes(b'')
    return files


def assert_cache_clear_removes_broken_files(args: argparse.Namespace) -> None:
    files = seed_broken_descriptor_files(args)
    cleared = int(scalar_sql(
        args,
        'SELECT public.psql_bm25s_generation_cache_clear();',
    ))
    if cleared < len(files):
        raise AssertionError(
            'generation cache clear did not report removed broken files: '
            f'cleared={cleared}, expected_at_least={len(files)}'
        )
    remaining = [path for path in files if path.exists()]
    if remaining:
        raise AssertionError(
            'generation cache clear left broken descriptor files: '
            f'{remaining}'
        )


def generation_state(args: argparse.Namespace) -> str:
    return scalar_sql(
        args,
        (
            'SELECT public.psql_bm25s_generation_cache_state('
            f"'{SCHEMA}.docs_bm25_idx'::regclass);"
        ),
    )


def hit_ids(args: argparse.Namespace, term: str) -> list[int]:
    output = scalar_sql(
        args,
        f'''
        SELECT id
        FROM public.psql_bm25s_query(
            '{SCHEMA}.docs_bm25_idx'::regclass,
            '{term}',
            10,
            NULL,
            true,
            NULL,
            false,
            false
        ) h
        JOIN {SCHEMA}.docs d ON d.ctid = h.ctid
        WHERE h.score > 0
        ORDER BY id;
        ''',
    )
    if not output:
        return []
    return [int(line) for line in output.splitlines()]


def assert_generation_key_changes_across_backends(args: argparse.Namespace) -> None:
    before = generation_state(args)
    base_hits = hit_ids(args, 'generation')
    if base_hits != [1, 2]:
        raise AssertionError(f'unexpected base hits: {base_hits}')

    run_psql(
        args,
        f'''
        INSERT INTO {SCHEMA}.docs VALUES
            (3, ARRAY['generation','new']);
        ''',
    )
    run_psql(
        args,
        (
            'SELECT public.psql_bm25s_index_try_maintain('
            f"'{SCHEMA}.docs_bm25_idx'::regclass);"
        ),
        raw=True,
    )

    after = generation_state(args)
    if before == after:
        raise AssertionError(
            'generation key did not change after cross-backend maintenance: '
            f'{before}'
        )
    new_hits = hit_ids(args, 'generation')
    if new_hits != [1, 2, 3]:
        raise AssertionError(f'new generation is not searchable: {new_hits}')


def main() -> None:
    args = parse_args()
    setup_schema(args)

    try:
        assert_cache_clear_removes_broken_files(args)
        assert_generation_key_changes_across_backends(args)
        print('generation cache smoke passed')
    finally:
        drop_schema(args)


if __name__ == '__main__':
    main()
