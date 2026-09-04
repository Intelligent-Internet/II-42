from __future__ import annotations

import importlib.util
from pathlib import Path
from types import SimpleNamespace

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'scripts' / 'qualify_shared_preload_recovery.py'
SPEC = importlib.util.spec_from_file_location(
    'qualify_shared_preload_recovery',
    SCRIPT,
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def arguments(**overrides: object) -> SimpleNamespace:
    values = {
        'allow_cache_clear': True,
        'expected_port': 56544,
        'k': 50,
        'clients': 8,
        'warm_repetitions': 4,
        'readmission_timeout_seconds': 120.0,
        'first_max_ms': 2_000.0,
        'warm_p50_max_ms': 500.0,
        'warm_p95_max_ms': 1_000.0,
        'rss_slack_mib': 64,
    }
    values.update(overrides)
    return SimpleNamespace(**values)


def test_validation_requires_explicit_cache_clear_acknowledgement() -> None:
    with pytest.raises(ValueError, match='allow-cache-clear'):
        MODULE.validate_args(arguments(allow_cache_clear=False))


def test_runtime_ready_accepts_worker_owned_page_metadata() -> None:
    page_ready = {
        'generation': {'auto_preload_priority': 100},
        'shared_preload': {
            'available': True,
            'query_warm_marker_valid': True,
            'query_metadata_warm': True,
            'loading': False,
        },
    }

    assert MODULE.runtime_ready(page_ready) is True
    page_ready['shared_preload']['query_metadata_warm'] = False
    assert MODULE.runtime_ready(page_ready) is False


def test_runtime_ready_accepts_current_resident_fold() -> None:
    resident_ready = {
        'generation': {'auto_preload_priority': 100},
        'shared_preload': {
            'available': True,
            'loading': False,
            'resident_fold_current': True,
            'resident_fold_loading': False,
        },
    }

    assert MODULE.runtime_ready(resident_ready) is True
    resident_ready['shared_preload']['resident_fold_loading'] = True
    assert MODULE.runtime_ready(resident_ready) is False


def test_semantic_runtime_requires_query_trace() -> None:
    assert MODULE.requires_query_trace({'sae_enabled': True}) is True
    assert MODULE.requires_query_trace({'sae_enabled': False}) is False


def test_target_eviction_requires_runtime_to_leave_ready_state() -> None:
    ready = {
        'generation': {'auto_preload_priority': 100},
        'shared_preload': {
            'available': True,
            'loading': False,
            'query_warm_marker_valid': True,
            'query_metadata_warm': True,
        },
    }

    assert MODULE.target_eviction_observed(ready) is False
    ready['shared_preload']['query_metadata_warm'] = False
    assert MODULE.target_eviction_observed(ready) is True


def test_semantic_recovery_requires_the_same_query_route() -> None:
    baseline = {'trace': {'query_route': 'ranked_prefix'}}
    clients = [{
        'samples': [
            {'trace': {'query_route': 'ranked_prefix'}},
            {'trace': {'query_route': 'ranked_prefix'}},
        ],
    }]

    assert MODULE.query_route_stable(baseline, clients, True) is True
    clients[0]['samples'][1]['trace']['query_route'] = 'forward_rows'
    assert MODULE.query_route_stable(baseline, clients, True) is False
    assert MODULE.query_route_stable({}, clients, False) is True


def test_memory_plateau_rejects_unbounded_tail_growth() -> None:
    mib = 1024 * 1024
    stable = {
        'baseline': {
            'backend_rss_bytes': 100 * mib,
            'backend_memory_bytes': 20 * mib,
        },
        'samples': [
            {
                'backend_rss_bytes': value * mib,
                'backend_memory_bytes': 21 * mib,
            }
            for value in (110, 120, 121, 121, 121)
        ],
    }
    growing = {
        **stable,
        'samples': [
            {
                'backend_rss_bytes': value * mib,
                'backend_memory_bytes': 21 * mib,
            }
            for value in (110, 120, 121, 200, 220)
        ],
    }

    assert MODULE.memory_plateau(stable, 8 * mib)['passed'] is True
    assert MODULE.memory_plateau(growing, 8 * mib)['passed'] is False


def test_generation_identity_ignores_runtime_observations() -> None:
    status = {
        'generation_id': '1/2/3',
        'contract_signature': 'contract',
        'posting': {'signature': 'posting'},
        'primary': {'manifest_start_block': 42},
        'semantic_accelerator': {
            'source_manifest_id': '2',
            'builder_policy_id': 7,
        },
        'workload_fold': {'shared_query_observations': 10},
    }
    changed = {
        **status,
        'workload_fold': {'shared_query_observations': 100},
    }

    assert MODULE.generation_identity(status) == (
        MODULE.generation_identity(changed)
    )
