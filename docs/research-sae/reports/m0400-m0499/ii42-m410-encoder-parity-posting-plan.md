# II-42 M410 Encoder Parity Posting Plan

Date: 2026-06-27

## Goal

M410 tests the hard boundary between M409 and a real text-to-posting model:

```text
raw text -> dense encoder -> deterministic posting
```

This is not another ranking-loss experiment.  It answers whether the raw text
path can reproduce the same dense vectors already used by M408, then whether
the deterministic posting post-process recovers the M408 route.

## Why

M408 proved that `dense embedding -> posting` is nearly solved.  M409 proved
that a small hash lexical encoder cannot learn the semantic mapping.  Therefore
the next useful question is not "add BM25" or "tune ranking loss"; it is:

```text
can the actual dense encoder regenerate the materialized dense geometry?
```

If yes, raw text to posting is feasible and the next stage is student
distillation.  If no, the materialized embedding generation path must be
reconciled first.

## Gate

For each task:

1. Load materialized `documents.jsonl`, `queries.jsonl`, and qrels.
2. Re-encode sampled documents and queries with
   `perplexity-ai/pplx-embed-v1-0.6B`.
3. Compare re-encoded dense vectors to materialized dense vectors after the
   same truncation and L2 normalization.
4. Apply the M408 deterministic rotation, active-coordinate split, and
   tail-sketch post-process to re-encoded vectors.
5. If full retrieval is enabled, evaluate:
   - materialized exact dense;
   - re-encoded exact dense;
   - materialized structural tail;
   - re-encoded structural tail;
   - the same rows with fixed BM25 z-blend.

## Promotion Rule

M410 passes only if:

- dense cosine parity is high on documents and queries;
- posting support parity is high enough to preserve the M408 structural route;
- full retrieval, when enabled, tracks materialized dense and materialized
  structural-tail rows rather than collapsing like M409.

If M410 passes, build M411 as a student distillation problem:

```text
raw text -> smaller semantic encoder -> dense/posting target
```

If M410 fails, do not train a student yet.  First reconcile the encoder
configuration, prompt, truncation, quantization, and normalization path.
