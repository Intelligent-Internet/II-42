# M660-M662 Delta Trust-Region Sweep Report

## Summary

M660-M662 tested whether the M658 near-miss was mainly caused by too much query
movement rather than insufficient training time. The probe keeps the M658 loss
shape and seed fixed, removes M659's top100 replay loss, and only reduces the
query compiler residual scale.

Result: reducing movement is the most useful signal after M658. It sharply
reduces dense-hit loss queries, but it also removes retrieval gains and still
does not pass the strict dense-equivalence gate. This suggests the next design
should be query-adaptive trust-region or exact support-preserving projection,
not longer training of the current objective.

## Runs

| Run | delta scale | Selected | Dev gain/loss | Test gain/loss |
| --- | ---: | --- | --- | --- |
| M658 | `0.001` | `epoch1/step17` | `4/1` | `2/7` |
| M660 | `0.0005` | `epoch1/step17` | `1/0` | `2/2` |
| M661 | `0.00025` | `epoch1/step17` | `1/0` | `2/2` |
| M662 | `0.0001` | `epoch3/step51` | `1/0` | `1/1` |

Test deltas versus P1:

| Run | O@100 | O@256 | R@100 | MAP@100 | NDCG@10 | support cosine | active support |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M658 | `-0.000404521` | `-0.000023345` | `+0.000595238` | `+0.000142345` | `0.000000000` | `-0.000000381` | `0.000000000` |
| M660 | `-0.000005051` | `+0.000020057` | `0.000000000` | `-0.000008912` | `0.000000000` | `-0.000000083` | `-0.000406784` |
| M661 | `-0.000005051` | `+0.000030908` | `0.000000000` | `-0.000004691` | `0.000000000` | `-0.000000008` | `-0.000047348` |
| M662 | `-0.000005051` | `-0.000030579` | `0.000000000` | `-0.000006442` | `0.000000000` | `-0.000000024` | `-0.000220960` |

## Interpretation

This directly addresses the training-depth concern.

Longer training is not the main missing factor for the current loss shape:

- M658 at `0.001` moves enough to create small Recall/MAP gains, but spends
  dense top100 membership.
- M660/M661/M662 reduce the movement budget and sharply reduce loss queries.
- The same reduced movement also removes the retrieval improvement.
- All reduced-scale runs still fail at least one dense-equivalence guard.

So the useful signal is not "train longer". It is "safe movement is extremely
small and must be query-conditioned".

## Conclusion

The current first-stage compiler has a narrow safe operating range:

- too much movement: qrels metrics can improve, but dense identity is spent;
- too little movement: dense damage shrinks, but improvement disappears;
- fixed global scale cannot find a clean safe point.

Next probe should implement query-adaptive movement:

- estimate each query's P1 rank100/rank101 margin;
- assign smaller movement budgets to low-margin boundary queries;
- allow larger movement only when the baseline boundary margin is safe;
- project generated queries back to exact active support before scoring;
- keep the same strict dense-equivalence gate.

This is still first-stage work. BM25, reranking, qrels loss, and learned gates
remain out of scope.
