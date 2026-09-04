# M654W Boundary Preservation Loss

Status: `not_promoted_negative`

M654W tests whether M654U's shared15 failure can be fixed by moving a
dense-only top100 preservation constraint into the training loss.  It remains a
first-stage experiment: frozen P1.3/M549U document posting geometry,
query-side compiler only, no BM25, no reranker, no learned gate, and no
qrels-driven objective.

## Change

M654W adds an optional `boundary_preservation_loss` to the M654U trainer.

For each query, it precomputes:

- protected docs: `dense_top100 ∩ P1_top100`;
- negatives: dense/P1 candidate docs outside dense top100 and outside P1 top100.

During training it applies a soft min/max hinge:

- keep the protected-doc score floor above the negative-doc score ceiling;
- keep M654U's boundary-constrained teacher and all existing support losses.

The loss is disabled by default.  M654W explicitly ran it with weight `0.1`.

## Apples-to-Apples Shared15 Result

Run: `m654w_shared15_boundary_loss_w01_seed6545`.

This uses the same all-query split seed as
`m654u_shared15_boundary_seed6545`.

| Metric | M654U delta | M654W delta |
| --- | ---: | ---: |
| Dense O@50 | 0.000000 | +0.000222 |
| Dense O@100 | -0.000048 | -0.000108 |
| Dense O@256 | +0.000022 | -0.000012 |
| CUB | 0.000000 | 0.000000 |
| Recall@100 | 0.000000 | 0.000000 |
| MAP@100 | +0.000006 | +0.000006 |
| NDCG@10 | 0.000000 | 0.000000 |
| MRR@20 | 0.000000 | 0.000000 |
| Support cosine | -0.000000 | -0.000000 |

Decision: `dense_equivalence_gate_failed`.

Failed checks: `dense_overlap_100_safe`, `dense_overlap_256_safe`.

## Boundary Ledger

Same-seed test split:

| Run | Rows | O@100-loss rows | dO@100 | dO@256 | dRecall@100 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M654U seed6545 | 134 | 2 | -0.000074627 | +0.000029151 | 0.000000000 | +0.000005676 |
| M654W seed6545 | 134 | 3 | -0.000149254 | 0.000000000 | 0.000000000 | +0.000006361 |

M654W's held-out O@100-loss queries were in `climate-fever`, `nq`, and
`scidocs`.  The failure is still sparse, but the new loss does not reduce it.
It increases the O@100-loss count on the comparable split.

An independent seed (`m654w_shared15_boundary_loss_w01_seed6547`) also failed
held-out O@100, with one O@100-loss query and no strict-gate pass.

## Conclusion

The current boundary preservation loss is not the right fix.  It protects a
coarse protected-vs-new-negative margin, but it does not preserve exact
top100 membership under global compiler transfer.  It can improve O@50 while
worsening O@100/O@256, so the problem is not solved by adding another overlap
gate or by scaling this hinge loss.

This result narrows the next step: the failure needs doc-level swap diagnosis
or a different transfer architecture/objective.  Specifically, we need to know
which exact dense-top100 documents are lost and which replacement documents
enter, then decide whether the compiler needs a listwise membership-preserving
loss over those swap pairs or a more constrained adapter.

Do not promote M654W.  Do not scale this loss further without a doc-level swap
audit.
