# M1820 PPLX Joint Token Router Contract

Date: 2026-07-12

## Question

Can the frozen, canonical `perplexity-ai/pplx-embed-v1-0.6B` token surface be
compiled into learned routing keys and compact signed vector payloads while
preserving the PPLX dense ranking in one physical inverted index?

M1820 is not a BM25, qrels, reranking, or scalar-posting experiment. It tests
the only unresolved mechanism after M1800-M1814:

```text
raw text
    -> frozen official PPLX token states
    -> jointly trained route and vector-payload heads
    -> key -> (document, route weight, fp16 vector payload)
    -> one inverted index
```

The optional dense/CLS side branch is forbidden.

## Fixed Surfaces

- Dense teacher: normalized official SentenceTransformer PPLX int8 output.
- Token input: one canonical pooled-root summary token plus final PPLX token
  embeddings from the same forward pass. The summary token is routed and
  stored in the same posting index; it is not an ANN or CLS side branch.
- Training data: corpus-derived MS MARCO query/positive/negative rows already
  frozen for M1518/M1600/M1610.
- Checkpoint selection: held-out PPLX dense overlap only. Paired positives are
  diagnostic and cannot select a checkpoint.
- Scoring: CITADEL-style shared learned routes, signed projected payload dot
  product, positive interaction clamp, and per-query-token MaxSim aggregation.
- Query routes per token: `1`.
- Document routes per token: `5`.
- Payload: `32` signed dimensions, evaluated after fp16 round-trip.

## Staged Gates

### S0: Signal Canary

- Train/validation queries: `512/128`.
- Candidate set: PPLX-dense-derived, qrels-free listwise candidates.
- Router namespace: `4096` keys.
- Maximum steps: `400`.
- Global load penalty: disabled.

Authorize S1 only if one non-initial checkpoint satisfies both:

- direct held-out dense O@100 improves by at least `0.05` over initialization;
- dense candidate-upper O@100 is at least `0.80`.

This is a learning-signal gate, not a product promotion gate.

### S1: Capacity Expansion

S1 is permitted only after S0 passes.

- Train/validation queries: `1000/250`.
- Router namespace: at least `8192` keys.
- Training depth may increase, but checkpoint selection remains held-out dense
  overlap with fixed milestones.

Authorize load-constrained training only if:

- direct dense O@100 is at least `0.95`;
- candidate-upper dense O@100 is at least `0.99`;
- paired-positive MRR does not regress by more than `0.05` from the dense
  teacher diagnostic.

### S2: Corpus-Global Load Constraint

S2 must use document-frequency measurements from the entire frozen training
corpus, not only a mini-batch sparsity proxy. The global DF vector may act as a
dual penalty on subsequent training intervals.

Authorize native closure only if S1 quality floors remain satisfied and the
maximum unique-document DF is reduced rather than exchanged for overlap.

### S3: Native Closure

Validation order is fixed:

1. SciFact and NFCorpus;
2. FiQA;
3. broader BEIR only if FiQA passes.

Final product gates:

- PPLX dense O@100 at least `0.99`;
- no material qrels regression versus PPLX dense;
- mean decoded FiQA payload at most `1 MiB/query`;
- index payload at most `10 KiB/document`.

## Stop Conditions

Stop the route without another loss or threshold sweep when any condition is
met:

1. S0 does not pass after the fixed 400-step run.
2. S1 improves listwise loss but cannot cross its hard overlap floors.
3. The corpus-global load penalty restores cost only by dropping below the S1
   quality floors.
4. FiQA remains above `1 MiB/query` after one predeclared load-constrained
   checkpoint.

Static routing, query-only adapters, post-hoc pruning sweeps, payload-width
sweeps, BM25 rescue, and dataset-specific policies are out of scope.

## Required Artifacts

- JSON and Markdown summary for every executed stage.
- ClearML task for every training run when the service is reachable.
- Cache manifests binding source data and the canonical PPLX model snapshot.
- Initial and selected checkpoints with immutable configuration.
- Unit tests, `py_compile`, runner `bash -n`, Ruff, and `git diff --check`.
