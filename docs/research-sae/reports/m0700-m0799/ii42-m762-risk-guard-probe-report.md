# M762 Risk-Guard Probe

## Decision

A small global risk guard over M758-selected rows does not solve the task-level
robustness problem.

The guard can preserve or improve macro metrics, but it still leaves negative
datasets on test. This means the M760 negative rows are not cleanly separable
from positive rows by the current inference-available features.

## Setup

For each M758 budget, the probe:

1. Replays the selected deterministic M758 policy on dev/test.
2. Labels selected rows as negative if any of MAP/NDCG/MRR/CUB/O@100 drops.
3. Trains a small global safe-row classifier on dev selected rows only.
4. Chooses a dev threshold requiring:
   - global gate pass;
   - no dev negative dataset rows.
5. Replays the same threshold on test.

Models tested:

- logistic regression with scaling;
- random forest;
- histogram gradient boosting.

## Results

| Budget | Guard | Test Applied | Test Negative Tasks | Gate | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| top16 | logreg | 122 | cqadupstack, scidocs | 0 | +0.000140 | +0.000210 | -0.000185 | +0.000014 | +0.000000 |
| top16 | random forest | 219 | fiqa, scidocs, trec-covid | 1 | +0.000244 | +0.000320 | +0.000000 | +0.000032 | +0.000000 |
| top16 | HGB | 212 | climate-fever, dbpedia-entity, fiqa, msmarco, scidocs | 1 | +0.000133 | +0.000233 | +0.000000 | +0.000032 | +0.000000 |
| top32 | logreg | 50 | cqadupstack, scidocs | 1 | +0.000067 | +0.000535 | +0.000066 | +0.000001 | +0.000000 |
| top32 | random forest | 62 | cqadupstack, dbpedia-entity, nfcorpus, scidocs | 1 | +0.000054 | +0.000535 | +0.000066 | +0.000001 | +0.000000 |
| top32 | HGB | 62 | cqadupstack, nfcorpus, scidocs | 1 | +0.000058 | +0.000684 | +0.000066 | +0.000012 | +0.000000 |
| top64 | logreg | 137 | climate-fever, cqadupstack, dbpedia-entity | 1 | +0.000065 | +0.000161 | +0.000000 | +0.000020 | +0.000000 |
| top64 | random forest | 195 | climate-fever, trec-covid, webis-touche2020 | 1 | +0.000177 | +0.000213 | +0.000000 | +0.000013 | +0.000000 |
| top64 | HGB | 189 | climate-fever, trec-covid, webis-touche2020 | 1 | +0.000177 | +0.000213 | +0.000000 | +0.000007 | +0.000000 |

## Interpretation

The risk guard is not useless, but it does not cross the promotion boundary:

- top32 HGB preserves the strong NDCG/MRR movement, but still has negative
  datasets;
- top16 random forest keeps the best MAP/CUB profile, but also keeps negative
  datasets;
- top64 guards mostly collapse back to the original M758 risk profile.

This suggests the available global features encode "macro safe enough", but not
"dataset/query locally safe". The negative rows are small and sparse, yet they
are not isolated by current context margins, proposal counts, or overlap floors.

## Next Direction

Do not keep tuning global risk guards. The next meaningful route is one of:

1. query-local native feedback: run a cheap local replay estimate or native
   score-stability check before accepting a delta;
2. task-family calibration without dataset-specific tuning: cluster rows by
   observable native geometry rather than dataset name, then use separate
   thresholds per cluster;
3. stronger proposal construction: generate proposals that are already
   row-local safe instead of relying on a post-hoc guard.

For immediate continuation, M763 should test option 2 first because it is still
qrels-free at inference time and cheaper than adding online native replay.
