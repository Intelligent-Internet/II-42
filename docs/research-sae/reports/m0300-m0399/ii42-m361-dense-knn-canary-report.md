# M361 Dense-kNN Sparse Preservation Canary

## Scope

This is a no-checkpoint-required canary over existing M80 dense embeddings.
It tests whether the sparse projector objective itself can preserve exact
dense query-document neighborhoods.

The run is intentionally local and small:

- It does not touch `spark-1`, `spark-2`, or any ongoing training job.
- It filters the mixed M80 artifact to the shared 768-dimensional dense
  embedding surface.
- It uses exact dense top-k as teacher labels, with no BM25 and no scorer.

## Data Filter

| Surface | Dimension counts | Kept dimension | Skipped |
| --- | --- | ---: | ---: |
| documents | `0:400`, `256:800`, `768:27151` | 768 | 1200 |
| queries | `0:100`, `768:440` | 768 | 100 |

The 256-dimensional and empty rows are excluded. Mixing them would corrupt a
dense-faithfulness test because they are not the same encoder surface.

## Run

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m361_dense_knn_canary.py \
  --output-dir /tmp/ii42-m361-dense-knn-canary \
  --max-docs 4096 \
  --max-queries 128 \
  --teacher-top-k 32 \
  --random-negative-k 32 \
  --latent-dims 512 \
  --active-dims 32 \
  --epochs 2 \
  --batch-rows 8 \
  --device cpu
```

Outputs:

- `/tmp/ii42-m361-dense-knn-canary/m361_dense_knn_canary.json`
- `/tmp/ii42-m361-dense-knn-canary/m361_dense_knn_canary.md`

## Metrics

| Variant | Top10 overlap | Top20 overlap | Sparse NDCG@10 | Dense NDCG@10 | NDCG tax | MRR tax | Recall tax |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `recon_only` | 0.4409 | 0.6841 | 0.4887 | 1.0000 | -0.5113 | -0.3950 | -0.4455 |
| `knn_kl` | 0.4909 | 0.6386 | 0.5457 | 1.0000 | -0.4543 | -0.2776 | -0.4091 |
| `knn_kl_pair` | 0.4682 | 0.6295 | 0.5742 | 1.0000 | -0.4258 | -0.2598 | -0.3727 |
| `knn_kl_ce` | 0.4455 | 0.6068 | 0.5395 | 1.0000 | -0.4605 | -0.2683 | -0.4455 |

Train/eval rows: 106 train, 22 eval.

## Interpretation

The first signal is clear: reconstruction-only sparse projection is not a
dense-neighborhood preservation objective. Adding dense-neighborhood KL and
pairwise ranking improves sparse NDCG and tax versus `recon_only`.

The second signal is also clear: the small objective fix is not enough. Even
the best canary remains far from dense, with Top10 overlap below 0.50 and
NDCG tax around -0.43. That means the current problem is not just downstream
BM25/scorer admission. The representation training objective and active-support
budget are still primary blockers.

The best current canary is `knn_kl_pair` on NDCG/MRR/recall tax, while
`knn_kl` has the best Top10 overlap. This suggests a real tradeoff between
neighborhood identity and ranked positive concentration, so future runs should
gate both exact dense top-k overlap and sparse-tax metrics.

## Decision

Promote this line from hypothesis to a controlled representation-first
experiment:

1. Run exact dense top-k versus current atom-only top-k overlap on current
   M320/M344 exports. This verifies whether the production-style atom route is
   suffering the same failure, not just the small projector canary.
2. Train a small current-checkpoint dense-kNN objective ablation with KL-only,
   KL+pairwise, and KL+qrel CE gates.
3. Do not spend the next main effort on posthoc scorer rescue until the
   atom-only dense-faithfulness gap is quantified.
