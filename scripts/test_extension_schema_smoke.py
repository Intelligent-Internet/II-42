from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DB_NAME_RE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')
VERSION_RE = re.compile(r"^default_version = '([^']+)'$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Run non-public extension schema wrapper smoke tests.'
    )
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--admin-db', default='postgres')
    parser.add_argument('--fresh-db', default='psql_bm25s_schema_fresh_smoke')
    parser.add_argument(
        '--relocate-db',
        default='psql_bm25s_schema_relocate_smoke',
    )
    parser.add_argument(
        '--upgrade-db',
        default='psql_bm25s_schema_upgrade_smoke',
    )
    return parser.parse_args()


def psql_cmd(args: argparse.Namespace, dbname: str) -> list[str]:
    return [
        args.psql,
        '-X',
        '-d',
        dbname,
        '-v',
        'ON_ERROR_STOP=1',
    ]


def run_psql(args: argparse.Namespace, dbname: str, sql: str) -> str:
    result = subprocess.run(
        psql_cmd(args, dbname),
        input=sql,
        text=True,
        cwd=REPO_ROOT,
        check=True,
        capture_output=True,
    )
    return result.stdout.strip()


def read_extension_version() -> str:
    for line in (REPO_ROOT / 'psql_bm25s.control').read_text().splitlines():
        match = VERSION_RE.match(line)
        if match is not None:
            return match.group(1)
    raise RuntimeError('could not read default_version from psql_bm25s.control')


def validate_database_name(dbname: str) -> None:
    if DB_NAME_RE.fullmatch(dbname) is None:
        raise ValueError(f'unsafe database name for smoke test: {dbname}')


def reset_database(args: argparse.Namespace, dbname: str) -> None:
    validate_database_name(dbname)
    run_psql(
        args,
        args.admin_db,
        f'''
        SELECT pg_terminate_backend(pid)
        FROM pg_stat_activity
        WHERE datname = '{dbname}'
          AND pid <> pg_backend_pid();
        DROP DATABASE IF EXISTS {dbname};
        CREATE DATABASE {dbname} TEMPLATE template0;
        ''',
    )


def drop_database(args: argparse.Namespace, dbname: str) -> None:
    validate_database_name(dbname)
    run_psql(
        args,
        args.admin_db,
        f'''
        SELECT pg_terminate_backend(pid)
        FROM pg_stat_activity
        WHERE datname = '{dbname}'
          AND pid <> pg_backend_pid();
        DROP DATABASE IF EXISTS {dbname};
        ''',
    )


