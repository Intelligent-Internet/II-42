# M766 Fixed Top16 Rank-Trace Replay Seed7642

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
| dev | 0 | dbpedia-entity, hotpotqa, scidocs | 57 | +0.000000 | -0.000250 | -0.000102 | +0.000119 | +0.000000 | +0.000000 |
| test | 1 | trec-covid | 58 | +0.000000 | +0.000225 | +0.000203 | +0.000185 | +0.000042 | +0.000000 |

## Decision

M766 fixed guard failed held-out negative-task replay.
