# M767 Fixed Top16 Rank-Trace Replay Seed7643

This report replays a fixed M764 rank-trace guard. It does not search
or retune the threshold on this surface.

## Guard

```json
{
  "feature": "trace_position_keep_at_128",
  "mode": "le",
  "threshold": 0.4765625
}
```

## Results

| Split | Gate | Negative Tasks | Applied | dRecall | dMAP | dNDCG | dMRR | dCUB | dO@100 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| dev | 1 | scidocs, scifact | 54 | +0.000000 | +0.000124 | +0.000147 | +0.000119 | +0.000004 | +0.000000 |
| test | 0 | dbpedia-entity, hotpotqa, nfcorpus, scidocs | 51 | +0.000000 | -0.000288 | -0.000145 | +0.000000 | +0.000038 | +0.000000 |

## Decision

M766 fixed guard failed held-out negative-task replay.
