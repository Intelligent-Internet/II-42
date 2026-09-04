# M1801 Static Token Multiprobe Contract

## Trigger

M1800A established that routed token MaxSim retains approximately 99% of the
exact-MaxSim candidate upper, and that deterministic int8 projection preserves
the routed score. The failed gate is candidate admission from the frozen M1610
query router.

M1801 asks one finite question before introducing a new teacher:

> Can a larger but still bounded static query multiprobe expose the frozen
> document source capacity once token payload scoring is retained?

## Frozen Surface

- M1610B 8,192-key initialization;
- document route Top-K fixed at 2;
- 64-dimensional deterministic int8 payload;
- query route Top-K in `8,16,32,64`;
- document-equivalent decoded-entry caps in `1,2,4,8`;
- exhaustive token MaxSim remains the qrels-free target;
- no training, qrels, dataset identity, threshold fitting, or checkpoint
  selection.

## Gate

Static multiprobe is viable only if one policy reaches exhaustive-MaxSim
candidate-upper O@100 of at least `0.90`, while:

- index payload remains below `5 KiB/document`;
- decoded payload remains below `1 MiB/query` on average;
- direct routed score retains at least `0.80` of candidate upper;
- positive MRR@100 remains within `0.02` of exhaustive MaxSim.

## Stop Rule

If no row passes, stop static Top-K, radius, co-occurrence, and threshold
expansions. The next allowed experiment is a token-witness observability audit:
derive document-token route labels from exhaustive MaxSim alignments and test
whether those labels are predictable from query token states before training.
