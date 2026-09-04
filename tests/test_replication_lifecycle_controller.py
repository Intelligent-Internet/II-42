from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT = REPO_ROOT / 'scripts' / 'test_replication_lifecycle_smoke.py'


def test_primary_generation_is_pinned_before_baseline_queries() -> None:
    source = SCRIPT.read_text(encoding='utf-8')

    lock_offset = source.index(
        'semantic_maintenance_lock = acquire_maintenance_lock('
    )
    generation_offset = source.index(
        'initial_semantic_generation = str(',
        lock_offset,
    )
    baseline_offset = source.index(
        'primary_initial_semantic_hits = semantic_hits(',
        generation_offset,
    )

    assert lock_offset < generation_offset < baseline_offset


def test_standby_must_replay_the_pinned_generation() -> None:
    source = SCRIPT.read_text(encoding='utf-8')

    assert "'semantic_generation_match': (" in source
    assert (
        "semantic['generation']['generation_id']\n"
        '                        ) == initial_semantic_generation'
    ) in source
