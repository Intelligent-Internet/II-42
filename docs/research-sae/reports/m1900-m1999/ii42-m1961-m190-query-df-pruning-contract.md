# M1961 M190 Query-DF Pruning Contract

## Question

M1960 has shown that frozen M190 learned impacts remain retrieval-competitive,
while exact posting work grows with corpus size:

```text
T(q) = sum_{a in A_q} df(a)
```

M1961 asks one bounded question:

> Can the historically selected query-side `max_df_ratio=0.12` policy reduce
> exact M190 posting traversal while preserving the modern native ranking?

This is a train-free cost diagnosis. It is not a new model family, scorer,
gate, or threshold search.

## Prior Evidence

The historical M150 C4 audit fixed `doc96/query96` and found that query-side
`max_df_ratio=0.12` reduced mean semantic posting work from `1,368,358` to
`1,196,693` while improving Recall@100, NDCG@10, and MAP@100. A ratio of `0.05`
collapsed quality. The old C4 labels therefore informed the `0.12` choice;
M1961 is an independent modern confirmation of that frozen choice, not a
qrels-free discovery. It sees no M1960 retrieval labels before execution.

The literature constrains the interpretation:

- [DeepImpact](https://arxiv.org/abs/2104.12016) and
  [uniCOIL](https://arxiv.org/abs/2106.14807) support contextual learned impact
  values, so document weights remain frozen.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) shows why row nnz does not
  control corpus-wide posting-list length, while also warning that a high-DF
  term may still be salient for a particular query.
- [Two-Step SPLADE](https://arxiv.org/abs/2404.13357) treats retrieval quality
  and traversal efficiency as separate proof surfaces.
- [Term Impact Decomposition](https://aclanthology.org/2022.findings-emnlp.205/)
  and [Block-Max Pruning](https://arxiv.org/abs/2405.01117) demonstrate that
  learned-sparse latency can instead be attacked inside the retrieval engine
  without deleting query evidence.

M210-M222 already rejected scorer calibration, adaptive admission, and safety
gate searches as a meaningful continuation. M1961 must not reproduce them.
If fixed query pruning fails its quality floor, the evidence points to
impact-aware/dynamic engine pruning rather than a new DF threshold.

## Frozen Policy

The primary policy is fixed before modern native replay:

```text
A_q(0.12) = {a in A_q : df(a) / N <= 0.12}
```

It changes query support only. It does not change:

- the M190 checkpoint or PPLX embeddings;
- document support or learned document impacts;
- retained query impacts;
- index rows or posting values;
- BM25, qrels, dataset identity, or scoring parameters.

Ratios `0.20`, `0.10`, and `0.05` are permitted only in the qrels-free
observability table to confirm the historical mechanism boundary. They cannot
be selected using M1961 retrieval metrics.

## Execution Ladder

1. Complete the frozen M1960 broad9 baseline without competing I/O.
2. Run a qrels-free observability audit over exported document/query CSR
   matrices.
3. Permit native replay only when every query remains non-empty, mean L2 query
   impact retention is at least `0.80`, and predicted mean `T(q)` falls by at
   least `5%`.
4. Materialize only fixed-0.12 query JSONL, signature-bind it to the source
   surface, and query the unchanged native raw M190 postings.
5. Evaluate broad9 once. Expand to full15 only if the quality/cost gate passes.

## Native Acceptance Gate

The fixed-0.12 policy must satisfy all of the following against raw M190:

- mean and p95 exact posting work each improve by at least `10%`;
- no principal macro metric decreases by more than `0.002` absolute;
- at least one of Recall@100, MAP@100, NDCG@10, or MRR@20 improves;
- no dataset loses more than `0.01` absolute on a principal metric;
- query IDs, query count, document index, and qrels remain identical;
- no dataset-specific threshold or post-hoc selection is used.

Before native quality results are available, the cost aggregation is fixed as
an equal-dataset macro, matching the BEIR quality macro:

```text
R_mean = mean_d(1 - mean_q T_0.12(q, d) / mean_q T_raw(q, d))
R_p95  = mean_d(1 - p95_q T_0.12(q, d) / p95_q T_raw(q, d))
```

Both `R_mean` and `R_p95` must be at least `0.10`. Query-weighted totals are
reported as an engineering diagnostic but cannot replace the contracted macro
gate. A dataset may save less than 10 percent because the policy is globally
fixed; it still remains subject to the per-dataset `0.01` quality-loss floor.

These are joint constraints. A large cost reduction cannot buy a material
quality regression, and a quality gain cannot hide unchanged posting work.

## Stop Conditions

Stop M1961 without another threshold or loss when:

- fixed `0.12` fails the observability gate;
- broad9 native replay fails any acceptance condition;
- the apparent saving comes from empty or near-empty queries;
- a result requires latent-IDF replacement, BM25 rescue, per-row selection, or
  a different document index.

If fixed `0.12` passes, it is a deterministic query policy over the M190
learned-impact index. Only then may a later experiment ask whether a trained
query compiler can internalize the same demonstrated policy.

## Required Artifacts

- `scripts/audit_m1961_m190_df_pruning.py`
- `m1961_df_pruning_observability.json`
- `ii42-m1961-m190-query-df-pruning-observability-report.md`
- signature-bound fixed-0.12 query surfaces, only if observability passes
- native broad9/full15 comparison, only as allowed by the ladder
