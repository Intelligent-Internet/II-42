# M1726 Full-Scale Load-Generalization Contract

## Question

M1725's complete train-corpus DF table reads `0.171965x` at the quality
optimum while the disjoint validation index reads `0.209341x`. M1726 asks:

> Is this query-selected posting-load shift a 35,831-document estimation
> problem, or a structural failure of the learned semantic namespace?

This is a data-scale diagnostic, not a waiver of the failed canary gate.

## Frozen Full Surface

- M1600 10,000/1,000 query pools;
- 88,992 train and 8,988 validation documents;
- M1600 full 8x512 source initialization;
- BM25 top256 materialized independently on both pools;
- identical M1725 residual and dense-control objectives;
- full hard DF refresh every 50 updates;
- 1,600 updates, batch 16, 128 candidates, 96 teacher documents;
- fixed 8 query probes/group, `0.18x` target, lambda 100;
- seed 1726;
- no qrels or dataset-specific policy.

At every validation checkpoint, record both exact train-pool query-selected
posting reads and exact validation reads. Training loss cannot substitute for
this measurement.

## Gate

The unchanged product gate requires:

- trained residual checkpoint;
- O@100 gain `>=0.01`, O@256 gain `>=0.005`;
- residual R@100 gain `>=0.01`, R@256 gain `>=0.005`;
- validation reads `<=0.18x`, max DF `<=0.02`;
- residual variant score `>=0.002` above the dense control.

The load-generalization diagnostic additionally records the train-validation
read gap. A smaller gap without a product-gate pass is useful diagnosis but
does not authorize further training.

## Stop

- If the full residual arm fails the unchanged gate, stop this joint source.
- Do not respond with more steps, a second lambda, fewer probes, head weights,
  or a relaxed read target.
- Only a full gate pass may run a second seed; only two full passes may proceed
  to direct additive scoring and native index evaluation.
