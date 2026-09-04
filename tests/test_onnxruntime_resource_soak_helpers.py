from __future__ import annotations

from scripts import test_onnxruntime_resource_soak as resource_soak


def phase(
    *,
    growth_kb: int,
    worker_growth_kb: int,
    slope_kb: float,
) -> dict[str, object]:
    return {
        'growth_kb': growth_kb,
        'worker_growth_kb': {'100': worker_growth_kb},
        'tail_slope_kb_per_iteration': slope_kb,
        'worker_tail_slopes_kb_per_iteration': {
            '100': slope_kb,
        },
        'iterations': 500,
        'clients': 2,
        'successful_rows': 1000,
        'session_cache_activity': {
            'load_delta': 0,
            'eviction_delta': 0,
        },
    }


def test_settling_can_absorb_bounded_high_water_growth() -> None:
    settling = phase(
        growth_kb=20 * 1024,
        worker_growth_kb=20 * 1024,
        slope_kb=0.0,
    )

    assert (
        resource_soak.phase_completed_without_churn(
            settling,
        )
    )
    assert not resource_soak.phase_memory_is_bounded(
        settling,
        16 * 1024,
        64.0,
    )


def test_measured_phase_retains_growth_and_slope_limits() -> None:
    measured = phase(
        growth_kb=8 * 1024,
        worker_growth_kb=8 * 1024,
        slope_kb=1.0,
    )

    assert resource_soak.phase_memory_is_bounded(
        measured,
        16 * 1024,
        64.0,
    )

    measured['worker_tail_slopes_kb_per_iteration'] = {
        '100': 65.0,
    }
    assert not resource_soak.phase_memory_is_bounded(
        measured,
        16 * 1024,
        64.0,
    )
