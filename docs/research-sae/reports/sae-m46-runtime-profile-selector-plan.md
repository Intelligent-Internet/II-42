# SAE M46 Runtime Profile Selector Plan

Status: closed with a promotable quality-cost selector.

## Goal

M45 produced a frontier instead of one universally best profile:

```text
M44 lower-cost baseline
M45 balanced
M45 high-quality
```

M46 asks whether a runtime-safe per-query selector can choose among these
profiles and get closer to the best of all worlds:

- retain M45 high-quality aggregate ranking quality;
- recover some M44 `trec-covid` stability;
- reduce SAE postings versus always using M45 high-quality;
- avoid dataset IDs, qrels, dense embeddings, or product-specific rules.

## Selector Inputs

Allowed runtime-safe signals:

- query lexical document-frequency mean;
- query high-DF token share;
- BM25 score concentration;
- query length;
- predicted SAE postings from the exported query atoms;
- predicted candidate docs from the exported query atoms.

The selector is intentionally simple and deterministic. M46 is not a learned
model; it is a routing/control experiment over the established M44/M45 payload
frontier.

## Profiles

```text
m44_low_cost:
    pool96 -> export48
    fanout_power = 0.25
    threshold = 0.12
    low/high SAE = 0.40 / 1.00

m45_balanced:
    pool128 -> export48
    fanout_power = 0.10
    threshold = 0.12
    low/high SAE = 0.45 / 1.00

m45_high_quality:
    pool96 -> export48
    fanout_power = 0.15
    threshold = 0.12
    low/high SAE = 0.45 / 1.00
```

## Implementation

Add:

```text
scripts/research_sae_m46_profile_selector_sweep.py
```

The runner reuses the M45 checkpoint export machinery, but evaluates selector
policies over multiple precomputed profiles:

- static profiles;
- DF-broad fallback policies;
- predicted-fanout fallback policies;
- combined DF/fanout policies;
- BM25-concentration fallback policies.

Physical cost must use the same dataset-macro averaging as M40-M45 so results
remain comparable.

## Acceptance Rule

Promote an M46 selector if it either:

- improves NDCG/MAP over M45 high-quality while reducing SAE postings; or
- keeps quality close to M45 high-quality while materially improving TREC
  stability and lowering postings.

M46 does not close the dense-removal product gate. It only selects the current
best runtime profile frontier.
