# M1610A Hierarchical Path Source Report

## Executive Decision

**Stop M1610A.** The apparent trained gain at a 256-leaf query beam was a
read-budget underfill artifact. At the same posting-read budget, the trained
hierarchy was materially worse than its deterministic initialization at every
dense-overlap depth. A second seed and larger surface are therefore forbidden
by the predeclared stop rule.

This is a source-level negative result. It does not use qrels, BM25, ANN, or a
post-retrieval dense guard.

## Tested Hypothesis

M1610A tested whether an EHI-style shared conditional hierarchy could make
query/document posting compatibility locally observable:

```text
frozen pooled dense-root
    -> four conditional 8-way decisions
    -> one hard document path/leaf
    -> bounded query leaf beam
    -> exact posting union
```

The 4,096-leaf hierarchy was initialized by hierarchical spherical k-means.
The qrels-free objective combined symmetric positive-path matching, a
positive/negative path margin, and conditional child balance. Training used
4,000 MS MARCO-derived queries and 35,831 documents; validation used 500
disjoint queries and 4,498 documents.

## Initial Result And Audit Trigger

The first 256-beam evaluator selected step 100. At the nominal `0.15x` cap it
appeared to improve all overlap metrics:

| Checkpoint | O@10 | O@100 | O@256 | Actual reads |
| --- | ---: | ---: | ---: | ---: |
| initial | 0.901000 | 0.668780 | 0.512307 | 0.065419x |
| selected | 0.908200 | 0.736000 | 0.616086 | 0.136579x |
| apparent delta | +0.007200 | +0.067220 | +0.103779 | +0.071160x |

The result passed the original automated gate, but the two checkpoints did
not consume comparable budgets. The initial hierarchy exhausted the 256-leaf
beam after reading only `0.065419x`, while the trained hierarchy read more
than twice as many postings. This violated the experiment's causal comparison
contract even though both values were below the cap.

## Matched-Budget Replay

The repair changed only the evaluator beam, not the model or source. Beams of
256, 512, 1,024, and 2,048 were replayed until both checkpoints filled the
same hard read cap. At beam 2,048, both sides consumed `0.074922x` and
`0.149844x`, respectively.

| Cap | Checkpoint | O@10 | O@100 | O@256 | Reads |
| ---: | --- | ---: | ---: | ---: | ---: |
| 0.075x | initial | 0.915000 | 0.701020 | 0.546820 | 0.074922x |
| 0.075x | selected | 0.813200 | 0.570520 | 0.443883 | 0.074922x |
| 0.075x | delta | -0.101800 | -0.130500 | -0.102937 | 0.000000x |
| 0.150x | initial | 0.959200 | 0.839040 | 0.729539 | 0.149844x |
| 0.150x | selected | 0.911000 | 0.747000 | 0.634453 | 0.149844x |
| 0.150x | delta | -0.048200 | -0.092040 | -0.095086 | 0.000000x |

The trained source increased max document frequency from `0.002890` to
`0.022232` and used that additional fanout to hide the loss of path quality
under the underfilled evaluator. Continued training eventually collapsed to
max DF `0.901289`, while O@100 fell near `0.124`.

## Interpretation

1. The hierarchy has ample source capacity: its exact source oracle reaches
   O@10/O@100/O@256 of `1.0` at `0.106826x` reads.
2. Conditional path training did not make the useful leaves more predictable
   from the query. It made leaves broader, which raised reads and created the
   false positive.
3. The failure is not fixed by more training. The final checkpoint is a much
   stronger collapse while the optimization objective continued to run.
4. A frozen-source query-router repair would repeat M1600's already-closed
   low-rank observability experiment, so it is not a distinct mechanism.

M1610A therefore establishes a useful evaluator rule: a read cap is not a
matched-cost comparison unless both policies fill it to the same tolerance.
Future posting-source gates must report budget fill and either match reads or
compare a full quality-cost frontier.

## Reproducibility

- Host: `spark-1`.
- Container: `nvcr.io/nvidia/pytorch:26.03-py3`.
- ClearML integration task: `48e2f9669da8484281055917a499c18d`.
- ClearML heldout task: `955a19965cd04d528f2b6db9499793b5`.
- Run:
  `/home/huoju/leask/runs/ii42-m1610-retrieval-source-v1/runs/m1610a-s1-hierarchical-path-seed1610`.
- Audit:
  `/home/huoju/leask/runs/ii42-m1610-retrieval-source-v1/runs/m1610a-s1-matched-budget-replay-seed1610`.
- Selected checkpoint SHA-256:
  `b1f93e257623b895625c5ec25037e141a49c52a7860d64b55e9ae6bd86c25578`.
- Matched-budget summary SHA-256:
  `fef917770780ef0670dfb86fd11b2d7954f6449457d89578a71e9eed496030ee`.
