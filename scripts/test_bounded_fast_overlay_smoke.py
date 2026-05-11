from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCHEMA = 'bm25_fast_overlay_smoke'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run a bounded fast-overlay concurrency smoke test.'
    )
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--dbname', default='contrib_regression')
    parser.add_argument('--hold-seconds', type=int, default=5)
    parser.add_argument('--statement-timeout-ms', type=int, default=2000)
    return parser.parse_args()


def psql_cmd(args: argparse.Namespace) -> list[str]:
    return [
        args.psql,
        '-X',
        '-d',
        args.dbname,
        '-v',
        'ON_ERROR_STOP=1',
    ]


def run_psql(args: argparse.Namespace, sql: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        psql_cmd(args),
        input=sql,
        text=True,
        cwd=REPO_ROOT,
        check=True,
        capture_output=True,
    )


def setup_schema(args: argparse.Namespace) -> None:
    run_psql(
        args,
        f'''
        DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;
        CREATE SCHEMA {SCHEMA};
        CREATE EXTENSION IF NOT EXISTS psql_bm25s;

        CREATE TABLE {SCHEMA}.docs (
            id int primary key,
            marker int not null,
            title_tokens text[] not null
        );

        INSERT INTO {SCHEMA}.docs
        SELECT
            g,
            g,
            CASE
                WHEN g % 5 = 0 THEN ARRAY['cancer','therapy']
                ELSE ARRAY['bird','study']
            END
        FROM generate_series(1, 2000) g;

        CREATE INDEX docs_marker_idx ON {SCHEMA}.docs(marker);
        CREATE INDEX docs_title_bm25_idx
            ON {SCHEMA}.docs USING psql_bm25s (title_tokens)
            WITH (
                method = 'lucene',
                idf_method = 'lucene',
                k1 = 1.5,
                b = 0.75,
                delta = 0.5,
                consistency = 'eventual',
                auto_rebuild_threshold = 10000
            );
        ''',
    )


def drop_schema(args: argparse.Namespace) -> None:
    run_psql(args, f'DROP SCHEMA IF EXISTS {SCHEMA} CASCADE;')


def start_writer(args: argparse.Namespace) -> subprocess.Popen[str]:
    writer = subprocess.Popen(
        psql_cmd(args),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=REPO_ROOT,
    )
    assert writer.stdin is not None
    writer.stdin.write(
        f'''
        BEGIN;
        UPDATE {SCHEMA}.docs
        SET marker = marker + 100000
        WHERE id <= 1000;
        SELECT pg_sleep({args.hold_seconds});
        ROLLBACK;
        '''
    )
    writer.stdin.close()
    return writer


def run_concurrent_query(args: argparse.Namespace) -> float:
    start = time.monotonic()
    run_psql(
        args,
        f'''
        SET statement_timeout = '{args.statement_timeout_ms}ms';
        SELECT count(*)
        FROM public.psql_bm25s_query_tokens(
            '{SCHEMA}.docs_title_bm25_idx'::regclass,
            ARRAY['cancer'],
            10,
            NULL
        ) h;
        ''',
    )
    return time.monotonic() - start


def wait_writer(writer: subprocess.Popen[str]) -> None:
    writer.wait(timeout=30)
    stdout = writer.stdout.read() if writer.stdout is not None else ''
    stderr = writer.stderr.read() if writer.stderr is not None else ''
    if writer.returncode != 0:
        sys.stderr.write(stdout)
        sys.stderr.write(stderr)
        raise RuntimeError(f'writer failed with exit code {writer.returncode}')


def main() -> None:
    args = parse_args()
    setup_schema(args)

    writer: subprocess.Popen[str] | None = None
    try:
        writer = start_writer(args)
        time.sleep(1)
        elapsed = run_concurrent_query(args)
        wait_writer(writer)
    finally:
        if writer is not None and writer.poll() is None:
            writer.kill()
            writer.wait()
        drop_schema(args)

    print(f'bounded fast-overlay smoke passed ({elapsed:.3f}s)')


if __name__ == '__main__':
    main()
