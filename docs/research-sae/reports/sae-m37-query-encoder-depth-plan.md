# SAE M37 Query Encoder Depth Plan

Status: closed after transformer-token-LSE primary run.

## Summary

M36 closed two data routes:

- current M32 train has too little TREC-like broad/many-positive coverage;
- existing M35/M35b synthetic sources should not be reused.

M37 therefore tests a different hypothesis:

```text
maybe the query-side encoder is too shallow
```

The experiment intentionally keeps the data fixed to the M32 clean train root
so the result isolates model depth from data expansion.

## Primary Arm

M37 primary uses the existing M31 final-ranking trainer with an opt-in model
override:

```text
checkpoint: M29 Arm-A query-ranking text atom student
base data: M32 clean train root
eval data: current full15 root
encoder override: transformer_token_lse
transformer layers: 1
transformer heads: 4
partial checkpoint load: enabled
```

Partial load keeps compatible tensors from the M29 checkpoint, including vocab,
embeddings, trunk, and atom heads. The transformer block is initialized from
scratch.

## Acceptance

M37 only promotes if it improves the hard broad-query collapse without causing
aggregate regression:

- `trec-covid` should improve materially versus M32/M36;
- `msmarco`, `dbpedia-entity`, and `nfcorpus` must not collapse;
- full15 NDCG/MAP should remain close to M32;
- physical cost should remain near current EATMH profile or improve.

## Result

The primary transformer arm failed the gate.

See `sae-m37-query-encoder-depth-results-report.md`.
