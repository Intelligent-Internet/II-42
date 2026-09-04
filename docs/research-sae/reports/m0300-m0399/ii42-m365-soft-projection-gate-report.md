# II-42 M365 Soft Projection Gate Report

## Goal

M365 continues the dense-only track. It tests two ideas proposed after M364:

- train with straight-through soft TopK, evaluate with hard TopK;
- add a per-atom scale projection head while preserving posting compatibility.

No BM25, qrels, scorer features, or admission objectives are used.

## Setup

- Input: `/Volumes/Betty/Tmp/psql_bm25s_sae_m80/neutral-stage-a-v0`
- Docs / queries: 4096 / 128
- Seed: 362
- Latent dims / active atoms: 2048 / 256
- Epochs: 20
- Gate: exact dense top-k overlap on heldout query split

## Results

| Variant | Train mode | Score mode | Best epoch | Exact O@10 | Exact O@20 | Exact O@100 | Candidate O@10 | NDCG tax |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hard recon | hard ReLU | plain | 10 | 0.7304 | 0.6913 | 0.6591 | 0.7304 | -0.1972 |
| ST recon | straight-through softplus | plain | 1 | 0.4957 | 0.4696 | 0.4157 | 0.5087 | -0.4077 |
| ST gram | straight-through softplus | plain | 1 | 0.4913 | 0.4543 | 0.3978 | 0.5087 | -0.4163 |
| ST scaled score | straight-through softplus | scaled | 1 | 0.4565 | 0.4783 | 0.3957 | 0.4826 | -0.4531 |
| hard scaled score | hard ReLU | scaled | 20 | 0.7130 | 0.6935 | 0.6704 | 0.7261 | -0.2187 |

## Interpretation

M365 is a negative result.

- Straight-through soft TopK training is much worse than hard TopK.
- The per-atom scale projection head does not beat hard reconstruction.
- The best hard-scaled variant improves O@100 slightly but hurts O@10 and NDCG
  tax, which is not acceptable for promotion.

This means the M364 route should not continue by adding soft-to-hard or simple
per-atom scaling. The next useful step is to challenge learned SAE atoms against
a much stronger dense-coordinate posting baseline.

## Decision

Do not promote M365. Do not add BM25 or qrel supervision on top of this.

The project should pivot to M366-style dense-coordinate or rotated-coordinate
postings as the baseline to beat.