def wrapper_smoke_sql(version_sql: str, upgrade_sql: str = '') -> str:
    return f'''
    CREATE SCHEMA ext;
    CREATE EXTENSION psql_bm25s WITH SCHEMA ext {version_sql};
    {upgrade_sql}

    DO $$
    DECLARE
        removed_function_count integer;
        removed_type_count integer;
    BEGIN
        SELECT count(*)
        INTO removed_function_count
        FROM pg_proc p
        JOIN pg_namespace n
            ON n.oid = p.pronamespace
        WHERE n.nspname = 'ext'
          AND p.proname IN (
            'psql_bm25s_build_ids',
            'psql_bm25s_build_tokens',
            'psql_bm25s_describe',
            'psql_bm25s_from_bytea',
            'psql_bm25s_field_queries',
            'psql_bm25s_field_query',
            'psql_bm25s_fuse',
            'psql_bm25s_fuse_results',
            'psql_bm25s_hybrid_check_direction',
            'psql_bm25s_hybrid_check_fusion',
            'psql_bm25s_hybrid_check_normalizer',
            'psql_bm25s_hybrid_fuse_candidates_sql_reference',
            'psql_bm25s_index_describe',
            'psql_bm25s_index_maintenance_policy',
            'psql_bm25s_index_maintenance_policy_details',
            'psql_bm25s_index_maintenance_policy_recommend',
            'psql_bm25s_index_maintenance_state',
            'psql_bm25s_index_value',
            'psql_bm25s_match_query_tokens',
            'psql_bm25s_num_docs',
            'psql_bm25s_recommend_maintenance_policy',
            'psql_bm25s_hits',
            'psql_bm25s_query_hits',
            'psql_bm25s_search',
            'psql_bm25s_search_field_queries',
            'psql_bm25s_search_ids',
            'psql_bm25s_search_ids_result',
            'psql_bm25s_search_indexes',
            'psql_bm25s_search_prepared_query',
            'psql_bm25s_search_prepared_query_result',
            'psql_bm25s_search_query',
            'psql_bm25s_search_query_result',
            'psql_bm25s_search_query_result_cfg',
            'psql_bm25s_search_result',
            'psql_bm25s_search_tokens',
            'psql_bm25s_search_tokens_result',
            'psql_bm25s_search_weighted_queries',
            'psql_bm25s_score_query_tokens',
            'psql_bm25s_to_bytea',
            'psql_bm25s_topk_ids',
            'psql_bm25s_topk_tokens',
            'psql_bm25s_vocab_size',
            'psql_bm25s_weighted_queries',
            'psql_bm25s_weighted_query'
          );

        IF removed_function_count <> 0 THEN
            RAISE EXCEPTION
                'removed API functions are still visible: %',
                removed_function_count;
        END IF;

        SELECT count(*)
        INTO removed_type_count
        FROM pg_type t
        JOIN pg_namespace n
            ON n.oid = t.typnamespace
        WHERE n.nspname = 'ext'
          AND t.typname IN (
            'psql_bm25s_field_query',
            'psql_bm25s_result',
            'psql_bm25s_result_field_query',
            'psql_bm25s_result_weighted_query',
            'psql_bm25s_weighted_query'
          );

        IF removed_type_count <> 0 THEN
            RAISE EXCEPTION 'removed result type is still visible';
        END IF;

        SELECT count(*)
        INTO removed_function_count
        FROM pg_proc p
        JOIN pg_namespace n
            ON n.oid = p.pronamespace
        WHERE n.nspname = 'ext'
          AND p.proname = 'psql_bm25s_query'
          AND pg_get_function_identity_arguments(p.oid)
              = 'regclass, text, boolean, text[], boolean, boolean';

        IF removed_function_count <> 0 THEN
            RAISE EXCEPTION 'old prepared-query builder is still visible';
        END IF;
    END;
    $$;

    CREATE TABLE public.docs (
        id serial PRIMARY KEY,
        body text,
        title varchar,
        tokens text[]
    );

    INSERT INTO public.docs (body, title, tokens) VALUES
        ('alpha beta gamma', 'alpha title', ARRAY['alpha', 'beta']),
        ('beta gamma delta', 'beta title', ARRAY['beta', 'gamma']),
        ('theta lambda', 'theta title', ARRAY['theta']);

    SET search_path = ext, public;
    CREATE INDEX docs_body_idx ON public.docs USING psql_bm25s (body);
    CREATE INDEX docs_title_idx ON public.docs USING psql_bm25s (title);
    CREATE INDEX docs_tokens_idx ON public.docs USING psql_bm25s (tokens);

    SET search_path = pg_catalog;

    SELECT count(*)
    FROM ext.psql_bm25s_ranked_query(
        'public.docs_body_idx'::regclass,
        'alpha',
        2
    ) q;

    SELECT count(*)
    FROM ext.psql_bm25s_fusion_query_weighted(
        ARRAY[
            ext.psql_bm25s_fusion_weighted_query(
                'public.docs_body_idx'::regclass,
                'alpha',
                2.0
            ),
            ext.psql_bm25s_fusion_weighted_query(
                'public.docs_title_idx'::regclass,
                'alpha',
                1.0
            )
        ]::ext.psql_bm25s_result_fusion_weighted_query[],
        2,
        10,
        NULL
    ) r;

    SELECT count(*)
    FROM ext.psql_bm25s_query(
        'public.docs_body_idx'::regclass,
        'alpha',
        2
    ) r;

    SELECT count(*)
    FROM ext.psql_bm25s_hybrid_fuse_candidates(
        ARRAY[
            ext.psql_bm25s_hybrid_bm25_candidate(
                'body',
                '(0,1)'::tid,
                1.0,
                1,
                2.0
            ),
            ext.psql_bm25s_hybrid_vector_candidate(
                'embedding',
                '(0,1)'::tid,
                0.2,
                1,
                1.0,
                'minmax'
            ),
            ext.psql_bm25s_hybrid_vector_candidate(
                'embedding',
                '(0,2)'::tid,
                0.4,
                2,
                1.0,
                'minmax'
            )
        ]::ext.psql_bm25s_result_hybrid_candidate[],
        2,
        'score'
    ) h;

    SELECT count(*)
    FROM public.docs
    WHERE body OPERATOR(ext.@@) 'alpha';

    SET search_path = public, pg_catalog;

    DO $$
    DECLARE
        explain_result jsonb;
    BEGIN
        explain_result := ext.psql_bm25s_fast_path_explain(
            'public.docs_body_idx'::regclass,
            $q$SELECT *
              FROM docs
              WHERE body OPERATOR(ext.@@) 'alpha'$q$
        );
        IF NOT explain_result ? 'used_psql_bm25s_index' THEN
            RAISE EXCEPTION
                'psql_bm25s_fast_path_explain returned unexpected JSON: %',
                explain_result;
        END IF;
    END;
    $$;
    '''


def run_case(
    args: argparse.Namespace,
    dbname: str,
    version_sql: str,
    upgrade_sql: str = '',
) -> None:
    reset_database(args, dbname)
    try:
        run_psql(args, dbname, wrapper_smoke_sql(version_sql, upgrade_sql))
    finally:
        drop_database(args, dbname)


def run_relocation_guard(args: argparse.Namespace) -> None:
    reset_database(args, args.relocate_db)
    try:
        run_psql(
            args,
            args.relocate_db,
            '''
            CREATE SCHEMA ext;
            CREATE SCHEMA ext2;
            CREATE EXTENSION psql_bm25s WITH SCHEMA ext;
            ''',
        )
        result = subprocess.run(
            psql_cmd(args, args.relocate_db),
            input='ALTER EXTENSION psql_bm25s SET SCHEMA ext2;',
            text=True,
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
        )
        if result.returncode == 0:
            raise AssertionError(
                'psql_bm25s unexpectedly allowed ALTER EXTENSION SET SCHEMA'
            )
        expected = 'extension "psql_bm25s" does not support SET SCHEMA'
        if expected not in result.stderr:
            raise AssertionError(
                'unexpected relocation error: '
                f'{result.stderr.strip()}'
            )
    finally:
        drop_database(args, args.relocate_db)


def main() -> None:
    args = parse_args()
    current_version = read_extension_version()
    run_case(args, args.fresh_db, '')
    run_case(
        args,
        args.upgrade_db,
        "VERSION '0.2.0'",
        f"ALTER EXTENSION psql_bm25s UPDATE TO '{current_version}';",
    )
    run_relocation_guard(args)
    print('extension schema smoke passed')


if __name__ == '__main__':
    main()
