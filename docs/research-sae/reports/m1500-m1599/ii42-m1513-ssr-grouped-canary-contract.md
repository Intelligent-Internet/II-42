# M1513 SSR Grouped-Posting Canary Contract

## Decision inherited from M1510-M1511

M1510 established that retrieval-trained sparse features contain real quality
signal, but the pooled SPLARE reproduction touches every document on all three
full official control corpora. M1511 showed that removing a few ubiquitous
atoms does not repair that collapse. M1512 is therefore stopped.

M1513 tests one structurally different hypothesis: preserve sparse token groups
and score them with sparse MaxSim in a single inverted index. This is not a
pooled posting model, a dense plus BM25 fusion, or a return to reconstruction-
only SAE training.

## Reproducibility correction

The public SSR repository at commit
`c64bc0599f3b00669db7b8dbddf130c07a6b3ea8` differs from the paper setup:

- `recons_4k` is computed but its loss coefficient is discarded.
- warmup is not exposed by the training CLI.
- document length is not exposed and defaults to 180 rather than the paper's
  controlled length of 32.
- README defaults use `auxK=1024`, learning rate `2e-5`, batch size 256, and
  one epoch rather than the paper's `auxK=2048`, learning rate `1e-3`, batch
  size 64, two epochs, and 20,000 warmup steps.

The M1513 patch restores the independent `1/8 * L_recon(4K)` term and exposes
warmup. Every canary command must still specify all paper-sensitive values
explicitly; defaults are not accepted as evidence.

## Paper-aligned objective

The controlled BERT experiment uses:

`L = L_recon(K) + 1/8 L_recon(4K) + 1/32 L_aux + 0.1 L_sparse_cl + 0.05 L_MaxSim`

The structural configuration is BERT base, latent width 16,384, K=32,
auxK=2,048, maximum sequence length 32, and BM25 negatives from MS MARCO.
The full paper schedule is batch size 64, two epochs, learning rate `1e-3`, and
20,000 warmup steps.

## Progressive experiment gates

### M1513-S0: implementation smoke

- Unit-test the K and 4K reconstruction terms independently.
- Run one forward/backward optimizer step on synthetic triplets.
- Seed model construction before the SAE is initialized and persist checkpoint
  zero; an unseeded or missing initial checkpoint invalidates comparison.
- Reloaded checkpoints must assert latent width 16,384 and K=32; silently
  rebuilding a default SAE invalidates all downstream metrics.
- Require all five configured losses and gradients to be finite.
- Require the 4K loss to be non-zero and separately logged.

Failure stops M1513 as an implementation failure, not a model result.

### M1513-S1: bounded signal canary

- Use 10,000 MS MARCO BM25 triplets with a held-out 1,000-triplet split.
- Use paper architecture and loss ratios.
- Limit runtime to a fixed small step budget; do not interpret absolute BEIR
  quality as a route verdict.
- Compare checkpoint zero and trained checkpoint on held-out reconstruction,
  MaxSim ordering, active features, dead-feature ratio, DF, fanout, and posting
  touches.

Proceed only if held-out MaxSim ordering improves, reconstruction does not
diverge, and sparse usage does not collapse into a few corpus-wide atoms.
Posting-touch gating must report both exact K=32 and the paper-native K=4
coarse surface. A full K=32 union is not alone a stop if K=4 coarse pruning
avoids near-full touch; K=4 near-full touch remains a hard stop.

If joint BERT plus SAE training collapses the backbone into a small shared
support, permit exactly one structural correction: freeze the pretrained BERT
backbone and train only the sparse projector with the same data and objective.
This follows the paper's modern-backbone control and directly tests whether
the collapse comes from moving dense token geometry rather than grouped
postings themselves.

### M1513-S2: depth validation

- Increase to at least 100,000 triplets and enough steps for a meaningful
  warmup/training segment.
- Evaluate zero-shot on full official NFCorpus and SciFact through SSR's
  inverted-index path.
- Compare K=16 and K=32 only after K=32 passes.

Proceed only if at least one trained checkpoint beats checkpoint zero on both
held-out MS MARCO and one independent corpus without near-full posting touches.

### M1513-S3: broader native validation

Only a passing S2 branch may move to FiQA, shared15, index-size and latency
measurement, and integration with the II-42 native index lifecycle.

## Hard stops

- Do not use BEIR test qrels for training or checkpoint selection.
- Do not make dataset-specific thresholds or loss weights.
- Stop after two objective corrections if checkpoint zero remains selected.
- Stop if K=16/32 cannot improve the M1401 quality/cost frontier.
- Stop if grouped postings approach full-corpus touches.
- Do not scale merely because training loss decreases.
