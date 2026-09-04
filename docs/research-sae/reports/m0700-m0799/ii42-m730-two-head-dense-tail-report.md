# M730 Two-Head Dense-Tail Compiler

Status: `dense_equivalence_gate_failed`

M730 tests whether the M653 boundary objective can be made safer by changing
the query-side compiler geometry.  M729 showed that the single-head direct
boundary objective spills dense overlap at top256 even when the query is covered
by the training rows.  M730 therefore splits the residual into:

- a boundary head for dense top100 missing-vs-false-P1 swaps;
- a tail head for dense/P1 top256 membership preservation;
- frozen document postings and frozen P1 native signed-dot geometry;
- no BM25, no reranker, no learned gate, and no qrels-driven training loss.

Qrels are used only for held-out reporting.  Checkpoint selection is governed
by dense-equivalence gates.

## Runs

| Run | Boundary scale | Tail scale | LR | Steps | Selected trained checkpoint | Gate |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `m730_two_head_dense_tail_seed7301` | 0.006 | 0.006 | 8e-4 | 160 | no | failed |
| `m730_two_head_dense_tail_s001_seed7301` | 0.001 | 0.001 | 5e-4 | 160 | no | failed |

Both runs wrote a final JSON and a per-run Markdown report under
`runs/m730_two_head_dense_tail_compiler_v1/`.  Both selected epoch0/P1 fallback
because no trained checkpoint passed the dev dense-equivalence gate.

## Example Counts

```json
{
  "dev": 1842,
  "test": 2491,
  "train": 7853
}
```

## Dev Gate Trace

The top-level test deltas are zero because both runs correctly fell back to the
untrained P1 baseline.  The meaningful signal is the dev trace below.

| Run | Epoch | Step | Failed checks | dO@100 | dO@256 | dR@100 |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| default | 1 | 62 | active support, O@100, O@256 | -0.005028 | -0.001949 | +0.000000 |
| default | 2 | 124 | active support, O@100, O@256, support cosine | -0.008080 | -0.003480 | +0.003125 |
| default | 3 | 160 | active support, O@100, O@256, support cosine | -0.007737 | -0.004054 | +0.003125 |
| conservative | 1 | 62 | active support, O@100 | -0.000522 | +0.000182 | +0.000000 |
| conservative | 2 | 124 | active support, O@100, O@256 | -0.000875 | -0.000405 | +0.000000 |
| conservative | 3 | 160 | active support, O@100, O@256 | -0.001365 | -0.000137 | +0.000000 |

## Final Fallback Macro

Since no trained checkpoint passed, the final selected model is P1-native for
both runs.  The fallback macro is therefore unchanged versus P1:

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 | Support cos |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `dense_root` | 0.983333 | 1.000000 | 1.000000 | 1.000000 | 1.000000 | 0.659914 | 0.575559 | 0.863747 | 0.766896 | n/a |
| `p1_native` | 0.983333 | 0.940281 | 0.943517 | 0.947674 | 0.950880 | 0.661792 | 0.580396 | 0.863047 | 0.775430 | 1.000000 |
| `m730_selected` | 0.983333 | 0.940281 | 0.943517 | 0.947674 | 0.950880 | 0.661792 | 0.580396 | 0.863047 | 0.775430 | 1.000000 |

## Interpretation

The default scale is clearly unsafe: it quickly spends dense overlap at both
top100 and top256.  The conservative scale reduces the damage by about an order
of magnitude, but it still fails the core first-stage gate: dense overlap@100
regresses at every checkpoint, and later checkpoints also regress dense
overlap@256.

This is not a training-depth positive signal.  Loss continues to decrease, but
the dense-equivalence gates do not improve into a valid checkpoint.  The result
is consistent with M728 and M729: the current direct query-side pair objective
can move boundaries, but the trainable residual does not know how to cross the
top100 boundary without perturbing dense support geometry.

## Decision

Do not promote M730.  Do not continue by only tuning the two scales or training
longer.  The useful conclusion is architectural:

- the two-head boundary/tail split is insufficient as implemented;
- strict fallback behavior worked and prevented a bad checkpoint from being
  reported as a gain;
- the next first-stage probe should change the preserved surface itself, for
  example an oracle-derived safe projection, an isolated doc-side compiler
  probe, or another geometry where allowed query deltas are constrained before
  training.

M730 therefore closes this branch as a negative but informative first-stage
dense-equivalence probe.
