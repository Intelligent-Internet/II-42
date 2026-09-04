# M653F Tail-Listwise Dense Preservation

Status: `not promoted`

This is a first-stage dense-equivalence probe.  It does not use BM25, qrels,
rerankers, learned gates, or dataset-specific tuning.  It keeps P1.3 document
posting geometry frozen and trains only the query-side boundary compiler.

## Motivation

M653-E passed the canary gate but failed full shared15 replay because
`dense_overlap_at_256` regressed by about `-0.000020`.  Failure analysis showed
this was a tail-preservation issue rather than support collapse.  M653F tests
whether adding dense top256 listwise teacher pressure can preserve that tail.

The literature scan supports the shape of this test but not blind expansion:

- [DREAM](https://arxiv.org/html/2606.24667v1) argues that candidate-set
  competition and the training interface determine retrieval signal quality.
- [SAE-SPLADE](https://arxiv.org/html/2604.21511v1) supports SAE-like sparse
  concepts as useful retrieval interfaces, but not traditional reconstruction
  loss as sufficient for our dense-equivalence target.
- [Li-LSR](https://hltcoe.jhu.edu/wp-content/uploads/2025/08/2025_SIGIR_Exploring_Expansion_in_LSR.pdf)
  reinforces that sparse-retrieval quality depends strongly on regularization
  and representation length, so tail/listwise pressure must be measured through
  overlap gates, not accepted by loss reduction alone.

## Implementation

Added optional arguments to
`scripts/train_m653_dense_boundary_compiler.py`:

- `--dense-teacher-root`
- `--tail-listwise-weight`
- `--tail-listwise-temperature`
- `--tail-listwise-top-k`
- `--tail-listwise-candidate-k`
- `--tail-listwise-query-batch-size`

The tail examples are built from
`runs/m653_dense_teacher_top256_canary_v1/*/dense_rankings.jsonl`.  For each
query, the candidate list is the union of dense teacher topK and P1 topK.  The
teacher distribution is dense-root scores over that union.  The student
distribution is frozen P1 doc postings scored by the generated query, masked to
the original P1 query support.  This keeps the objective inside first-stage
dense preservation.

Tail example counts:

```json
{
  "dev": 60,
  "test": 86,
  "train": 253
}
```

## Runs

### `m653f_tail_w005_seed6532`

This first run used `tail-listwise-weight=0.05`, but the original
implementation used `kl_div(..., reduction='batchmean')` on a one-dimensional
doc distribution.  That incorrectly diluted the per-query KL by candidate
count.  The observed `tail_listwise` loss was only about `0.00006`, so the
effective gradient was near zero.

Canary test gate passed, but this is not strong evidence because the tail loss
was under-scaled:

| Delta | Value |
| --- | ---: |
| O@100 | `+0.000125819` |
| O@256 | `+0.000039062` |
| CUB | `+0.000000000` |
| Recall@100 | `+0.001680672` |
| MAP@100 | `+0.000377947` |

Shared15 replay still failed the same strict gate:

| Delta | Value |
| --- | ---: |
| O@100 | `+0.000029536` |
| O@256 | `-0.000017562` |
| CUB | `+0.000018623` |
| Recall@100 | `+0.000111558` |
| MAP@100 | `-0.000063293` |

Conclusion: weak tail pressure slightly reduces the O@256 regression versus
the original `-0.000020166`, but does not eliminate it.

### `m653f_tail_sum_w005_seed6532`

After fixing the KL reduction to `sum`, `tail-listwise-weight=0.05` became a
real objective.  It was too strong.  No dev checkpoint passed the gate; final
test selected epoch0 and failed overlap guards.

| Delta | Value |
| --- | ---: |
| O@100 | `-0.000905858` |
| O@256 | `-0.000104595` |
| CUB | `+0.000000000` |
| Recall@100 | `+0.000000000` |
| MAP@100 | `+0.000036552` |

Conclusion: real top256 listwise pressure conflicts with strict dense-overlap
preservation at this weight.

### `m653f_tail_sum_w001_seed6532`

Lowering the corrected KL objective to `tail-listwise-weight=0.001` restored a
dev-selected checkpoint but still failed canary test on `dense_overlap_at_256`.

| Delta | Value |
| --- | ---: |
| O@100 | `+0.000125819` |
| O@256 | `-0.000018382` |
| CUB | `+0.000000000` |
| Recall@100 | `+0.001680672` |
| MAP@100 | `+0.000377947` |

Conclusion: once the tail-listwise objective has meaningful scale, it does not
solve the tail-preservation failure; even low weight flips O@256 negative on
the canary.

## Decision

Do not promote M653F.

The result is useful because it narrows the bottleneck.  The issue is not
simply that M653-E lacked a dense top256 listwise term.  With correct scale,
that term spends dense overlap before it fixes the shared15 tail.  This
supports the current user hypothesis that the first-stage problem is deeper
than scorer/reranker tuning, but it also argues against solving it by adding a
small auxiliary loss to the existing query-side boundary compiler.

## Next Step

The next first-stage experiment should change the compiler/teacher geometry
rather than add another small loss term:

1. Keep P1.3/M549U signed-dot as frozen baseline and keep qrels/BM25 out of
   training.
2. Test a two-head query compiler with an explicit dense-tail head whose output
   is selected by dense-overlap replay gate, not by training loss alone.
3. If query-side only still cannot move boundary positives without O@256 loss,
   isolate a doc-side compiler probe with frozen query geometry.  Do not mix
   query and doc updates in the first doc-side test.
4. Only after one side proves dense-equivalence should we return to native
   full-matrix engineering evaluation.
