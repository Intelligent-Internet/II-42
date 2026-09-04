# M1600A-S1C Balanced Query Router Report

## Decision

**Hold a small positive router signal; do not promote it.** The fixed-source
low-rank query router improved both heldout overlap metrics at step 200, but it
captured only a tiny fraction of the qrels-free teacher capacity and regressed
with additional training.

An exact second seed reproduced the small gain, authorizing one unchanged
full-data run. The full-data result is recorded separately in the M1600A-S2
report. It did not authorize loss, rank, threshold, or depth sweeps.

## Locked Run

- ClearML task: `a76f8a4060d1497e9cc36154a34f9ece`.
- Frozen source: M1600A-S1B 8 x 512 codebook, one document key per group.
- Query router: rank-64 residual over the frozen nearest-code logits.
- Teacher: greedy dense-top256 key utility with head weight 10, tail weight 1,
  and exact posting-length cost.
- Read cap: fixed 0.15x.
- Training/validation: the same 4,000/500 disjoint qrels-free surfaces used by
  S1A/S1B.

## Result

| Surface | O@10 | O@100 | O@256 | Reads | Candidate union |
| --- | ---: | ---: | ---: | ---: | ---: |
| frozen nearest-code base | 0.948200 | 0.754980 | 0.599649 | 0.149844x | 0.107961 |
| greedy teacher | 1.000000 | 1.000000 | 0.967070 | 0.149844x | 0.129479 |
| selected step 200 | 0.949400 | 0.758060 | 0.600443 | 0.149844x | 0.107264 |

Selected deltas versus the frozen base:

- O@10: +0.001200;
- O@100: +0.003080;
- O@256: +0.000794;
- reads: unchanged.

The trained trajectory was not monotonic:

| Step | O@100 | O@256 |
| ---: | ---: | ---: |
| 100 | 0.755880 | 0.600400 |
| 200 | **0.758060** | **0.600443** |
| 300 | 0.743780 | 0.590572 |
| 500 | 0.699680 | 0.557805 |
| 1000 | 0.664700 | 0.532930 |

## Exact Replication

Seed 1603 used the same source, objective, read cap, rank, learning rate, and
schedule. It again selected step 200.

| Seed | Base O@100 | Selected O@100 | Delta | Base O@256 | Selected O@256 | Delta |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1602 | 0.754980 | 0.758060 | +0.003080 | 0.599649 | 0.600443 | +0.000794 |
| 1603 | 0.754980 | 0.757500 | +0.002520 | 0.599649 | 0.600805 | +0.001156 |

ClearML task for seed 1603:
`a05f9e3fe77f4f2abf018ab77cea5d1f`.

## Interpretation

The source and target are not the remaining problem: the exact teacher
reproduces the S1B oracle at the same read cap. A deployable query-only residual
can move heldout key ordering in the correct direction, so the mapping is not
strictly unobservable. However, the captured gain is less than 2% of the
teacher O@100 gap and less than 1% of the O@256 gap.

The sharp post-step-200 decline while training loss continues to improve is a
generalization failure, not an optimization-depth failure. The replication
shows that the early movement is real, but its magnitude is too small to close
the source-oracle gap. A larger head or more steps would therefore repeat
M1530A-style overfitting.

## Scale Decision

The replication authorized the predeclared 10,000/1,000 run without changing
the mechanism. Cross-corpus work requires that run to improve O@100 by at
least 0.01 and O@256 by at least 0.005 while protecting O@10. Smaller gains
remain evidence of learnability, not product viability.
