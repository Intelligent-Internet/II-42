from __future__ import annotations

from scripts import test_backend_memory_ownership as memory_ownership


def test_parse_proc_private_memory_uses_writable_private_pages() -> None:
    smaps = '''
1000-2000 rw-p 00000000 00:00 0 [heap]
Pss:                   3 kB
Private_Clean:         1 kB
Private_Dirty:         2 kB
Private_Hugetlb:       0 kB
2000-3000 r--p 00000000 00:00 0 /tmp/read-only
Pss:                   4 kB
Private_Clean:         4 kB
Private_Dirty:         0 kB
Private_Hugetlb:       0 kB
3000-4000 rw-s 00000000 00:00 0 /dev/shm/shared
Pss:                   1 kB
Private_Clean:         0 kB
Private_Dirty:         1 kB
Private_Hugetlb:       0 kB
'''.strip()
    rollup = '''
1000-4000 ---p 00000000 00:00 0 [rollup]
Rss:                  20 kB
Pss:                  12 kB
Private_Clean:         5 kB
Private_Dirty:         3 kB
'''.strip()

    observed = memory_ownership.parse_proc_private_memory(smaps, rollup)

    assert observed['observer'] == 'linux_proc_smaps'
    assert observed['physical_footprint_bytes'] == 12 * 1024
    assert observed['private_writable_bytes'] == 3 * 1024
    assert observed['private_live_writable_bytes'] == 3 * 1024
    assert observed['malloc_allocated_bytes'] is None
    assert observed['allocator_empty_bytes'] is None
    assert observed['private_writable_by_region_bytes'] == {
        '[heap]': 3 * 1024,
    }


def test_parse_proc_private_memory_falls_back_to_mapping_pss() -> None:
    smaps = '''
1000-2000 rw-p 00000000 00:00 0
Pss:                   7 kB
Private_Clean:         2 kB
Private_Dirty:         3 kB
'''.strip()

    observed = memory_ownership.parse_proc_private_memory(smaps, None)

    assert observed['physical_footprint_bytes'] == 7 * 1024
    assert observed['private_writable_bytes'] == 5 * 1024
    assert observed['private_writable_by_region_bytes'] == {
        '[anonymous]': 5 * 1024,
    }


def test_unavailable_metrics_remain_unavailable() -> None:
    assert memory_ownership.nonnegative_growth(None, 1) is None
    assert memory_ownership.nonnegative_growth(1, None) is None
    assert memory_ownership.scaled_growth(None, 1) is None
    assert memory_ownership.scaled_growth(1, None) is None
    assert memory_ownership.nonnegative_growth(8, 3) == 5
    assert memory_ownership.scaled_growth(3, 8) == 0
