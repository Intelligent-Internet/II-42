# M1963 Raw M190 Exact-BMP Engine-Cost Attribution

## Scope

This is a qrels-free systems diagnostic over the frozen M190 representation.
It does not change query atoms, document atoms, learned impacts, quantization,
or the predeclared M1963 product gate.

For query `q`, the exhaustive posting work is:

```math
W(q)=\sum_{a\in\operatorname{supp}(q)}df(a).
```

All three compared rows use document TopK-64, query TopK-80, 16,384 atoms,
u8 document impacts, u32 accumulation, and the same exact patched BMP engine.
This isolates corpus/query activation from model and engine changes.

## Observed Work And Latency

| Dataset | Documents | maxDF ratio | Mean W(q) | p95 W(q) | BMP p95 | Bytes/doc | Product gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| hotpotqa | 5,233,329 | 0.0824 | 2,882,121 | 3,797,330 | 82.00 ms | 1,245.7 | fail latency |
| fever | 5,416,568 | 0.0942 | 2,422,471 | 3,286,871 | 60.27 ms | 1,296.9 | fail latency |
| climate-fever | 5,416,593 | 0.0942 | 1,659,219 | 2,448,092 | 27.29 ms | 1,304.9 | pass |

The ordering of p95 posting work and p95 latency is identical. FEVER and
climate-FEVER are the strongest control: they have almost identical corpus
size, document nnz, maximum DF, index bytes, and build configuration, but
their query distributions produce materially different posting work and
latency.

This does not prove a per-query causal model from three aggregate rows. It
does show that corpus size, average document nnz, and maximum DF alone cannot
explain the observed cost. Query-weighted posting traversal is the missing
variable.

## Historical Pruning Check

The historical query-side `maxDF=0.12` policy is a no-op on all three rows:
their maximum document-frequency ratios are only `0.0824-0.0942`. The audit
therefore retains 100% of query L1/L2 mass and 100% of posting work at that
threshold.

This closes the M1961 shortcut for these large rows. Reopening the same static
threshold would not repair HotpotQA or FEVER and would repeat an already
measured failure mode.

## Literature And Mathematical Interpretation

[DF-FLOPS](https://arxiv.org/abs/2505.15070) motivates controlling corpus-wide
document frequency rather than treating average activation as a sufficient
cost proxy. M1963 adds the complementary query-weighted measurement `W(q)`:
even bounded individual `df(a)` values can sum to millions of posting visits
when every query carries 80 atoms.

[Faster Learned Sparse Retrieval with Block-Max Pruning](https://arxiv.org/abs/2405.01117)
and [Two-Step SPLADE](https://arxiv.org/abs/2404.13357) support separating
representation quality from traversal optimization. The exact-BMP audit
proves that the engine returns the complete quantized score surface; a latency
failure is therefore not evidence that M190 retrieval quality or quantization
failed.

If full15 quality passes, the next cost work should be engine-side term
ordering, tighter block bounds, or a documented learned-sparse index such as
[Seismic](https://arxiv.org/abs/2404.18812). It should not begin with another
representation loss or the ineffective `0.12` static query filter.

## Decision

- Retain climate-FEVER as an engine-and-quality passing row.
- Retain HotpotQA and FEVER quality evidence, but keep product promotion on
  hold because their predeclared 50 ms p95 gate fails.
- Do not alter the M1963 full15 quality decision until MSMARCO completes.
- Do not reopen M1961 pruning from this diagnostic.

Raw audit artifact:
`/Volumes/Betty/II42/m1963-m190-bmp-full15-v1/m1963_large_row_posting_work.json`.
