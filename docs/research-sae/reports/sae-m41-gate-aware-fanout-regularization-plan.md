# SAE M41 Gate-Aware Fanout Regularization Plan

Status: completed; see `sae-m41-gate-aware-fanout-regularization-results-report.md`.

## Summary

M40 found the strongest ranking-quality direction so far:

```text
validated broad-query source
+ runtime-safe lexical-DF SAE scale gate
  -> better full15 NDCG/MAP and better trec-covid MAP
```

The remaining M40 problem is physical cost. The best M40 query latents raise
mean SAE postings from M36 `2384.2` to `2734.7`. The lexical-DF gate changes
runtime score scale, but it does not change which SAE atoms are exported and
touched. M41 therefore targets the query atom support itself.

## Hypothesis

The useful M40 signal should be preserved while reducing query-time fanout if
training regularizes the atoms that enter the final exported query payload:

```text
M40 lexical-DF gate
+ top-k exported-atom fanout regularization
  -> keep most NDCG/MAP gain
  -> lower SAE postings and candidate docs
```

This is different from the older dense fanout loss. The older loss penalizes
the full soft latent vector. M41 adds a loss on the top-k support logits that
will actually become query atoms.

## Implementation

M41 extends `scripts/research_sae_m31_joint_final_ranking_train.py` with
opt-in arguments:

```text
--topk-fanout-loss-weight
--topk-fanout-active-dims
--broad-topk-fanout-multiplier
--ordinary-topk-fanout-multiplier
```

The new regularizer:

1. selects the top-k atom ids by query support logits;
2. gathers their soft activation values;
3. computes activation-weighted document-frequency cost;
4. applies optional broad-query and ordinary-query multipliers.

The top-k selection is discrete, so this is not a fully smooth atom allocator.
It is still aligned with deployment because gradients flow through the selected
activation/value path and discourage high-DF atoms in the exported payload.

M41 also updates `scripts/research_sae_m40_df_gate_sweep.py` to report physical
means from the same query latents used by the sweep:

```text
candidate_docs
bm25_postings
sae_postings
```

## Runs

All M41 runs use the M40/M39 training surface:

```text
/Volumes/Betty/Tmp/ii42_sae_m39_train_merged/m39-broad-plus-nfcorpus-expanded
```

Evaluation remains the full15 artifact:

```text
/Volumes/Betty/Tmp/ii42_sae_beir15_shared
```

The tested fanout-loss weights are:

| Run | top-k fanout weight | Broad multiplier | Ordinary multiplier |
| --- | ---: | ---: | ---: |
| `m41_w0p01` | 0.01 | 0.5 | 1.0 |
| `m41_w0p02` | 0.02 | 0.5 | 1.0 |
| `m41_w0p04` | 0.04 | 0.5 | 1.0 |

The broad multiplier is lower because M39/M40 showed broad queries are exactly
where semantic evidence is most fragile. M41 should reduce wasteful atom
fanout, not erase the broad-query semantic signal.

## Acceptance Criteria

M41 is useful if it forms a clear quality-cost frontier:

- M41 must lower SAE postings compared with M40 `2734.7`.
- M41 should preserve most of the M40 DF-gate quality gain.
- A single run does not need to dominate M40 on every metric if it provides a
  practical lower-cost operating point.

M41 should be rejected as a main direction if lowering postings always causes a
large NDCG/MAP collapse or destroys `trec-covid` MAP.
