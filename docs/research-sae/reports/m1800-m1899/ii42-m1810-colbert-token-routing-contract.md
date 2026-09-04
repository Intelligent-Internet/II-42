# M1810 ColBERT Token-Routing Capacity Contract

## Trigger

M1804 closes the raw-BGE/frozen-M1610 router family. The remaining route-level
question is whether the single-index token payload design works when the token
representation was actually trained for late interaction.

M1810 uses the official `colbert-ir/colbertv2.0` checkpoint as a representation
control. It does not adopt PLAID, ANN, or ColBERT's production engine.

## Frozen Mechanism Canary

1. Encode the existing M1610 1,000/250 disjoint MS MARCO rows with the official
   ColBERT query/document markers, 32-token query augmentation, 64-token
   document cap, punctuation masking, and 128-dimensional normalized output.
2. Verify exhaustive MaxSim positive retrieval.
3. Fit a qrels-free 4,096-key spherical k-means codebook from frozen train
   document tokens.
4. Route query/document tokens to nearest keys.
5. Store compact token payloads in key posting lists and score only matching
   query/document-token routes.
6. Report candidate upper, direct score, positive MRR, decoded entries, bytes,
   maximum DF, and index bytes.

The canary is seen-domain and only tests representation/index compatibility.
It cannot support a broader generalization claim.

## Gate

The route passes only if one 64-dimensional int8 policy satisfies all of:

- exhaustive MaxSim positive MRR@100 at least `0.85`;
- candidate-upper O@100 against exhaustive MaxSim at least `0.90`;
- direct routed O@100 retains at least `0.80` of candidate upper;
- routed positive MRR@100 is within `0.03` of exhaustive MaxSim;
- decoded payload is at most `1 MiB/query` on average;
- index payload is at most `10 KiB/document`.

## Stop Rule

If exhaustive ColBERT MaxSim fails, stop because the checkpoint/tokenization
surface is invalid. If exact MaxSim passes but routed capacity fails, do not
train another local adapter: the next distinct mechanism would require the
published CITADEL learned router or a joint large-corpus router. If the routed
gate passes, authorize one native/small-BEIR engineering replay before any
model training.
