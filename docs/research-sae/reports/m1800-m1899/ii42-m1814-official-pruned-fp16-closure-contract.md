# M1814 Official Pruned-FP16 Closure Contract

## Fixed Policy

Use the official CITADEL retrieval shape without fitting:

- query top-1;
- document top-5;
- route-weight pruning greater than `0.9`;
- 32-dimensional fp16 payload;
- full traversal of every selected query key.

The M1812 caches predict `5.70 KiB/document` on SciFact and
`5.49 KiB/document` on NFCorpus. This is the only untested policy that removes
int8 rank perturbation while remaining below the frozen index-cost gate.

## Gate

Both complete official datasets must satisfy:

- conditional O@10/O@100/O@256 against all nonzero float learned-route
  results at least `0.999`;
- NDCG@10, MAP@100, Recall@100, and MRR@20 within `0.001` of truth;
- average query payload at most `1 MiB`;
- index payload at most `10 KiB/document`.

Passing proves the learned-key vector-payload representation is compatible
with one bounded inverted index. It does not promote the external checkpoint:
M1812 separately establishes that this MS MARCO model is weaker than the PPLX
dense product root on these BEIR rows.
