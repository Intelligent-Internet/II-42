# M361A Atom-vs-Dense Exact Overlap

## Scope

This report compares exact dense hidden-mean cosine top-k against M320 pooled
atom-vector top-k. It deliberately excludes BM25, IDF, scorer, and qrel
reranking.

The run uses the local M320 checkpoint:

- `/Volumes/Betty/Tmp/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`

The evaluation uses the local all-test roots:

- `/Volumes/Betty/Tmp/ii42-m1000/official-unified-v1/all-test`

This is a reduced but current-checkpoint representation audit:

- datasets: `nfcorpus`, `scifact`
- docs: 1024 sampled docs per dataset
- queries: 64 qrel-bearing queries per dataset
- top-k: 100
- device: local `mps`

## Command

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m361_atom_dense_overlap.py \
  --output-dir /tmp/ii42-m361-atom-dense-overlap-reduced \
  --dataset nfcorpus \
  --dataset scifact \
  --max-docs-per-dataset 1024 \
  --max-queries-per-dataset 64 \
  --top-k 100 \
  --hidden-batch-size 8 \
  --device mps \
  --trust-remote-code
```

Outputs:

- `/tmp/ii42-m361-atom-dense-overlap-reduced/m361_atom_dense_overlap.json`
- `/tmp/ii42-m361-atom-dense-overlap-reduced/m361_atom_dense_overlap.md`

## Results

| Dataset | Docs | Queries | Score | Overlap@10 | Overlap@20 | Overlap@100 | NDCG@10 vs dense100 | MRR vs dense100 |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 1024 | 64 | `sparse_cosine` | 0.0984 | 0.1281 | 0.2495 | 0.3939 | 0.6438 |
| `nfcorpus` | 1024 | 64 | `sparse_dot` | 0.0844 | 0.1063 | 0.2280 | 0.3370 | 0.5138 |
| `scifact` | 1024 | 64 | `sparse_cosine` | 0.1375 | 0.1617 | 0.2603 | 0.4900 | 0.7218 |
| `scifact` | 1024 | 64 | `sparse_dot` | 0.0625 | 0.0680 | 0.1789 | 0.2511 | 0.4180 |

## Interpretation

This confirms the main M361 concern on a current M320 checkpoint: the pure atom
representation is not dense-neighborhood faithful even before BM25, IDF,
scorer, or final admission logic is involved.

`sparse_cosine` is the primary representation-fidelity score. It removes vector
norm effects and still gives only 0.098 to 0.138 Top10 overlap with exact dense
top10. `sparse_dot` is worse, which also supports keeping normalized/cosine
gates as mandatory diagnostics.

The practical implication is that continuing to rescue the current atom route
with downstream scorer/admission objectives is unlikely to cross dense. The
next valuable route is representation-first training:

1. Train directly against dense neighborhood identity/ranking.
2. Gate by exact dense top-k overlap, not reconstruction loss or qrel metrics
   alone.
3. Treat BM25/scorer work as downstream only after atom-only dense overlap is
   materially higher.
