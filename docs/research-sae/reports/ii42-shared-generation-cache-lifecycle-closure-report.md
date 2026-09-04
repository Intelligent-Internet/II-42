# II-42 Shared Generation Cache Lifecycle Closure Report

Status: closed

This report closes the temporary shared generation cache lifecycle planning
track. The canonical product documentation is now:

- [Shared Runtime And Residency](../../shared-runtime-and-residency.md)
- [Index Policy](../../index-policy.md)
- [Index Parameters](../../index-parameters.md)

## Closed Policy

The shared holder is a volatile cache tier. The PostgreSQL index relation
remains the durable source of truth, and query correctness must not depend on a
specific resident entry staying in shared memory.

When the optional shared-preload arena is configured, immutable generations
prefer this order:

1. Attach an already resident shared-preload generation.
2. Wait for an existing shared-preload publisher or preload worker.
3. Publish the generation into the shared-preload arena with single-flight
   coordination.
4. Use DSM for DSM-eligible large generations when shared-preload is
   unavailable or cannot admit the generation.
5. Use backend-local decode only for small non-DSM generations after shared
   attach, wait, publish, and eligible pressure eviction cannot admit them.

DSM-eligible large generations remain share-required. They must not silently
fall back to private backend-local copies, because connection pools can amplify
one large generation into many large private copies.

## Closed Implementation

The implementation is in `src/ii42_am.c`:

- `ii42_am_generation_share_required(...)` now reserves required-share behavior
  for DSM-eligible large generations.
- Shared-preload entries carry `auto_preload` priority and access-clock state.
- Background preload is conservative: it does not evict marked preload
  residents to load another marked resident.
- Foreground query pressure can evict ordinary unreferenced residents,
  generic blobs, lower-priority preload residents, and finally older
  same-priority residents as a last-resort query tradeoff.
- Active residents with `refcount > 0` are never eviction candidates.
- Oversized or blocked marked preload candidates record admission-miss
  diagnostics and bounded warning state instead of spinning.

The SQL diagnostics expose the policy through
`ii42_generation_cache_state(...)` and `ii42_generation_cache_state_json(...)`.

## Closed Validation

The closure regression bundle is:

```bash
python3 -m py_compile \
    scripts/test_shared_preload_lifecycle_closure.py \
    scripts/test_shared_preload_generation_cache.py \
    scripts/test_shared_preload_auto_preload.py \
    scripts/test_shared_preload_generation_rollout.py

git diff --check -- \
    scripts/test_shared_preload_lifecycle_closure.py \
    scripts/test_shared_preload_generation_cache.py \
    scripts/test_shared_preload_auto_preload.py \
    scripts/test_shared_preload_generation_rollout.py \
    src/ii42_am.c \
    sql/ii42--0.1.0.sql \
    docs/shared-generation-cache.md \
    docs/index-policy.md \
    docs/index-parameters.md

PATH="/opt/homebrew/opt/postgresql@18/bin:$PATH" make

python3 scripts/test_shared_preload_lifecycle_closure.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin
python3 scripts/test_shared_preload_generation_cache.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin
python3 scripts/test_shared_preload_auto_preload.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin
python3 scripts/test_shared_preload_generation_rollout.py \
    --bindir /opt/homebrew/opt/postgresql@18/bin
python3 scripts/test_generation_cache_smoke.py \
    --psql /opt/homebrew/opt/postgresql@18/bin/psql \
    --dbname postgres
```

The command above is preserved as dated evidence. Its former product-document
path is retired; the current contract is
[Shared Runtime And Residency](../../shared-runtime-and-residency.md).

The new lifecycle closure test specifically verifies:

- oversized marked `auto_preload` indexes produce admission-miss diagnostics;
- the bounded warning path includes admission-miss counts;
- foreground pressure publishes ordinary non-DSM indexes into shared-preload
  instead of immediately falling back to backend-local decode;
- high-priority marked residents survive lower-priority foreground churn;
- relation-entry eviction counters and access-clock state move under pressure.

## Non-Blocking Follow-Up

The lifecycle is closed for correctness and basic priority-aware pressure
behavior. Future production observability could add active tier labels, failed
attach counters, and invalidated generation counters, but those are diagnostics
improvements rather than blockers for this cache lifecycle policy.
