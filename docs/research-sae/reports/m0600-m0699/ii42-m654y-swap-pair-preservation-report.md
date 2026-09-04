# M654Y Swap-Pair Preservation

Status: `mixed_signal_not_promoted`

M654Y tests a more precise first-stage fix for the M654U/M654W failure.  It
keeps frozen P1.3/M549U document postings, trains only the query-side compiler,
and does not use BM25, rerankers, learned gates, or qrels-driven objectives.

## Change

M654X showed that shared15 O@100 failures are sparse rank100/101 swaps: a
dense-hit document at P1 rank 100 gets pushed to candidate rank 101 by a nearby
non-dense challenger.  M654W's coarse protected-vs-negative hinge did not fix
this.

M654Y therefore adds a targeted `swap_preservation_loss`:

- protected positives: P1 top100 documents that are also in dense top100,
  restricted to the P1 tail;
- challengers: P1 rank101+ window documents outside dense top100;
- pair target: preserve the original P1 margin between the protected positive
  and challenger.

This loss is disabled by default.  M654Y explicitly tested weights `0.25` and
`1.0`.

## Results

All rows are shared15 all-query split with seed6545 unless noted.

| Run | Decision | O@50 | O@100 | O@256 | CUB | Recall@100 | MAP@100 | Support cosine |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M654U baseline | fail O@100 | 0.000000 | -0.000048 | +0.000022 | 0.000000 | 0.000000 | +0.000006 | -0.000000 |
| M654Y w0.25 | fail O@100/O@256 | +0.000111 | -0.000048 | -0.000031 | 0.000000 | 0.000000 | -0.000026 | -0.000000 |
| M654Y w1.0 | pass | +0.000212 | +0.000026 | +0.000131 | 0.000000 | 0.000000 | -0.000026 | -0.000002 |
| M654Y w1.0 seed6547 | no checkpoint | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | -0.000000 |

Weight `1.0` is the first variant in this local sequence to pass the shared15
macro dense-equivalence gate on seed6545.  It keeps CUB and Recall@100 flat and
improves O@50/O@100/O@256.  However, MAP@100 regresses and an independent
seed6547 run selects no trained checkpoint.

## Doc-Level Swap Audit

M654Y w1.0 seed6545 passes macro O@100, but not by eliminating all dense-hit
losses.  It balances gains and losses:

| Run | Rows | Dense-hit gains | Dense-hit losses | Net dDenseHits@100 |
| --- | ---: | ---: | ---: | ---: |
| M654U seed6545 | 134 | 1 | 2 | -0.000075 |
| M654Y w1.0 seed6545 | 134 | 8 | 8 | 0.000000 |

This explains the mixed result.  The pairwise loss can recover macro dense
overlap, but it increases doc-level churn.  The model is not cleanly preserving
the original boundary; it is trading boundary docs across queries.

## Conclusion

M654Y provides a real first-stage signal: exact swap-pair preservation is more
useful than M654W's coarse hinge and can make shared15 macro dense-equivalence
gates pass.  It is still not promotable:

- MAP@100 regresses on the passing seed;
- seed6547 selects no trained checkpoint;
- doc-level audit shows gain/loss churn rather than no-loss preservation.

The next step should not be a blind weight sweep.  The useful information is
that a pairwise boundary objective can move the right macro metric, but the
current global adapter is too unconstrained.  A follow-up should either:

- constrain the adapter further so swap-pair loss cannot create new
  dense-hit losses elsewhere; or
- select/checkpoint using query-level no-loss swap counts, not only macro
  overlap averages.

M654Y should be preserved as a mixed positive signal, not promoted.
