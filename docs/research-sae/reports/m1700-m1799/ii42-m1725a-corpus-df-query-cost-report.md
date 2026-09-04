# M1725A Corpus-DF Query-Cost Report

## Decision

**The 4,000/500 canary remains below the promotion gate, but it identifies a
data-scale/load-generalization hypothesis that merits one diagnostic full
run.** The full-corpus DF table does not remove the quality/cost frontier. It
does prove that the remaining error is not a biased minibatch estimator:
semantic key load itself shifts between disjoint document pools.

ClearML task: `fdaff7a9320e4a568e83afa057b6ed76`.

## Cost-Safe Result

| Variant | Step | O@100 | O@256 | R@100 | R@256 | Reads | Max DF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Dense control | 500 | 0.820260 | 0.681070 | 0.741481 | 0.587315 | 0.174597x | 0.012228 |
| Residual | 800 | 0.822400 | 0.689289 | 0.746016 | 0.599811 | 0.179171x | 0.016452 |

Residual gains versus initialization are:

- O@100 `+0.006780`;
- O@256 `+0.024469`;
- residual R@100 `+0.011926`;
- residual R@256 `+0.034919`.

The checkpoint is trained, beats the same-seed dense control, and passes
O@256, both residual recovery gates, exact read cost, and max DF. O@100 is the
only failed requirement.

## Quality Peak

Residual step 500 reaches:

| O@100 | O@256 | R@100 | R@256 | Validation reads | Max DF |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.842620 | 0.716258 | 0.773711 | 0.633313 | 0.209341x | 0.018008 |

Relative to initialization this is `+0.027000/+0.051438` O@100/O@256 and
`+0.039622/+0.068421` residual recovery. The learned residual source is real;
it is not product-cost-safe at its quality optimum.

## Cross-Split Load Failure

M1725 recomputed hard source DF over all 35,831 train documents every 50
updates. At step 500:

- exact train-corpus query-selected DF estimate: `0.171965x`;
- exact validation posting reads: `0.209341x`;
- train-side cost penalty: zero because train cost is below `0.18x`.

Thus the differentiable query cost is aligned with the index statistic on the
corpus it sees. The failure is that key assignments and query preferences have
different load on the 4,498 heldout documents. A larger and more diverse
document pool may stabilize DF; increasing the coefficient cannot.

## Decision Boundary

The M1723-M1725 canary cost-repair family is closed. No proxy, lambda, probe,
depth, or threshold variant follows.

One non-promotion scale diagnostic is authorized because it tests a different
cause: sample-size dependence of semantic-key DF. It must use the existing
M1600 10,000/1,000 pools and 88,992/8,988 documents, report exact train and
validation query-selected reads, and retain every existing gate. It cannot be
reported as progress merely because a quality checkpoint exists.

If the larger pool does not pass O@100 and cost simultaneously, stop this
representation. If it does, require a second full-scale seed before native
scoring or qrel evaluation.

## Reproducibility

- host: `spark-1`;
- run:
  `/home/huoju/leask/runs/ii42-m1725-corpus-df-v1/runs/m1725a-corpus-df-residual-source4k-seed1725-v1`;
- summary SHA-256:
  `34f644216b92b07aa02b1ad56f8ed5ce2d1dc02c59f388827c10fdee2697c575`;
- local result: `ii42-m1725a-corpus-df-residual-source-result.json`.
