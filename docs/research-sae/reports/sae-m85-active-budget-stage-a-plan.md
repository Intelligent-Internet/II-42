# SAE M85 Active-Budget Stage-A Probe

Status: active. M84 low-weight label-positive CE showed early negative signal:
it did not improve the MRR/top-rank tax enough and often made the closure gate
worse. The next question is whether Stage A is blocked by objective scalar
tuning or by active-support capacity.

## Hypothesis

If `active_dims=384` or `active_dims=512` closes Stage A while the same
objective at `active_dims=256` does not, then the blocker is atom allocation and
support capacity. That would mean the correct next move is retrieval-aware atom
allocation or adaptive active budget, not more CE/KL/pairwise weight sweeps.

If larger active budgets still fail, the issue is representation or score
calibration itself, and Stage A needs a different architecture before Stage B.

## Probe Matrix

All runs use the M82 ranking-first objective unless noted:

| Run | Active dims | KL | Pairwise | Recon | Label CE |
| --- | ---: | ---: | ---: | ---: | ---: |
| `m85-k384-kl4-pair05-recon025` | 384 | 4.0 | 0.5 | 0.25 | 0.0 |
| `m85-k512-kl4-pair05-recon025` | 512 | 4.0 | 0.5 | 0.25 | 0.0 |
| `m85-k512-kl3-pair04-recon035` | 512 | 3.0 | 0.4 | 0.35 | 0.0 |

## Stage-A Gate

- overall Recall@10 tax `>= -0.006`;
- overall MRR tax `>= -0.020`;
- overall NDCG@10 tax `>= -0.015`;
- every source-family NDCG@10 tax `>= -0.020`;
- no source-family Recall@10 tax below `-0.010`.

Passing at k384/k512 does not mean the final index should ship with that active
budget. It means Stage A can be closed at the representation level, and the next
work should compress the support without losing the closed-quality profile.
