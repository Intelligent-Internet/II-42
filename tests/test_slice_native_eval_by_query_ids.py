import argparse
import json

from scripts.slice_native_eval_by_query_ids import run


def test_slice_baseline_rows_recomputes_summary(tmp_path) -> None:
    input_json = tmp_path / 'baselines.json'
    query_ids = tmp_path / 'query_ids.txt'
    output_json = tmp_path / 'out.json'
    query_ids.write_text('q2\n', encoding='utf-8')
    input_json.write_text(
        json.dumps({
            'rows': [
                {
                    'query_rows': [
                        {
                            'metrics': {'recall_at_100': 0.0},
                            'query_id': 'q1',
                        },
                        {
                            'metrics': {'recall_at_100': 1.0},
                            'query_id': 'q2',
                        },
                    ],
                    'source': 'bm25',
                    'summary': {'query_count': 2, 'recall_at_100': 0.5},
                },
            ],
        }),
        encoding='utf-8',
    )

    payload = run(argparse.Namespace(
        input_json=input_json,
        output_json=output_json,
        query_ids_file=query_ids,
    ))

    assert payload['rows'][0]['summary']['query_count'] == 1
    assert payload['rows'][0]['summary']['recall_at_100'] == 1.0


def test_slice_eval_query_rows_recomputes_summary(tmp_path) -> None:
    input_json = tmp_path / 'eval.json'
    query_ids = tmp_path / 'query_ids.txt'
    output_json = tmp_path / 'out.json'
    query_ids.write_text('q1\nq3\n', encoding='utf-8')
    input_json.write_text(
        json.dumps({
            'query_rows': [
                {
                    'metrics': {'mrr_at_20': 1.0, 'recall_at_100': 1.0},
                    'query_id': 'q1',
                },
                {
                    'metrics': {'mrr_at_20': 0.5, 'recall_at_100': 0.0},
                    'query_id': 'q2',
                },
                {
                    'metrics': {'mrr_at_20': 0.0, 'recall_at_100': 0.0},
                    'query_id': 'q3',
                },
            ],
            'summary': {'query_count': 3},
        }),
        encoding='utf-8',
    )

    payload = run(argparse.Namespace(
        input_json=input_json,
        output_json=output_json,
        query_ids_file=query_ids,
    ))

    assert payload['summary']['query_count'] == 2
    assert payload['summary']['mrr_at_20'] == 0.5
    assert payload['summary']['recall_at_100'] == 0.5
