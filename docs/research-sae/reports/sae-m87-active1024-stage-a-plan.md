# SAE M87 Active-1024 Stage-A Probe

Status: closed. M85 showed that k512 improves NDCG and Recall but still leaves
MRR/top-positive ordering outside the Stage-A gate. M86 low-dose and normal
positive-margin losses degraded MRR, so qrel-margin scalar supervision is not
the next promising path.

M87 tests one clean question:

```text
Can the current representation close Stage A if active support is no longer the
main bottleneck?
```

The first run uses `latent_dims=8192`, `active_dims=1024`, KL `4.0`, pairwise
`0.5`, reconstruction `0.25`, and no qrel-positive correction. If it passes,
Stage A is representation-feasible and the next work is support compression. If
it fails, the model needs a different representation or atom allocation method,
not more scalar loss tuning.

## Result

M87 itself was near-miss, not a direct pass. It selected epoch 16 and produced
overall Recall@10/MRR/NDCG@10 tax of about
`-0.00699/-0.02081/-0.01322`, failing only the overall Recall and MRR gates.

The follow-up M88/M89 runs split the tradeoff:

- M88 selected the MRR-favorable epoch 9 and passed MRR/NDCG, but missed Recall.
- M89 selected the Recall-favorable epoch 2 and passed Recall/NDCG, but missed
  MRR.

M90 then interpolated those two compatible checkpoints into one sparse encoder.
The canonical `alpha=0.35` checkpoint with baseline scoring passed the full
Stage-A gate:

| Scope | Recall@10 tax | MRR tax | NDCG@10 tax |
| --- | ---: | ---: | ---: |
| Overall | `-0.00449` | `-0.01864` | `-0.00844` |
| BEIR15 current eval | `+0.00522` | `-0.02308` | `-0.01391` |
| Broad generated query | `-0.00596` | `-0.01878` | `-0.00780` |
| Large supervised split | `+0.00000` | `-0.01111` | `-0.00880` |

Conclusion: Stage A is closed at `8192/k1024`. The next blocker is support
compression, not another scalar ranking loss sweep.
