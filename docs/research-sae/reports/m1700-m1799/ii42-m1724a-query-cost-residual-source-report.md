# M1724A Query-Cost Residual Source Report

## Decision

**Do not scale M1724 unchanged. Retain the stronger quality signal and reject
the minibatch cost surrogate.** M1724 improves the residual quality frontier
again and produces a cost-safe checkpoint that passes every gate except the
fixed O@100 gain. The intended query-cost penalty did not track exact heldout
posting reads closely enough to constrain the quality optimum.

ClearML task: `e0a7d6b2d5e44123affae55c8dcfb255`.

## Matched Result

| Variant | Selected step | O@100 | O@256 | R@100 | R@256 | Reads | Max DF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Dense control | 400 | 0.820760 | 0.681898 | 0.741866 | 0.588669 | 0.176842x | 0.012228 |
| Residual | 800 | 0.822060 | 0.686297 | 0.747475 | 0.596192 | 0.177331x | 0.013117 |

The residual checkpoint is trained, cost-safe, max-DF-safe, and exceeds the
same-seed dense control. Relative to initialization it gains:

- O@100 `+0.006440`;
- O@256 `+0.021477`;
- residual R@100 `+0.013386`;
- residual R@256 `+0.031300`.

Only the O@100 requirement (`>=+0.01`) fails. The result is stronger and safer
than M1723 step 700, but remains below the predeclared scale gate.

## Quality Frontier

| Step | O@100 | O@256 | R@100 | R@256 | Exact reads | Max DF |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 300 | 0.828040 | 0.689945 | 0.751334 | 0.597845 | 0.183463x | 0.012228 |
| 400 | 0.840440 | 0.711711 | 0.770025 | 0.626661 | 0.207626x | 0.013562 |
| 500 | 0.835580 | 0.705992 | 0.764206 | 0.620907 | 0.200756x | 0.014229 |
| 700 | 0.824780 | 0.690836 | 0.750143 | 0.602016 | 0.182207x | 0.014006 |
| 800 | 0.822060 | 0.686297 | 0.747475 | 0.596192 | 0.177331x | 0.013117 |

The step-400 quality gain is substantial:

- O@100 `+0.024820`;
- O@256 `+0.046891`;
- residual R@100 `+0.035936`;
- residual R@256 `+0.061769`.

The cost issue remains localized to query-selected posting lists. Max DF stays
well below 2% through the quality peak.

## Why The Surrogate Failed

The training loss estimated posting reads from 128 hard/soft document
assignments sampled in the current minibatch. At residual step 400:

- estimated train-batch reads: `0.182129x`;
- exact heldout posting reads: `0.207626x`;
- squared excess penalty before weight: only `4.53e-6`.

The proxy underestimates the corpus-level DF of query-selected keys, so
`lambda=100` contributes less than `0.001` around the actual quality optimum.
Increasing lambda would tune a biased proxy and is explicitly rejected.

## Final Authorized Repair

One final source-cost experiment is justified:

- compute the hard `(group,key)` DF table over the complete train corpus;
- refresh it every 50 updates as source assignments move;
- treat the DF table as fixed metadata during each interval;
- backpropagate query assignment against exact posting-list fractions;
- keep the same target `0.18x`, coefficient 100, architecture, probes, data,
  control arm, and scale gate.

This is not another loss-weight variation. It replaces a measured-biased
estimator with the actual index statistic. If it cannot preserve O@100 gain
`>=0.01` under the exact cost cap, stop the joint residual-source route and
record M1724 as its bounded frontier.

## Reproducibility

- host: `spark-1`;
- run:
  `/home/huoju/leask/runs/ii42-m1724-query-cost-v1/runs/m1724a-query-cost-residual-source4k-seed1724-v1`;
- summary SHA-256:
  `76c2ea17ac93d8a8408a930dc5d1e623a6b74384fd1365bd0f40ad0a4c2013ad`;
- local result: `ii42-m1724a-query-cost-residual-source-result.json`.
