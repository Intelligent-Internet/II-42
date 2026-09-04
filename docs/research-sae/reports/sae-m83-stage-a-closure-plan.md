# SAE M83 Stage-A Closure Plan

Status: closed by M90. M82 proved that ranking pressure can repair the BEIR15
sparse tax, but it moved some loss into broad/generated and overall MRR. M83
therefore targets Stage-A closure with balanced training pressure and
collapse-aware checkpoint selection.

## M82 Finding

Best useful M82 run:

```text
rank-kl4-pair05-recon025-text120k
```

Tax versus dense student:

| Scope | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| Overall | -0.0035 | -0.0271 | -0.0147 |
| BEIR15 | +0.0152 | -0.0016 | +0.0031 |
| Broad generated | -0.0058 | -0.0306 | -0.0166 |
| Large supervised | +0.0000 | -0.0196 | -0.0151 |

This is not a representation-capacity failure. It is a pressure-allocation and
model-selection problem: the model can preserve BEIR15 ranking when asked, but
the current objective can transfer tax into other families.

## M83 Changes

Trainer updates:

- `--candidate-sampling family_balanced`
- `--candidate-sampling dataset_balanced`
- `--selection-metric stage_a_closure_gate`

The new gate rewards overall sparse preservation but penalizes family-level
MRR/NDCG/Recall collapse. Training-time balanced sampling is used only for
candidate rows; evaluation remains unchanged and weighted by the true eval
surface.

## Matrix

First closure matrix:

| Run | Sampling | KL | Pairwise | Recon | Text records |
| --- | --- | ---: | ---: | ---: | ---: |
| `m83-family-kl4-pair05-recon025-text120k` | family | 4.0 | 0.5 | 0.25 | 120k |
| `m83-family-kl3-pair04-recon035-text120k` | family | 3.0 | 0.4 | 0.35 | 120k |
| `m83-dataset-kl4-pair05-recon025-text120k` | dataset | 4.0 | 0.5 | 0.25 | 120k |
| `m83-family-kl2-pair03-recon035-text160k` | family | 2.0 | 0.3 | 0.35 | 160k |

## Stage-A Closure Target

Stage A is considered closed for the current k256 profile if a run reaches:

- overall Recall@10 tax `>= -0.006`;
- overall MRR tax `>= -0.020`;
- overall NDCG@10 tax `>= -0.015`;
- every source-family NDCG@10 tax `>= -0.020`;
- no source-family Recall@10 tax below `-0.010`.

If this passes, Stage A can move to a lower-k Pareto curve and then Stage B. If
it fails, the next step is not more scalar loss tuning; it is changing the SAE
representation or adding an explicit qrel-positive soft top-k loss.

## Closure Update

M83-M86 did not close the gate. M87 raised active support to `k1024` and nearly
closed it, M88/M89 isolated rank-favorable and recall-favorable checkpoints,
and M90 closed Stage A by interpolating those two checkpoints into a single
encoder. The promoted M90 `alpha=0.35` baseline-scored point reaches overall
Recall@10/MRR/NDCG@10 tax of `-0.00449/-0.01864/-0.00844`, passing the full
overall and source-family gate.
