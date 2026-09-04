# M654V Overlap50 Gate Probe

Status: `not_promoted_no_checkpoint`

M654V tests one narrow hypothesis from the M654U shared15 failure: the selected
M654U checkpoint had positive dev O@100/O@256 but negative dev O@50, then
failed held-out O@100.  M654V keeps the M654U boundary-constrained teacher
unchanged and adds `--overlap-gate-k-values 50,100,256` for checkpoint
selection.

It remains a first-stage probe.  It does not use BM25, rerankers, learned
gates, qrels-driven training, or per-dataset tuning.

## Run

Run: `m654v_shared15_overlap50_seed6546`.

Surface: all 15 shared15 datasets, all-query split.

Examples: train 1074, dev 134, test 134.

Decision: `dense_equivalence_gate_failed`.

Failed check: `trained_checkpoint_selected`.

No trained checkpoint passed the stricter dev gate, so final output fell back
to epoch0/no-op.

## Key Evidence

Late dev trace shows the O@50 guard itself is not the bottleneck:

| Epoch | dO@50 | dO@100 | dO@256 | dRecall@100 | dSupport cosine |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | +0.000133 | -0.000061 | -0.000047 | 0.000000 | -0.000000127 |
| 11 | +0.000133 | -0.000061 | -0.000048 | 0.000000 | -0.000000147 |
| 12 | +0.000133 | -0.000061 | -0.000084 | 0.000000 | -0.000000139 |

Adding O@50 as a selection gate does not fix O@100 instability.  In this seed,
the model can improve O@50 while losing O@100/O@256.  That means the failure is
not a simple mid-rank overlap predictor issue.

## Conclusion

M654V rejects the "add O@50 gate" fix.  The broader shared15 failure is better
explained as compiler transfer losing exact top100 membership for a few
queries, even when the teacher target is per-query safe.

The next attempt should move from selection-only guards to a training-time
dense boundary preservation loss.  The loss should protect P1 baseline
dense-overlap documents around top100 while distilling M654U's teacher.  That
directly targets the observed failure: safe teacher, unsafe global compiler
transfer.
