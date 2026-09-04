# SAE M44 Pool-Aware Export Regime Plan

Status: completed; see `sae-m44-pool-aware-export-regime-results-report.md`.

## Summary

M43 made utility/fanout-aware payload allocation canonical. M44 tests whether a
larger query atom pool can improve the quality-cost frontier:

```text
model emits larger support-logit pool
-> export smaller physical payload by weight / df^alpha
```

The main hypothesis is:

```text
pool64 -> export48 was good;
pool96 -> export48 may recover more useful atoms while keeping physical cost low.
```

## Workstreams

### M44.1 Eval-Only Pool Sweep

Start with the M40 checkpoint and avoid retraining:

```text
checkpoint = M40 lexical-DF gate model
pool = 96
export = 48 or 56
fanout_power = 0.25 or 0.5
```

This checks whether the model already has useful atoms beyond the old top64
pool.

### M44.2 Canonical Gate Sweep

For the best pool/export/fanout setting, sweep only the runtime score gate:

```text
thresholds = 0.10, 0.12, 0.15, 0.18
low_sae = 0.30, 0.35, 0.40
high_sae = 0.75, 1.0
```

### M44.3 Export-Aware Training

If eval-only pool96 is useful, test a conservative finetune where training
ranking loss sees the same exported query payload that runtime evaluation uses.

This requires two implementation fixes:

1. load the saved M31 joint weight head from checkpoint before finetuning;
2. compute training ranking scores from the exported query subset when
   `--train-exported-query-scoring` is enabled.

## Acceptance Criteria

M44 is promoted if it beats M43 on quality while keeping or improving physical
cost.

M44 training is promoted only if finetuning improves the eval-only M44 profile
without increasing SAE postings materially.
