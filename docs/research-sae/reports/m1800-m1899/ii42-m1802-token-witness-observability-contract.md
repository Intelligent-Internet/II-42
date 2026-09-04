# M1802 Token-Witness Observability Contract

## Trigger

M1801 rejects static query multiprobing. At qTopK 64 and approximately 7.4
document-equivalent token reads, exhaustive-MaxSim candidate-upper O@100 is
only about 0.758 and direct routed scoring retains about 65% of that capacity.

The M1610 objective pooled token activations into a document vector. It never
trained the local CITADEL-style condition required by the new representation:

> a query token and the document token responsible for its MaxSim contribution
> should route to compatible keys.

M1802 tests whether this local target is observable before any training.

## Frozen Audit

For every heldout query token, select its maximum-cosine witness token from
three frozen teacher documents:

1. the disjoint weak-pair positive document;
2. pooled-dense top1;
3. exhaustive token-MaxSim top1.

The witness token's two frozen document keys are target labels. Measure their
rank in the current query router over all 8,192 keys, recall at
`1/4/16/64/256`, target margin, aligned-token cosine, unique keys/query, posting
reads, and an oracle query route that emits only the witness keys.

No qrels, dataset identity, checkpoint selection, or training is allowed.

## Training Gate

A frozen-root token-alignment router canary is authorized only if the weak-pair
surface satisfies all of:

- target-key recall@64 at least `0.75`;
- median best target-key rank at most `64`;
- mean aligned-token cosine at least `0.65`;
- witness-key oracle positive MRR@100 no worse than exhaustive MaxSim by more
  than `0.02`;
- witness-key oracle reads at most `8.0x` document equivalents.

The dense-top1 and MaxSim-top1 surfaces are diagnostics. They cannot override a
failed weak-pair gate.

## Stop Rule

If the gate fails, do not train another pooled sparse router. Any continuation
must jointly change the token representation and router, or adopt a published
late-interaction checkpoint rather than trying to recover the signal from raw
BGE token states.
