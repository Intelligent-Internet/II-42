# SAE M45 Checkpoint Export Sweep Plan

Status: closed by focused full15 sweep and exact M31 confirmations.

## Goal

M44 established `pool96 -> export48` as useful, but it left one open question:
whether a larger support-logit pool, especially `pool128`, gives more useful
ranking headroom before changing the model or training objective.

M45 tests this as an evaluation-only checkpoint export sweep:

```text
M40 checkpoint
-> cached query support/value outputs
-> pool/export/fanout payload selection
-> lexical-DF BM25+SAE gate sweep
```

The purpose is not to train a new model. It is to decide whether the current
M40 checkpoint has better physical payload settings than the M44 canonical
profile.

## Implementation Plan

Add a reusable runner:

```text
scripts/research_sae_m45_checkpoint_export_sweep.py
```

The runner must:

- load the checkpoint once;
- build each full15 dataset context once;
- encode each query once into support logits and value predictions;
- cache BM25 normalized scores once per dataset/query;
- generate query payload variants by `pool -> export` fanout-aware selection;
- evaluate many lexical-DF gate rows without repeated checkpoint reloads;
- output both JSON and Markdown reports.

This is deliberately separate from M31 training. M31 remains the exact
confirmation path, while M45 is the fast sweep path.

## Focused Sweep

The initial full grid was too broad for interactive iteration, so M45 uses a
focused grid around the M44 profile:

```text
pool_active_dims: 96,128
export_active_dims: 48
fanout_power: 0.10,0.15,0.20,0.25,0.30,0.35
threshold: 0.10,0.12,0.15
low_sae: 0.35,0.40,0.45
high_sae: 1.00
```

This grid is enough to answer the M45 question because it covers:

- the M44 canonical point: pool96/export48/fanout0.25/low0.40/high1.00;
- less aggressive fanout discounting for quality recovery;
- pool128 as the larger-pool candidate;
- the low-SAE settings that trade broad-query suppression against full15
  aggregate quality.

## Acceptance Criteria

M45 can promote a new profile only if it forms a clear quality/cost frontier
against M44:

- higher full15 NDCG/MAP than M44;
- physical cost still materially below the unpruned M40 payload;
- exact M31 evaluation confirms the sweep result;
- hard-dataset collapse is reported separately, not hidden by aggregate
  improvements.

M45 does not attempt to pass the dense-removal product gate. It is a payload
allocation and gate-selection milestone only.
